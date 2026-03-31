package main

import (
	"bytes"
	"encoding/json"
	"os"
	"path/filepath"
	"strings"
	"testing"

	"merkle-hashing/internal/merkle"
)

type proofResponse struct {
	Index int                    `json:"index"`
	Root  string                 `json:"root"`
	Proof []merkle.ProofStepJSON `json:"proof"`
}

func runCommand(args ...string) (string, string, int) {
	var out bytes.Buffer
	var errOut bytes.Buffer
	exitCode := run(args, &out, &errOut)
	return out.String(), errOut.String(), exitCode
}

func writeJSONData(t *testing.T) (string, string) {
	t.Helper()

	tempDir := t.TempDir()
	path := filepath.Join(tempDir, "data.json")

	content := []byte(`{"items":["a","b","c","d"]}`)
	if err := os.WriteFile(path, content, 0o600); err != nil {
		t.Fatalf("write file failed: %v", err)
	}

	tree := merkle.NewMerkleTree([][]byte{
		[]byte("a"),
		[]byte("b"),
		[]byte("c"),
		[]byte("d"),
	})
	return path, merkle.Hex(tree.Root())
}

func TestCLIBuild(t *testing.T) {
	t.Parallel()

	path, expectedRoot := writeJSONData(t)
	stdout, stderr, code := runCommand("build", "-json", path)
	if code != 0 {
		t.Fatalf("expected exit code 0, got %d, err=%q", code, stderr)
	}
	if got := strings.TrimSpace(stdout); got != expectedRoot {
		t.Fatalf("unexpected root: got %s, expected %s", got, expectedRoot)
	}
}

func TestCLIProof(t *testing.T) {
	t.Parallel()

	path, expectedRoot := writeJSONData(t)
	stdout, stderr, code := runCommand("proof", "-json", path, "-index", "1")
	if code != 0 {
		t.Fatalf("expected exit code 0, got %d, err=%q", code, stderr)
	}

	var response proofResponse
	if err := json.Unmarshal([]byte(stdout), &response); err != nil {
		t.Fatalf("proof output invalid json: %v", err)
	}

	if response.Index != 1 {
		t.Fatalf("expected index 1, got %d", response.Index)
	}
	if response.Root != expectedRoot {
		t.Fatalf("expected proof root %s, got %s", expectedRoot, response.Root)
	}
	if len(response.Proof) == 0 {
		t.Fatal("proof must not be empty for non-leaf-tree")
	}
}

func TestCLIVerifyAndVerifyProof(t *testing.T) {
	t.Parallel()

	path, expectedRoot := writeJSONData(t)
	proofOut, proofErr, proofCode := runCommand("proof", "-json", path, "-index", "1")
	if proofCode != 0 {
		t.Fatalf("proof command failed: code=%d err=%q", proofCode, proofErr)
	}

	out := t.TempDir()
	proofPath := filepath.Join(out, "proof.json")
	if err := os.WriteFile(proofPath, []byte(proofOut), 0o600); err != nil {
		t.Fatalf("write proof file failed: %v", err)
	}

	stdout, _, code := runCommand("verify", "-json", path, "-index", "1", "-data", "b")
	if code != 0 {
		t.Fatal("verify command failed")
	}
	if strings.TrimSpace(stdout) != "true" {
		t.Fatalf("expected verify to return true, got %s", stdout)
	}

	stdout, _, code = runCommand("verify", "-json", path, "-index", "1", "-data", "wrong")
	if code != 0 {
		t.Fatal("verify command failed")
	}
	if strings.TrimSpace(stdout) != "false" {
		t.Fatalf("expected verify false for wrong data, got %s", stdout)
	}

	stdout, _, code = runCommand("verify-proof", "-root", expectedRoot, "-proof", proofPath, "-data", "b")
	if code != 0 {
		t.Fatal("verify-proof command failed")
	}
	if strings.TrimSpace(stdout) != "true" {
		t.Fatalf("expected verify-proof true, got %s", stdout)
	}
}

func TestCLIUpdate(t *testing.T) {
	t.Parallel()

	path, oldRoot := writeJSONData(t)
	out, errOut, code := runCommand("update", "-json", path, "-index", "1", "-data", "z")
	if code != 0 {
		t.Fatalf("expected exit code 0, got %d err=%q", code, errOut)
	}

	newRoot := strings.TrimSpace(out)
	if newRoot == oldRoot {
		t.Fatal("root should change after update")
	}
}

func TestCLIMissingCommand(t *testing.T) {
	t.Parallel()

	_, _, code := runCommand("missing-command")
	if code != 1 {
		t.Fatalf("expected usage path to have exit code 1, got %d", code)
	}
}
