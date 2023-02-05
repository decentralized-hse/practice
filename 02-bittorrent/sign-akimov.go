package main

import (
	"crypto/ed25519"
	"fmt"
	"io"
	"os"
)

func main() {
	// input namefile
	name_file := os.Args[1]

	// input hash
	file, err := os.Open(name_file + ".root")
	if err != nil {
		fmt.Println(err)
		os.Exit(1)
	}

	data := make([]byte, 64)
	var str_hash string

	for {
		n, err1 := file.Read(data)
		if err1 == io.EOF {
			break
		}
		str_hash = str_hash + string(data[:n])
	}

	hash := []byte(str_hash)

	// generate pub and prv key
	publ, priv, _ := ed25519.GenerateKey(nil)

	// output keys
    pubfile, _ := os.Create(name_file + ".public_key")
    fmt.Fprintf(pubfile, "%x\n", publ);

    secfile, _ := os.Create(name_file + ".private_key")
    fmt.Fprintf(secfile, "%x\n", priv);

	// sign
	sign := ed25519.Sign(priv, hash[:])

	// output sign
    signfile, _ := os.Create(name_file + ".sign")
	defer file.Close()
    fmt.Fprintf(signfile, "%x\n", sign);
}
