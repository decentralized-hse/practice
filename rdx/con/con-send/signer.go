package sender

import (
	"crypto/ecdsa"
	"crypto/rand"
	"crypto/sha256"
	"encoding/asn1"
	"encoding/json"
	"log"
	"math/big"
	"net/http"
	"os"
)

type Transaction struct {
	From   string
	To     string
	Amount int
	Key    *ecdsa.PrivateKey
}

type txData struct {
	From   string `json:"from"`
	To     string `json:"to"`
	Amount int    `json:"amount"`
}

type signedTransaction struct {
	Data      txData `json:"data"`
	Signature []byte `json:"signature"`
}

func SignAndSend(tx Transaction) {

	data := txData{
		From:   tx.From,
		To:     tx.To,
		Amount: tx.Amount,
	}

	dataBytes, err := json.Marshal(data)
	if err != nil {
		log.Fatalf("Error marshaling transaction data: %v", err)
	}

	hash := sha256.Sum256(dataBytes)

	r, s, err := ecdsa.Sign(rand.Reader, tx.Key, hash[:])
	if err != nil {
		log.Fatalf("Error signing transaction: %v", err)
	}

	sig, err := asn1.Marshal(struct {
		R *big.Int
		S *big.Int
	}{r, s})
	if err != nil {
		log.Fatalf("Error encoding signature: %v", err)
	}

	signedTx := signedTransaction{
		Data:      data,
		Signature: sig,
	}

	output, err := json.Marshal(signedTx)
	if err != nil {
		log.Fatalf("Error marshaling signed transaction: %v", err)
	}

	os.Stdout.Write(output)
}

func transactionHandler(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Invalid request method", http.StatusMethodNotAllowed)
		return
	}

	var tx Transaction
	err := json.NewDecoder(r.Body).Decode(&tx)
	if err != nil {
		http.Error(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	if tx.Key == nil {
		http.Error(w, "Missing private key in transaction", http.StatusBadRequest)
		return
	}

	SignAndSend(tx)
	w.WriteHeader(http.StatusOK)
	w.Write([]byte("Transaction signed and sent successfully"))
}

// func main() {
// 	privKey, err := ecdsa.GenerateKey(elliptic.P256(), rand.Reader)
// 	if err != nil {
// 		log.Fatal(err)
// 	}

// 	tx := Transaction{
// 		From:   "Alice",
// 		To:     "Bob",
// 		Key:    privKey,
// 		Amount: 100,
// 	}

// 	SignAndSend(tx)
// }
