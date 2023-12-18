package router

import (
	"encoding/binary"
	"fmt"
	"github.com/jamesruan/sodium"
	"testing"
	"time"
)

func TestNode_Announce(t *testing.T) {
	keypair := sodium.MakeBoxKP()
	now := time.Now()
	nano := now.UnixNano()
	nano -= nano % (60 * 60 * 1000000000) // round to an hour
	var timestr [8]byte
	binary.LittleEndian.PutUint64(timestr[:], uint64(nano))
	str := append(keypair.PublicKey.Bytes, timestr[0:8]...)
	hash0 := SHA256Sum(str)
	hash1 := hash0.Next()

	ann0, _ := TLVAppend(nil, 'A', hash0[:])
	ann1, _ := TLVAppend(nil, 'A', hash1[:])

	text := "Hello world!"
	msg1, _ := TLVAppend2(nil, 'M', hash1[:], []byte(text))

	fmt.Printf("Current time: %s\n", now.String())
	fmt.Printf("Current time (Unix): %d\n", now.Unix())
	fmt.Printf("Current time (Unix nano): %d\n", now.UnixNano())
	fmt.Printf("Current time (start of the hour): %d\n", nano)
	fmt.Printf("Public key: %s\n", hexize(keypair.PublicKey.Bytes))
	fmt.Printf("Hash A(0): %s\n", hash0.String())
	fmt.Printf("Hash A(1): %s\n", hash1.String())
	fmt.Printf("Hash A(2): %s\n", hash1.Next().String())
	fmt.Printf("Announce A(0) ToyTLV: %s\n", hexize(ann0))
	fmt.Printf("Announce A(1) ToyTLV: %s\n", hexize(ann1))
	fmt.Printf("Message text: %s\n", text)
	fmt.Printf("Message relayed for A(1): %s\n", hexize(msg1))
}
