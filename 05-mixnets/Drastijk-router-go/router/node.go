package router

import (
	"bytes"
	"crypto"
	"crypto/sha256"
	"encoding/binary"
	"encoding/hex"
	"errors"
	"fmt"
	"net"
	"os"
	"strconv"
	"sync"
	"time"

	"github.com/cockroachdb/pebble"
	"github.com/jamesruan/sodium"
	"github.com/learn-decentralized-systems/toykv"
)

const MAXD = 5

type pd uint32

type SHA256 [32]byte

func (sha SHA256) String() string {
	return string(hexize(sha[:]))
}

func (sha SHA256) Next() SHA256 {
	return sha256.Sum256(sha[:])
}

func SHA256Sum(data []byte) SHA256 {
	return sha256.Sum256(data)
}

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
	DB     toykv.KeyValueStore
}

type Message struct {
	to   SHA256
	body []byte
}

func Hex(data []byte) string {
	ret := make([]byte, len(data)*2)
	hex.Encode(ret, data)
	return string(ret)
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

// announce cycle:
//  1. on connect, all announces are sent
//  2. own keys are announced by a job
//  3. announces that make difference get relayed
func (node *Node) Register(recvd SHA256, address string) error {
	_, err := node.DB.Get('N', recvd.String())
	if err == nil {
		return HaveBetterPath // a shorter path is known
	}
	hash := recvd
	for i := 0; i < MAXD; i++ {
		next := hash.Next()
		_ = node.DB.Set('N', next.String(), hash.String())
		hash = next
	}
	value := fmt.Sprintf("%d,%s", time.Now().UnixNano(), address)
	_ = node.DB.Set('A', recvd.String(), value)
	err = node.DB.Commit()
	if err != nil {
		return err
	}
	next := sha256.Sum256(recvd[:])
	fmt.Printf("\n\rreg %s\r\nfwd %s\r\n", Hex(recvd[:]), Hex(next[:]))

	relay, _ := TLVAppend(nil, 'A', next[:])
	for _, p := range node.peers {
		if p == nil || p.address == address {
			continue
		}
		p.Queue(relay)
	}
	return nil
}

var exposed_cnt int

func (node *Node) ExposedKey(address string, recvd []byte) error {
	exposed_cnt += 1
	err := node.DB.Set('K', "peer" + strconv.Itoa(exposed_cnt), Hex(recvd))
	if err != nil {
		return err
	}
	_ = node.DB.Commit()
	return nil
}

func (node *Node) Announce(kp sodium.BoxKP) error {
	hash0 := CurrentHash(kp.PublicKey)
	return node.Register(hash0, "-")
}

func (node *Node) Expose(kp sodium.BoxKP) error {
	relay, _ := TLVAppend(nil, 'E', kp.PublicKey.Bytes)
	for _, p := range node.peers {
		if p == nil {
			continue
		}
		p.Queue(relay)
	}
	return nil
}

var ErrAddressUnknown = errors.New("Address unknown")
var RouteUnknown = errors.New("RouteUnknown")

func (node *Node) findPrevHash(hash SHA256) (prev SHA256, err error) {
	recprev, err := node.DB.Get('N', hash.String())
	if err != nil {
		return
	}
	prev, err = SHA256Bin([]byte(recprev))
	return
}

func (node *Node) findFwdPeer(hash SHA256) (peer *Peer, err error) {
	recndx, err := node.DB.Get('A', hash.String())
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
	return
}

func (node *Node) Init() (err error) {
	node.peers = make(map[string]*Peer)
	node.listen = make(map[string]net.Listener)
	// TODO replace logger
	err = node.DB.Open("drastijk.db")
	if err != nil {
		return
	}
	
	// Commented. It doesn't work, but We don't need it yet.

	// {
	// 	i := node.DB.Range('L', "", "~")
	// 	for i.Valid() {
	// 		address := string(i.Key())
	// 		go node.Listen(address)
	// 		fmt.Printf("\rListen on %s\r\n", address)
	// 		i.Next()
	// 	}
	// 	i.Close()
	// }

	// {
	// 	i := node.DB.Range('C', "", "~")
	// 	for i.Valid() {
	// 		address := string(i.Key())
	// 		go node.Connect(address, nil)
	// 		fmt.Printf("\rConnect to %s\r\n", address)
	// 		i.Next()
	// 	}
	// 	i.Close()
	// }

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

func (node *Node) RouteMessageUDP(msg Message) error {
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

func (node *Node) RouteMessageTCP(msg Message) error {
	prev, err := node.findPrevHash(msg.to)
	if err != nil {
		return err
	}
	peer, err := node.findFwdPeer(prev)
	if err != nil {
		return err
	}
	if peer == nil { // FIXME check have secret
		index := binary.LittleEndian.Uint32(msg.body[:4])
		snd_key_hex := Hex(msg.body[4:36])
		body := msg.body[36:]

		var keypair sodium.BoxKP
		keypair.PublicKey.Bytes, err = unhexize(snd_key_hex)
		if err != nil {
			return err
		}
		
		fmt.Printf("\r\nReceived for: %s", Hex(msg.to[:]))
		fmt.Printf("\r\nACK index: %d", index)
		fmt.Printf("\r\nSender pubkey: %s", snd_key_hex)
		fmt.Printf("\r\nBody: %s\r\n", body)

		node.SendUDP(keypair.PublicKey, "ACK_" + strconv.FormatUint(uint64(index), 10))
		return nil
	}
	if peer != nil {
		fwd, _ := TLVAppend2(nil, 'R', prev[:], msg.body)
		peer.boxmx.Lock()
		peer.outbox = append(peer.outbox, fwd)
		peer.boxmx.Unlock()
	}
	return nil
}

func HourlyHash(key sodium.BoxPublicKey, time int64) SHA256 {
	time -= time % (60 * 60 * 1000000000) // round to an hour
	var timestr [8]byte
	binary.LittleEndian.PutUint64(timestr[:], uint64(time))
	str := append(key.Bytes, timestr[0:8]...)
	return sha256.Sum256(str)
}

func CurrentHash(key sodium.BoxPublicKey) SHA256 {
	return HourlyHash(key, time.Now().UnixNano())
}

func (node *Node) SendUDP(key sodium.BoxPublicKey, txt string) error {
	hash := CurrentHash(key)
	fmt.Fprintf(os.Stderr, "current hash %s\r\n", hexize(hash[:]))
	for i := 0; i < MAXD; i++ {
		peer, err := node.findFwdPeer(hash)
		if err == nil && peer != nil {
			msg, _ := TLVAppend2(nil, 'M', hash[:], []byte(txt))
			peer.Queue(msg)
			fmt.Fprintf(os.Stderr, "'M' message sent to %s\r\n", hexize(key.Bytes))
			return nil
		}
		hash = sha256.Sum256(hash[:])
	}
	return ErrAddressUnknown
}

var ack_cnt uint32

func (node *Node) SendTCP(key sodium.BoxPublicKey, snd sodium.BoxPublicKey, txt string) error {
	ack_cnt += 1

	hash := CurrentHash(key)
	fmt.Fprintf(os.Stderr, "current hash %s\r\n", hexize(hash[:]))
	for i := 0; i < MAXD; i++ {
		peer, err := node.findFwdPeer(hash)
		if err == nil && peer != nil {
			bs := make([]byte, 4)
    		binary.LittleEndian.PutUint32(bs, ack_cnt)
			msg, _ := TLVAppend2(nil, 'R', hash[:], bytes.Join([][]byte{bs, snd.Bytes, []byte(txt)}, []byte("")))
			peer.Queue(msg)
			fmt.Fprintf(os.Stderr, "'R' message sent to %s\r\n", hexize(key.Bytes))
			println("Payload: ", Hex(bytes.Join([][]byte{bs, snd.Bytes, []byte(txt)}, []byte(""))))
			return nil
		}
		hash = sha256.Sum256(hash[:])
	}
	return ErrAddressUnknown
}

var ErrKeyIsAlreadyDefined = errors.New("the key is already defined")

func hexize(data []byte) string {
	h := make([]byte, len(data)*2)
	hex.Encode(h, data)
	return string(h)
}

func unhexize(hexdata string) (bin []byte, err error) {
	bin = make([]byte, len(hexdata)/2)
	_, err = hex.Decode(bin, []byte(hexdata))
	return
}

// private key 		P nick: 	privhex
// public keys 		K nick: 	pub (hex)
func (node *Node) SaveKeys(name string, keypair sodium.BoxKP) error {
	_, err := node.DB.Get('K', name)
	if err == nil {
		return ErrKeyIsAlreadyDefined
	}
	_ = node.DB.Set('K', name, hexize(keypair.PublicKey.Bytes))
	_ = node.DB.Set('P', name, hexize(keypair.SecretKey.Bytes))
	return node.DB.Commit()
}

func (node *Node) LoadKeys(name string) (keypair sodium.BoxKP, err error) {
	pubhex, err := node.DB.Get('K', name)
	if err != nil {
		return
	}
	keypair.PublicKey.Bytes, err = unhexize(pubhex)
	if err != nil {
		return
	}
	sechex, err := node.DB.Get('P', name)
	if err != nil {
		err = nil
		return
	}
	keypair.SecretKey.Bytes, err = unhexize(sechex)
	return
}

// private key 		P nick: 	privhex
// public keys 		K nick: 	pub (hex)
func (node *Node) ShowKeys() {

	for i := node.DB.Range('K', "", "~"); i.Valid(); i.Next() {
		name := string(i.Key())
		key := i.Value()
		_, err := node.DB.Get('P', name)
		mine := ""
		if err == nil {
			mine = " (mine)"
		}
		fmt.Printf("%s:\t%s%s\r\n", name, string(key), mine)
	}
}

func (node *Node) ShowListenAddresses() {
	i := node.DB.Range('L', "", "~")
	for i.Valid() {
		address := string(i.Key())
		fmt.Printf("%s\r\n", address)
		i.Next()
	}
}

func (node *Node) ShowAll() {
	io := pebble.IterOptions{}
	i := node.DB.DB.NewIter(&io)
	for i.SeekGE([]byte{0}); i.Valid(); i.Next() {
		fmt.Printf("%s:\t%s\r\n", i.Key(), i.Value())
	}
	_ = i.Close()
}
