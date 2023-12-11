package router

import (
	"crypto"
	"crypto/sha256"
	"encoding/binary"
	"encoding/hex"
	"errors"
	"fmt"
	"github.com/cockroachdb/pebble"
	"github.com/jamesruan/sodium"
	"net"
	"os"
	"sync"
	"time"
)

const MAXD = 5

type pd uint32

type SHA256 [32]byte

// Node database scheme
// private key 		P nick: 	privhex
// announce         A hash: 	time, uri
// connections 		C uri:      -
// public keys 		K nick: 	pub (hex)
// message history	M nick: 	time, string
// ignored announce E hash uri: time
// next-hash		N hash:		prev hash hex
// traffix stats    S ndx32 time: packets, bytes, announces
// listen addresses L addr: 	-
type Node struct {
	pubkey crypto.PublicKey
	inbox  [][]byte
	peers  map[string]*Peer
	listen map[string]net.Listener
	boxmx  sync.Mutex
	DB     *pebble.DB
}

type Message struct {
	to   SHA256
	body []byte
}

// database

var _litKey = [64]byte{'K'}
var litKey = _litKey[0:1]

var _litPrivate = [64]byte{'P'}
var litPrivate = _litPrivate[0:1]

var _litListen = [64]byte{'L'}
var litListen = _litListen[0:1]

var _litNext = [64]byte{'N'}
var litNext = _litNext[0:1]

var _litAnnounce = [64]byte{'A'}
var litAnnounce = _litAnnounce[0:1]

func Hex(data []byte) string {
	ret := make([]byte, len(data)*2)
	hex.Encode(ret, data)
	return string(ret)
}

func ValPD(ndx pd) []byte {
	var ret [4]byte
	binary.LittleEndian.PutUint32(ret[:], uint32(ndx))
	return ret[:]
}

func SHA256Hex(hash SHA256) []byte {
	ret := make([]byte, 64)
	hex.Encode(ret[:], hash[:])
	return ret
}

var ErrMalformedRecord = errors.New("malformed record")

func SHA256Bin(hexhash []byte) (ret SHA256, err error) {
	i, err := hex.Decode(ret[:], hexhash)
	if i != len(SHA256{}) {
		err = ErrMalformedRecord
	}
	return
}

var HaveBetterPath = errors.New("have a shorter path")

func KeyHash(lit byte, sha256 SHA256) []byte {
	ret := make([]byte, 65)
	ret[0] = lit
	hex.Encode(ret[1:], sha256[:])
	return ret
}

func KeyString(lit byte, str string) []byte {
	ret := make([]byte, len(str)+1)
	ret[0] = lit
	copy(ret[1:], str)
	return ret
}

// announce cycle:
//  1. on connect, all announces are sent
//  2. own keys are announced by a job
//  3. announces that make difference get relayed
func (node *Node) Register(recvd SHA256, address string) error {
	_, cl, err := node.DB.Get(KeyHash('N', recvd))
	if err == nil {
		_ = cl.Close()
		return HaveBetterPath // a shorter path is known
	}
	wo := pebble.WriteOptions{}
	wb := pebble.Batch{}
	hash := recvd
	for i := 0; i < MAXD; i++ {
		next := sha256.Sum256(hash[:])
		_ = wb.Set(KeyHash('N', next), SHA256Hex(hash), &wo)
		hash = next
	}
	value := fmt.Sprintf("%d,%s", time.Now().UnixNano(), address)
	_ = wb.Set(KeyHash('A', recvd), []byte(value), &wo)
	err = node.DB.Apply(&wb, &wo)
	if err != nil {
		return err
	}
	next := sha256.Sum256(recvd[:])
	fmt.Printf("reg %s\r\nfwd %s\r\n", Hex(recvd[:]), Hex(next[:]))

	relay, _ := TLVAppend(nil, 'A', next[:])
	for _, p := range node.peers {
		if p == nil || p.address == address {
			continue
		}
		p.Queue(relay)
	}
	return nil
}

func (node *Node) Announce(kp sodium.BoxKP) error {
	hash0 := sha256.Sum256(kp.PublicKey.Bytes)
	return node.Register(hash0, "-")
}

var ErrAddressUnknown = errors.New("Address unknown")
var RouteUnknown = errors.New("RouteUnknown")

func (node *Node) findPrevHash(hash SHA256) (prev SHA256, err error) {
	recprev, cl1, err := node.DB.Get(KeyHash('N', hash))
	if err != nil {
		return
	}
	prev, err = SHA256Bin(recprev)
	_ = cl1.Close()
	return
}

func (node *Node) findFwdPeer(hash SHA256) (peer *Peer, err error) {
	key := KeyHash('A', hash)
	recndx, cl, err := node.DB.Get(key)
	if err != nil {
		return
	}
	var ts int64 = 0
	addr := ""
	n, err := fmt.Sscanf(string(recndx), "%d,%s", &ts, &addr)
	// TODO check too old
	if n != 2 {
		fmt.Fprintf(os.Stderr, "malformed record\r\n")
		err = ErrMalformedRecord
	} else if addr == "-" {
		fmt.Fprintf(os.Stderr, "message for me\r\n")
		peer = nil
	} else {
		peer = node.peers[addr]
	}
	_ = cl.Close()
	return
}

func (node *Node) Init() (err error) {
	node.peers = make(map[string]*Peer)
	node.listen = make(map[string]net.Listener)
	// TODO replace logger
	opts := pebble.Options{Logger: pebble.DefaultLogger}
	node.DB, err = pebble.Open("drastijk", &opts)
	io := pebble.IterOptions{}
	i, err := node.DB.NewIter(&io)
	if err != nil {
		return
	}
	for i.SeekGE([]byte{'L'}); i.Valid() && i.Key()[0] == 'L'; i.Next() {
		address := string(i.Key()[1:])
		go node.Listen(address)
	}
	for i.SeekGE([]byte{'C'}); i.Valid() && i.Key()[0] == 'C'; i.Next() {
		address := string(i.Key()[1:])
		go node.Connect(address, nil)
	}
	_ = i.Close()
	return
}

func (node *Node) Listen(addr string) (err error) {
	listener, err := net.Listen("tcp", addr)
	if err != nil {
		fmt.Fprintf(os.Stderr, "listen() fails: %s\r\n", err.Error())
		return
	}
	pre, ok := node.listen[addr]
	if ok {
		_ = pre.Close()
	}
	node.listen[addr] = listener
	for {
		con, err := listener.Accept()
		fmt.Fprintf(os.Stderr, "%s connected\r\n", con.RemoteAddr().String())
		if err != nil {
			fmt.Println(err.Error())
			break
		}
		node.Connect("", con)
	}
	delete(node.listen, addr)
	return
}

func (node *Node) RouteMessage(msg Message) error {
	prev, err := node.findPrevHash(msg.to)
	if err != nil {
		return err
	}
	peer, err := node.findFwdPeer(prev)
	if err != nil {
		return err
	}
	if peer == nil { // FIXME check have secret
		fmt.Printf("Received for %s\r\n%s\r\n", Hex(msg.to[:]), msg.body)
		return nil
	}
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
		peer, err := node.findFwdPeer(hash)
		if err == nil && peer != nil {
			msg, _ := TLVAppend2(nil, 'M', hash[:], []byte(txt))
			peer.Queue(msg)
			fmt.Fprintf(os.Stderr, "message sent to %s\r\n", hexize(key.Bytes))
			return nil
		}
		hash = sha256.Sum256(hash[:])
	}
	return ErrAddressUnknown
}

var ErrKeyIsAlreadyDefined = errors.New("the key is already defined")

func hexize(data []byte) []byte {
	h := make([]byte, len(data)*2)
	hex.Encode(h, data)
	return h
}

func unhexize(hexdata []byte) (bin []byte, err error) {
	bin = make([]byte, len(hexdata)/2)
	_, err = hex.Decode(bin, hexdata)
	return
}

// private key 		P nick: 	privhex
// public keys 		K nick: 	pub (hex)
func (node *Node) SaveKeys(name string, keypair sodium.BoxKP) error {
	_, _, err := node.DB.Get(append(litKey, name...))
	if err == nil {
		return ErrKeyIsAlreadyDefined
	}
	wb := pebble.Batch{}
	wo := pebble.WriteOptions{}
	_ = wb.Set(append(litKey, name...), hexize(keypair.PublicKey.Bytes), &wo)
	_ = wb.Set(append(litPrivate, name...), hexize(keypair.SecretKey.Bytes), &wo)
	return node.DB.Apply(&wb, &wo)
}

func (node *Node) LoadKeys(name string) (keypair sodium.BoxKP, err error) {
	pubhex, cl, err := node.DB.Get(KeyString('K', name))
	if err != nil {
		return
	}
	keypair.PublicKey.Bytes, err = unhexize(pubhex)
	_ = cl.Close()
	if err != nil {
		return
	}
	sechex, cl2, err := node.DB.Get(KeyString('P', name))
	if err != nil {
		err = nil
		return
	}
	keypair.SecretKey.Bytes, err = unhexize(sechex)
	_ = cl2.Close()
	return
}

// private key 		P nick: 	privhex
// public keys 		K nick: 	pub (hex)
func (node *Node) ShowKeys() {
	io := pebble.IterOptions{}
	i, err := node.DB.NewIter(&io)
	if err != nil {
		return
	}
	for i.SeekGE(litKey); i.Valid() && i.Key()[0] == litKey[0]; i.Next() {
		name := string(i.Key()[1:])
		key := i.Value()
		seckey := KeyString('P', name)
		_, cl, err := node.DB.Get(seckey)
		mine := ""
		if err == nil {
			mine = " (mine)"
			_ = cl.Close()
		}
		fmt.Printf("%s:\t%s%s\r\n", name, string(key), mine)
	}
	_ = i.Close()
}

func (node *Node) ShowListenAddresses() {
	io := pebble.IterOptions{}
	i, err := node.DB.NewIter(&io)
	if err != nil {
		return
	}
	i.SeekGE(litListen)
	for i.Valid() && i.Key()[0] == litListen[0] {
		address := string(i.Key()[1:])
		fmt.Printf("%s\r\n", address)
		i.Next()
	}
	_ = i.Close()
}

func (node *Node) ShowAll() {
	io := pebble.IterOptions{}
	i, err := node.DB.NewIter(&io)
	if err != nil {
		return
	}
	for i.SeekGE([]byte{0}); i.Valid(); i.Next() {
		fmt.Printf("%s:\t%s\r\n", i.Key(), i.Value())
	}
	_ = i.Close()
}
