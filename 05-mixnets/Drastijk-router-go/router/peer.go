package router

import (
	"crypto/sha256"
	"errors"
	"fmt"
	"github.com/cockroachdb/pebble"
	"net"
	"os"
	"sync"
	"time"
)

type Peer struct {
	address   string
	node      *Node
	announces map[SHA256]uint64
	conn      net.Conn
	outbox    [][]byte
	boxmx     sync.Mutex
}

const RETRY = time.Minute

func (node *Node) Connect(addr string, conn net.Conn) {
	con_backoff := time.Millisecond
	read_backoff := time.Millisecond
	for len(addr) > 0 || conn != nil {
		var err error
		if conn == nil {
			time.Sleep(read_backoff)
			conn, err = net.Dial("tcp", addr)
		}
		if err != nil {
			if con_backoff < RETRY/2 {
				con_backoff = con_backoff * 2
			} else {
				con_backoff = RETRY
			}
			time.Sleep(con_backoff)
			continue
		} else {
			con_backoff = time.Millisecond
		}
		conntime := time.Now()
		key := addr
		if len(addr) == 0 {
			key = conn.RemoteAddr().String()
		}
		node.boxmx.Lock()
		peer := Peer{
			node:    node,
			conn:    conn,
			address: key,
		}
		node.peers[key] = &peer
		node.boxmx.Unlock()
		go peer.doWrite()
		err = peer.Read()
		node.boxmx.Lock()
		delete(node.peers, key)
		node.boxmx.Unlock()
		if err != nil {
			fmt.Fprintln(os.Stderr, err.Error())
			if len(addr) > 0 && conntime.Add(time.Minute*5).After(time.Now()) && read_backoff < RETRY/2 {
				read_backoff *= 2
			} else {
				read_backoff = time.Millisecond
			}
		}
		conn = nil
	}
}

func (peer *Peer) Read() (err error) {
	var buf []byte
	var lit byte
	var body []byte
	conn := peer.conn
	for conn != nil {
		buf, err = ReadBuf(buf, conn)
		//fmt.Fprintf(os.Stderr, "bytes pending %d\n", len(buf))
		if err != nil {
			break
		}
		lit, body, buf, err = TLVTake(buf)
		if err == NoDataYet {
			time.Sleep(time.Millisecond)
			continue
		} else if err != nil {
			break
		}
		switch lit {
		case 'A':
			var to SHA256
			copy(to[:], body[:32])
			err = peer.node.Register(to, peer.address)
			if err == HaveBetterPath {
				err = nil
			}
			if err != nil {
				fmt.Fprintln(os.Stderr, err.Error())
			}
		case 'M':
			var to SHA256
			copy(to[:], body[:32])
			msg := Message{to: to, body: body[32:]}
			fmt.Printf("Messaged by %s: %s\n\r", peer.address, body)
			err = peer.node.RouteMessage(msg)
		case 'P':
			fmt.Printf("Pinged by %s: %s\n\r", peer.address, body)
			pong, _ := TLVAppend2(nil, 'O', []byte("Re: "), body)
			peer.boxmx.Lock()
			peer.outbox = append(peer.outbox, pong)
			peer.boxmx.Unlock()
		case 'O':
			fmt.Printf("Ping response from %s: %s\n\r", peer.address, body)
		default:
			err = errors.New("unsupported message type")
		}
		if err != nil {
			break
		}
		conn = peer.conn
	}
	if err != nil {
		_, _ = fmt.Fprintf(os.Stderr, "%s\n\r", err.Error())
	}

	peer.conn = nil
	return
}

func (peer *Peer) doWrite() {
	var buf = make([]byte, 0, 4096)
	// send all the announces
	io := pebble.IterOptions{}
	db := peer.node.DB
	if db == nil {
		return
	}
	i, err := db.NewIter(&io)
	if err != nil {
		return
	}
	for i.SeekGE(litAnnounce); i.Valid() && i.Key()[0] == litAnnounce[0]; i.Next() {
		// FIXME expiration check
		hex := i.Key()[1:]
		bin, err := unhexize(hex)
		if len(bin) != 32 || err != nil {
			continue
		}
		next := sha256.Sum256(bin)
		buf, _ = TLVAppend(buf, 'A', next[:])
	}
	_ = i.Close()
	// the send loop
	conn := peer.conn
	for conn != nil {
		peer.boxmx.Lock()
		if len(peer.outbox) != 0 {
			buf = append(buf, peer.outbox[0]...)
			peer.outbox = peer.outbox[1:]
			//buf, _ = TLVAppend2(buf, 'M', msg.to[:], msg.body)
		}
		peer.boxmx.Unlock()
		if len(buf) == 0 {
			time.Sleep(time.Millisecond) // TODO backoff
			continue
		}
		n, err := conn.Write(buf)
		//fmt.Fprintf(os.Stderr, "sent %d bytes\n", n)
		if err != nil {
			peer.conn = nil
			_, _ = fmt.Fprint(os.Stderr, err.Error())
			break
		}
		buf = buf[n:]
		conn = peer.conn
	}
}

var NoSuchPeer = errors.New("no such peer")

func (node *Node) Ping(addr string, msgtxt string) error {
	peer, ok := node.peers[addr]
	if !ok {
		return NoSuchPeer
	}
	msg, _ := TLVAppend(nil, 'P', []byte(msgtxt))
	peer.boxmx.Lock()
	peer.outbox = append(peer.outbox, msg)
	peer.boxmx.Unlock()
	return nil
}

func (peer *Peer) Queue(msg []byte) {
	peer.boxmx.Lock()
	peer.outbox = append(peer.outbox, msg)
	peer.boxmx.Unlock()
}
