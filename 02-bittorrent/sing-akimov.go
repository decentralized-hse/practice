package main

import (
	"crypto/ed25519"
	"fmt"
	"os"
)

func main() {
	var str_hash string
	fmt.Fscan(os.Stdin, &str_hash)

	// tree hash
	hash := []byte(str_hash)

	// generate pub and prv key
	publ, priv, _ := ed25519.GenerateKey(nil)

	// sign
	sig := ed25519.Sign(priv, hash[:])

	fmt.Printf("Public key = \t%x\n\n", publ)
	fmt.Printf("Private key = \t%x\n\n", priv)
	fmt.Printf("Signature = \t%x\n\n", sig)
}
