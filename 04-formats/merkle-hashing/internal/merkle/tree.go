package merkle

import (
	"crypto/sha256"
	"encoding/hex"
	"encoding/json"
	"errors"
	"fmt"
	"os"
)

const hashSize = sha256.Size

// Node is a Merkle tree vertex.
type Node struct {
	Hash  []byte
	Data  []byte
	Left  *Node
	Right *Node
}

// ProofStep represents one sibling hash in the Merkle proof.
type ProofStep struct {
	SiblingHash []byte
	IsLeft      bool
}

// ProofStepJSON is a JSON-friendly representation of ProofStep.
type ProofStepJSON struct {
	Sibling string `json:"sibling"`
	IsLeft  bool   `json:"is_left"`
}

// MerkleTree stores all tree levels for proof construction.
type MerkleTree struct {
	levels [][]*Node
}

// Common merkle errors.
var (
	ErrEmptyData    = errors.New("merkle: no data provided")
	ErrNilTree      = errors.New("merkle: tree is empty")
	ErrInvalidIndex = errors.New("merkle: invalid leaf index")
	ErrInvalidProof = errors.New("merkle: invalid proof")
)

type jsonInput struct {
	Items []string `json:"items"`
}

// NewMerkleTreeFromJSON builds a tree from JSON: {"items":[...]}.
func NewMerkleTreeFromJSON(path string) (*MerkleTree, error) {
	content, err := os.ReadFile(path)
	if err != nil {
		return nil, err
	}

	var input jsonInput
	if err := json.Unmarshal(content, &input); err != nil {
		return nil, err
	}

	if len(input.Items) == 0 {
		return nil, ErrEmptyData
	}

	data := make([][]byte, 0, len(input.Items))
	for _, item := range input.Items {
		data = append(data, []byte(item))
	}

	tree := NewMerkleTree(data)
	if tree == nil {
		return nil, ErrEmptyData
	}

	return tree, nil
}

// NewMerkleTree builds a Merkle tree from raw items.
// Returns nil for empty input.
func NewMerkleTree(data [][]byte) *MerkleTree {
	if len(data) == 0 {
		return nil
	}

	level := make([]*Node, 0, len(data))
	for _, item := range data {
		itemCopy := make([]byte, len(item))
		copy(itemCopy, item)
		level = append(level, &Node{
			Hash: hashBytes(itemCopy),
			Data: itemCopy,
		})
	}

	levels := make([][]*Node, 0, 1+len(data)/2)
	levels = append(levels, level)

	for len(level) > 1 {
		next := make([]*Node, 0, (len(level)+1)/2)
		for i := 0; i < len(level); i += 2 {
			left := level[i]
			right := left
			if i+1 < len(level) {
				right = level[i+1]
			}

			parentHash := hashPair(left.Hash, right.Hash)
			next = append(next, &Node{
				Hash:  parentHash,
				Left:  left,
				Right: right,
			})
		}
		levels = append(levels, next)
		level = next
	}

	return &MerkleTree{levels: levels}
}

// Root returns a copy of the Merkle root hash.
func (t *MerkleTree) Root() []byte {
	if t == nil || len(t.levels) == 0 {
		return nil
	}
	rootLevel := t.levels[len(t.levels)-1]
	if len(rootLevel) == 0 {
		return nil
	}
	return copyBytes(rootLevel[0].Hash)
}

// GetProof builds Merkle proof for leaf at index.
func (t *MerkleTree) GetProof(index int) ([]ProofStep, error) {
	if t == nil || len(t.levels) == 0 {
		return nil, ErrNilTree
	}

	if index < 0 || index >= len(t.levels[0]) {
		return nil, ErrInvalidIndex
	}

	if len(t.levels) == 1 {
		return []ProofStep{}, nil
	}

	current := index
	proof := make([]ProofStep, 0, len(t.levels)-1)

	for level := 0; level < len(t.levels)-1; level++ {
		nodes := t.levels[level]
		var siblingIndex int
		isLeft := false

		if current%2 == 0 {
			siblingIndex = current + 1
			if siblingIndex >= len(nodes) {
				siblingIndex = current
			}
			isLeft = false
		} else {
			siblingIndex = current - 1
			isLeft = true
		}

		proof = append(proof, ProofStep{
			SiblingHash: copyBytes(nodes[siblingIndex].Hash),
			IsLeft:      isLeft,
		})
		current /= 2
	}

	return proof, nil
}

// Verify checks that data is included in a tree with given root using proof.
func (t *MerkleTree) Verify(data []byte, proof []ProofStep, root []byte) bool {
	return VerifyProof(data, proof, root)
}

// VerifyProof checks that data is included in a tree with a given root using proof.
func VerifyProof(data []byte, proof []ProofStep, root []byte) bool {
	if len(root) != hashSize {
		return false
	}
	current := hashBytes(data)
	for _, step := range proof {
		if len(step.SiblingHash) != hashSize {
			return false
		}
		if step.IsLeft {
			current = hashPair(step.SiblingHash, current)
		} else {
			current = hashPair(current, step.SiblingHash)
		}
	}
	return bytesEqual(current, root)
}

// UpdateLeaf updates one leaf and rebuilds all parent hashes on the path to root.
func (t *MerkleTree) UpdateLeaf(index int, newData []byte) error {
	if t == nil || len(t.levels) == 0 {
		return ErrNilTree
	}
	if index < 0 || index >= len(t.levels[0]) {
		return ErrInvalidIndex
	}

	dataCopy := make([]byte, len(newData))
	copy(dataCopy, newData)

	t.levels[0][index].Data = dataCopy
	t.levels[0][index].Hash = hashBytes(dataCopy)

	current := index
	for level := 0; level < len(t.levels)-1; level++ {
		parentIndex := current / 2
		leftChildIndex := parentIndex * 2
		rightChildIndex := leftChildIndex + 1

		left := t.levels[level][leftChildIndex]
		right := left
		if rightChildIndex < len(t.levels[level]) {
			right = t.levels[level][rightChildIndex]
		}

		t.levels[level+1][parentIndex].Hash = hashPair(left.Hash, right.Hash)
		current = parentIndex
	}

	return nil
}

// ParseProofJSON parses proof from JSON representation.
func ParseProofJSON(data []byte) ([]ProofStep, error) {
	list, err := parseProofJSONList(data)
	if err == nil {
		return parseProofStepList(list)
	}

	var wrapped struct {
		Proof []ProofStepJSON `json:"proof"`
	}
	if err := json.Unmarshal(data, &wrapped); err != nil {
		return nil, err
	}
	if wrapped.Proof == nil {
		return nil, fmt.Errorf("%w: proof field is missing", ErrInvalidProof)
	}

	return parseProofStepList(wrapped.Proof)
}

func parseProofJSONList(data []byte) ([]ProofStepJSON, error) {
	var raw []ProofStepJSON
	if err := json.Unmarshal(data, &raw); err != nil {
		return nil, err
	}
	return raw, nil
}

func parseProofStepList(raw []ProofStepJSON) ([]ProofStep, error) {
	proof := make([]ProofStep, 0, len(raw))
	for i, step := range raw {
		sibling, err := hex.DecodeString(step.Sibling)
		if err != nil {
			return nil, err
		}
		if len(sibling) != hashSize {
			return nil, fmt.Errorf("%w: sibling hash at position %d has length %d", ErrInvalidProof, i, len(sibling))
		}
		proof = append(proof, ProofStep{
			SiblingHash: sibling,
			IsLeft:      step.IsLeft,
		})
	}

	return proof, nil
}

// MarshalProofJSON serializes proof to JSON representation.
func MarshalProofJSON(proof []ProofStep) ([]byte, error) {
	raw := make([]ProofStepJSON, 0, len(proof))
	for i, step := range proof {
		if len(step.SiblingHash) != hashSize {
			return nil, fmt.Errorf("%w: sibling hash at position %d has length %d", ErrInvalidProof, i, len(step.SiblingHash))
		}
		raw = append(raw, ProofStepJSON{
			Sibling: Hex(step.SiblingHash),
			IsLeft:  step.IsLeft,
		})
	}

	return json.Marshal(raw)
}

func hashBytes(data []byte) []byte {
	sum := sha256.Sum256(data)
	return sum[:]
}

func hashPair(left, right []byte) []byte {
	joined := make([]byte, 0, len(left)+len(right))
	joined = append(joined, left...)
	joined = append(joined, right...)
	sum := sha256.Sum256(joined)
	return sum[:]
}

func copyBytes(input []byte) []byte {
	output := make([]byte, len(input))
	copy(output, input)
	return output
}

// Hex returns hash as hex string for tests and debug.
func Hex(data []byte) string {
	return hex.EncodeToString(data)
}

func bytesEqual(a, b []byte) bool {
	if len(a) != len(b) {
		return false
	}
	for i := range a {
		if a[i] != b[i] {
			return false
		}
	}
	return true
}
