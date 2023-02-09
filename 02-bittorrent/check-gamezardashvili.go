package main

import (
	"crypto/ed25519"
	"encoding/hex"
	"fmt"
	"io/ioutil"
	"os"
)

func readHexFile(filename string) ([]byte, error) {
	file, err := os.Open(filename)
	if err != nil {
		return []byte{}, fmt.Errorf("Error opening file %s", filename)
	}
	defer file.Close()
	hexEncoder := hex.NewDecoder(file)
	result, err := ioutil.ReadAll(hexEncoder)
	if err != nil {
		fmt.Println(err)
		return []byte{}, fmt.Errorf("Error reading from file %s, please ensure it is in hex format", filename)
	}
	return result, nil
}

func main() {
    fn := os.Args[1]
	pubKeyFileName := fn + ".pub"

	publicKeyB, err := readHexFile(pubKeyFileName)
	if err != nil {
		fmt.Println("Error importing public key: ", err)
		os.Exit(1)
	}

	publicKey := ed25519.PublicKey(publicKeyB)

	if len(publicKey) != ed25519.PublicKeySize {
		fmt.Println("Wrong public key size. Expected ", ed25519.PublicKeySize, " but got ", len(publicKey))
		os.Exit(1)
	}

	messageFileName := fn + ".root"

	message, err := ioutil.ReadFile(messageFileName)
	if err != nil {
		fmt.Println("Error importing message file: ", err)
		os.Exit(1)
	}

	sigFileName := fn + ".sign"

	sig, err := readHexFile(sigFileName)
	if err != nil {
		fmt.Println("Error importing signature file: ", err)
		os.Exit(1)
	}

	if ed25519.Verify(publicKey, message, sig) {
		fmt.Println("Valid signature!!!")
	} else {
		fmt.Println("Signature for provided message and public key is not valid.")
		os.Exit(1)
	}

}
