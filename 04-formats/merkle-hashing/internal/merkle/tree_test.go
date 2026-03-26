package merkle

import (
	"bytes"
	"os"
	"path/filepath"
	"testing"
)

func TestMerkleTreeRootOdd(t *testing.T) {
	t.Parallel()

	data := [][]byte{
		[]byte("block-0"),
		[]byte("block-1"),
		[]byte("block-2"),
	}

	tree := NewMerkleTree(data)
	if tree == nil {
		t.Fatal("expected non-nil tree")
	}

	h0 := hashBytes(data[0])
	h1 := hashBytes(data[1])
	h2 := hashBytes(data[2])

	ab := hashPair(h0, h1)
	cc := hashPair(h2, h2)
	expected := hashPair(ab, cc)

	if !bytes.Equal(tree.Root(), expected) {
		t.Fatalf("unexpected root: got %s, expected %s", Hex(tree.Root()), Hex(expected))
	}
}

func TestMerkleTreeProofAndVerify(t *testing.T) {
	t.Parallel()

	data := [][]byte{
		[]byte("a"),
		[]byte("b"),
		[]byte("c"),
		[]byte("d"),
	}
	tree := NewMerkleTree(data)
	if tree == nil {
		t.Fatal("expected non-nil tree")
	}

	proof, err := tree.GetProof(2)
	if err != nil {
		t.Fatalf("get proof failed: %v", err)
	}

	root := tree.Root()
	if !tree.Verify(data[2], proof, root) {
		t.Fatalf("proof is invalid for correct data")
	}
	if tree.Verify([]byte("changed"), proof, root) {
		t.Fatalf("proof should be invalid for changed data")
	}
}

func TestMerkleTreeRootSingleAndPair(t *testing.T) {
	t.Parallel()

	single := NewMerkleTree([][]byte{[]byte("single")})
	if single == nil {
		t.Fatal("expected non-nil tree")
	}
	expectedSingle := hashBytes([]byte("single"))
	if !bytes.Equal(single.Root(), expectedSingle) {
		t.Fatalf("unexpected root for single item: got %s, expected %s", Hex(single.Root()), Hex(expectedSingle))
	}

	pair := NewMerkleTree([][]byte{[]byte("left"), []byte("right")})
	if pair == nil {
		t.Fatal("expected non-nil tree")
	}
	hLeft := hashBytes([]byte("left"))
	hRight := hashBytes([]byte("right"))
	expectedPair := hashPair(hLeft, hRight)
	if !bytes.Equal(pair.Root(), expectedPair) {
		t.Fatalf("unexpected root for pair: got %s, expected %s", Hex(pair.Root()), Hex(expectedPair))
	}
}

func TestMerkleTreeDeterministicRoot(t *testing.T) {
	t.Parallel()

	data := [][]byte{
		[]byte("x"),
		[]byte("y"),
		[]byte("z"),
	}

	r1 := NewMerkleTree(data).Root()
	r2 := NewMerkleTree(data).Root()
	if !bytes.Equal(r1, r2) {
		t.Fatalf("expected deterministic root for the same input")
	}
}

func TestProofJSONRoundTrip(t *testing.T) {
	t.Parallel()

	original := []ProofStep{
		{SiblingHash: hashBytes([]byte("first")), IsLeft: true},
		{SiblingHash: hashBytes([]byte("second")), IsLeft: false},
	}

	encoded, err := MarshalProofJSON(original)
	if err != nil {
		t.Fatalf("marshal failed: %v", err)
	}

	decoded, err := ParseProofJSON(encoded)
	if err != nil {
		t.Fatalf("parse failed: %v", err)
	}

	if len(decoded) != len(original) {
		t.Fatalf("proof size mismatch: got %d, expected %d", len(decoded), len(original))
	}

	for i := range original {
		if original[i].IsLeft != decoded[i].IsLeft {
			t.Fatalf("isLeft mismatch at index %d", i)
		}
		if !bytes.Equal(original[i].SiblingHash, decoded[i].SiblingHash) {
			t.Fatalf("sibling mismatch at index %d", i)
		}
	}
}

func TestParseProofJSONInvalid(t *testing.T) {
	t.Parallel()

	if _, err := ParseProofJSON([]byte(`[{"sibling":"not-hex","is_left":true}]`)); err == nil {
		t.Fatalf("expected parse error for invalid hex")
	}

	if _, err := ParseProofJSON([]byte(`[{"sibling":"AA","is_left":true}]`)); err == nil {
		t.Fatalf("expected parse error for wrong hash size")
	}

	wrapped := []byte(`{"index":1,"root":"abcdef","proof":[{"sibling":"` + Hex(hashBytes([]byte("first"))) + `","is_left":true}]}`)
	parsed, err := ParseProofJSON(wrapped)
	if err != nil {
		t.Fatalf("wrapped parse failed: %v", err)
	}
	if len(parsed) != 1 {
		t.Fatalf("expected wrapped proof size 1")
	}
	if !parsed[0].IsLeft {
		t.Fatalf("expected is_left=true")
	}

	if _, err := ParseProofJSON([]byte(`{"index":1,"root":"abcdef","proof":[]}`)); err != nil {
		t.Fatalf("expected empty proof array to be valid")
	}
}

func TestMerkleTreeUpdateLeaf(t *testing.T) {
	t.Parallel()

	data := [][]byte{
		[]byte("a"),
		[]byte("b"),
		[]byte("c"),
	}
	tree := NewMerkleTree(data)
	if tree == nil {
		t.Fatal("expected non-nil tree")
	}
	oldRoot := copyBytes(tree.Root())

	if err := tree.UpdateLeaf(1, []byte("new-block")); err != nil {
		t.Fatalf("update leaf failed: %v", err)
	}

	newRoot := tree.Root()
	if bytes.Equal(oldRoot, newRoot) {
		t.Fatalf("root must change after leaf update")
	}

	proof, err := tree.GetProof(1)
	if err != nil {
		t.Fatalf("get proof failed: %v", err)
	}

	if !tree.Verify([]byte("new-block"), proof, newRoot) {
		t.Fatalf("proof must be valid for updated leaf")
	}
	if tree.Verify([]byte("b"), proof, newRoot) {
		t.Fatalf("proof must be invalid for outdated leaf data")
	}
}

func TestMerkleTreeJSONLoading(t *testing.T) {
	tmpDir := t.TempDir()
	jsonPath := filepath.Join(tmpDir, "data.json")

	content := []byte(`{"items":["one","two","three"]}`)
	if err := os.WriteFile(jsonPath, content, 0o600); err != nil {
		t.Fatalf("write file failed: %v", err)
	}

	tree, err := NewMerkleTreeFromJSON(jsonPath)
	if err != nil {
		t.Fatalf("create tree from json failed: %v", err)
	}
	if tree == nil || len(tree.Root()) == 0 {
		t.Fatalf("tree root should exist")
	}
}

func TestMerkleTreeErrors(t *testing.T) {
	t.Parallel()

	if _, err := NewMerkleTreeFromJSON("non_existing_file.json"); err == nil {
		t.Fatalf("expected error for missing file")
	}

	if tree := NewMerkleTree(nil); tree != nil {
		t.Fatalf("expected nil tree for empty input")
	}

	if _, err := NewMerkleTreeFromJSON(filepath.Join(t.TempDir(), "empty.json")); err == nil {
		t.Fatalf("expected empty-json parsing error")
	}

	tmpPath := filepath.Join(t.TempDir(), "empty-items.json")
	if err := os.WriteFile(tmpPath, []byte(`{"items":[]}`), 0o600); err != nil {
		t.Fatalf("write file failed: %v", err)
	}
	tree, err := NewMerkleTreeFromJSON(tmpPath)
	if err != ErrEmptyData {
		t.Fatalf("expected ErrEmptyData, got %v", err)
	}
	if tree != nil {
		t.Fatalf("tree must be nil for empty items")
	}
}
