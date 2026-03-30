package main

import (
	"encoding/hex"
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"os"

	"merkle-hashing/internal/merkle"
)

func main() {
	os.Exit(run(os.Args[1:], os.Stdout, os.Stderr))
}

func run(args []string, out, errOut io.Writer) int {
	if len(args) < 1 {
		usage(out)
		return 1
	}

	switch args[0] {
	case "build":
		if err := runBuild(args[1:], out, errOut); err != nil {
			fmt.Fprintln(errOut, err)
			return 1
		}
	case "proof":
		if err := runProof(args[1:], out, errOut); err != nil {
			fmt.Fprintln(errOut, err)
			return 1
		}
	case "verify":
		if err := runVerify(args[1:], out, errOut); err != nil {
			fmt.Fprintln(errOut, err)
			return 1
		}
	case "update":
		if err := runUpdate(args[1:], out, errOut); err != nil {
			fmt.Fprintln(errOut, err)
			return 1
		}
	case "verify-proof":
		if err := runVerifyProof(args[1:], out, errOut); err != nil {
			fmt.Fprintln(errOut, err)
			return 1
		}
	case "help":
		usage(out)
		return 0
	default:
		usage(out)
		return 1
	}
	return 0
}

func runBuild(args []string, out, errOut io.Writer) error {
	_ = errOut
	fs := flag.NewFlagSet("build", flag.ContinueOnError)
	jsonPath := fs.String("json", "", "Path to JSON file with items")
	if err := fs.Parse(args); err != nil {
		return fmt.Errorf("build args error: %w", err)
	}

	if *jsonPath == "" {
		return fmt.Errorf("build: missing required flag -json")
	}

	tree, err := merkle.NewMerkleTreeFromJSON(*jsonPath)
	if err != nil {
		return fmt.Errorf("build: %w", err)
	}

	_, err = fmt.Fprintln(out, merkle.Hex(tree.Root()))
	return err
}

func runProof(args []string, out, errOut io.Writer) error {
	_ = errOut
	fs := flag.NewFlagSet("proof", flag.ContinueOnError)
	jsonPath := fs.String("json", "", "Path to JSON file with items")
	index := fs.Int("index", -1, "Leaf index to build proof for")
	if err := fs.Parse(args); err != nil {
		return fmt.Errorf("proof args error: %w", err)
	}

	if *jsonPath == "" {
		return fmt.Errorf("proof: missing required flag -json")
	}
	if *index < 0 {
		return fmt.Errorf("proof: missing or invalid -index")
	}

	tree, err := merkle.NewMerkleTreeFromJSON(*jsonPath)
	if err != nil {
		return fmt.Errorf("proof: %w", err)
	}

	proof, err := tree.GetProof(*index)
	if err != nil {
		return fmt.Errorf("proof: %w", err)
	}

	response := make([]merkle.ProofStepJSON, 0, len(proof))
	for _, step := range proof {
		response = append(response, merkle.ProofStepJSON{
			Sibling: merkle.Hex(step.SiblingHash),
			IsLeft:  step.IsLeft,
		})
	}

	output := struct {
		Index int                    `json:"index"`
		Root  string                 `json:"root"`
		Proof []merkle.ProofStepJSON `json:"proof"`
	}{
		Index: *index,
		Root:  merkle.Hex(tree.Root()),
		Proof: response,
	}

	body, err := json.MarshalIndent(output, "", "  ")
	if err != nil {
		return fmt.Errorf("proof: %w", err)
	}

	_, err = fmt.Fprintln(out, string(body))
	return err
}

func runVerify(args []string, out, errOut io.Writer) error {
	_ = errOut
	fs := flag.NewFlagSet("verify", flag.ContinueOnError)
	jsonPath := fs.String("json", "", "Path to JSON file with items")
	index := fs.Int("index", -1, "Leaf index to verify")
	data := fs.String("data", "", "Expected leaf data")
	if err := fs.Parse(args); err != nil {
		return fmt.Errorf("verify args error: %w", err)
	}

	if *jsonPath == "" {
		return fmt.Errorf("verify: missing required flag -json")
	}
	if *index < 0 {
		return fmt.Errorf("verify: missing or invalid -index")
	}

	tree, err := merkle.NewMerkleTreeFromJSON(*jsonPath)
	if err != nil {
		return fmt.Errorf("verify: %w", err)
	}

	proof, err := tree.GetProof(*index)
	if err != nil {
		return fmt.Errorf("verify: %w", err)
	}

	ok := tree.Verify([]byte(*data), proof, tree.Root())
	_, err = fmt.Fprintln(out, ok)
	return err
}

func runVerifyProof(args []string, out, errOut io.Writer) error {
	_ = errOut
	fs := flag.NewFlagSet("verify-proof", flag.ContinueOnError)
	rootHex := fs.String("root", "", "Expected root hash in hex")
	data := fs.String("data", "", "Expected leaf data")
	proofPath := fs.String("proof", "", "Path to proof JSON file")
	if err := fs.Parse(args); err != nil {
		return fmt.Errorf("verify-proof args error: %w", err)
	}

	if *rootHex == "" {
		return fmt.Errorf("verify-proof: missing required flag -root")
	}
	if *proofPath == "" {
		return fmt.Errorf("verify-proof: missing required flag -proof")
	}

	root, err := hex.DecodeString(*rootHex)
	if err != nil {
		return fmt.Errorf("verify-proof: invalid root hash")
	}

	proofData, err := os.ReadFile(*proofPath)
	if err != nil {
		return fmt.Errorf("verify-proof: %w", err)
	}

	proof, err := merkle.ParseProofJSON(proofData)
	if err != nil {
		return fmt.Errorf("verify-proof: %w", err)
	}

	ok := merkle.VerifyProof([]byte(*data), proof, root)
	_, err = fmt.Fprintln(out, ok)
	return err
}

func runUpdate(args []string, out, errOut io.Writer) error {
	_ = errOut
	fs := flag.NewFlagSet("update", flag.ContinueOnError)
	jsonPath := fs.String("json", "", "Path to JSON file with items")
	index := fs.Int("index", -1, "Leaf index to update")
	data := fs.String("data", "", "New leaf data")
	if err := fs.Parse(args); err != nil {
		return fmt.Errorf("update args error: %w", err)
	}

	if *jsonPath == "" {
		return fmt.Errorf("update: missing required flag -json")
	}
	if *index < 0 {
		return fmt.Errorf("update: missing or invalid -index")
	}

	tree, err := merkle.NewMerkleTreeFromJSON(*jsonPath)
	if err != nil {
		return fmt.Errorf("update: %w", err)
	}

	if err := tree.UpdateLeaf(*index, []byte(*data)); err != nil {
		return fmt.Errorf("update: %w", err)
	}

	_, err = fmt.Fprintln(out, merkle.Hex(tree.Root()))
	return err
}

func usage(out io.Writer) {
	fmt.Fprintln(out, `Usage:
  merkle build -json <file>
  merkle proof -json <file> -index <n>
  merkle verify -json <file> -index <n> -data <value>
  merkle verify-proof -root <hex> -proof <file> -data <value>
  merkle update -json <file> -index <n> -data <value>`)
}
