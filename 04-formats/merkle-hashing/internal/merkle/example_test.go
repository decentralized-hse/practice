package merkle

import (
	"fmt"
)

func ExampleMerkleTree() {
	tree := NewMerkleTree([][]byte{
		[]byte("a"),
		[]byte("b"),
		[]byte("c"),
	})

	root := tree.Root()
	proof, err := tree.GetProof(1)
	if err != nil {
		fmt.Println("proof error:", err)
		return
	}

	ok := tree.Verify([]byte("b"), proof, root)
	fmt.Println("verify:", ok)
	fmt.Println("root length:", len(root))

	// Output:
	// verify: true
	// root length: 32
}
