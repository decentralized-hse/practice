package router

import (
	"crypto"
	"crypto/sha256"
	"encoding/hex"
	"errors"
	"fmt"
	"github.com/jamesruan/sodium"
	"net"
	"sync"
)

const MAXD = 5

type SHA256 [32]byte

type Node struct {
	pubkey crypto.PublicKey
	inbox  [][]byte
	peers  []*Peer
	boxmx  sync.Mutex
	prev   map[SHA256]SHA256
	routes map[SHA256]int
}

type Message struct {
	to   SHA256
	body []byte
}

type Peer struct {
	announces map[SHA256]uint64
	conn      net.Conn
	outbox    [][]byte
	boxmx     sync.Mutex
}

func Hex(data []byte) string {
	ret := make([]byte, len(data)*2)
	hex.Encode(ret, data)
	return string(ret)
}

func (node *Node) Register(recvd SHA256, ndx int) {
	_, ok := node.prev[recvd]
	if ok {
		return // a shorter path is known
	}
	hash := recvd
	for i := 0; i < MAXD; i++ {
		next := sha256.Sum256(hash[:])
		node.prev[next] = hash
		hash = next
	}
	node.routes[recvd] = ndx
	next := sha256.Sum256(recvd[:])
	fmt.Printf("reg %s\r\nfwd %s\r\n", Hex(recvd[:]), Hex(next[:]))

	relay, _ := TLVAppend(nil, 'A', next[:])
	for i := 0; i < len(node.peers); i++ {
		peer := node.peers[i]
		if i == ndx || peer == nil {
			continue
		}
		peer.Queue(relay)
	}
}

func (node *Node) Announce(kp sodium.BoxKP) {
	hash0 := sha256.Sum256(kp.PublicKey.Bytes)
	node.Register(hash0, -1)
}

var AddressUnknown = errors.New("Address unknown")
var RouteUnknown = errors.New("RouteUnknown")

func (node *Node) RouteMessage(msg Message) error {
	prev, ok := node.prev[msg.to]
	if !ok {
		return AddressUnknown
	}
	ndx, ok := node.routes[prev]
	if !ok {
		return RouteUnknown
	}
	if ndx == -1 {
		fmt.Printf("Received for %s\r\n%s\r\n", Hex(prev[:]), msg.body)
		return nil
	}
	peer := node.peers[ndx]
	if peer != nil {
		fwd, _ := TLVAppend2(nil, 'M', prev[:], msg.body)
		peer.boxmx.Lock()
		peer.outbox = append(peer.outbox, fwd)
		peer.boxmx.Unlock()
	}
	return nil
}

func (node *Node) Send(key sodium.BoxPublicKey, txt string) error {
	hash := sha256.Sum256(key.Bytes)
	for i := 0; i < MAXD; i++ {
		ndx, ok := node.routes[hash]
		if ok {
			peer := node.peers[ndx]
			if peer != nil {
				msg, _ := TLVAppend2(nil, 'M', hash[:], []byte(txt))
				peer.Queue(msg)
				return nil
			}
		}
		hash = sha256.Sum256(hash[:])
	}
	return AddressUnknown
}
