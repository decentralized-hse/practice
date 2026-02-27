package router

import (
	"crypto/sha256"
	"errors"
	"fmt"
	"net"
	"os"
	"sync"
	"time"
)

type Peer struct {
	address       string
	node          *Node
	announces     map[SHA256]uint64
	conn          net.Conn
	outbox        [][]byte
	boxmx         sync.Mutex
	LoadIndex     float32
	SendDurations []time.Duration
}

const (
	RETRY                  = time.Minute
	MTU                    = 256
	LoadedNetworkThreshold = 1024
	FreeNetworkThreshold   = 512
	LoadIncreaseStep       = 1.2
	LoadDecreaseStep       = 0.8
)

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
		case 'E':
			err = peer.node.ExposedKey(peer.address, body[:32])
			if err != nil {
				fmt.Fprintln(os.Stderr, err.Error())
			}
		case 'M':
			var to SHA256
			copy(to[:], body[:32])
			msg := Message{to: to, body: body[32:]}
			fmt.Printf("\r\nMessaged by %s: %s\r\n", peer.address, body)
			err = peer.node.RouteMessageUDP(msg)
		case 'R':
			var to SHA256
			copy(to[:], body[:32])
			msg := Message{to: to, body: body[32:]}
			fmt.Printf("\r\nMessaged by %s: %s\r\n", peer.address, body)
			err = peer.node.RouteMessageTCP(msg)
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
	i := peer.node.DB.Range('A', "", "~")
	for i.Next() {
		// FIXME expiration check
		hex := i.Key()[1:]
		bin, err := unhexize(hex)
		if len(bin) != 32 || err != nil {
			continue
		}
		next := sha256.Sum256(bin)
		buf, _ = TLVAppend(buf, 'A', next[:])
	}
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

		err := peer.WriteByChunks(buf)
		peer.UpdateWorkload()

		if err != nil {
			fmt.Printf("failed to send bytes by chunks: %v", err)
			break
		}

		//fmt.Fprintf(os.Stderr, "sent %d bytes\n", n)
		if err != nil {
			peer.conn = nil
			_, _ = fmt.Fprint(os.Stderr, err.Error())
			break
		}
		buf = make([]byte, 0)
		conn = peer.conn
	}
}

func (peer *Peer) WriteByChunks(buf []byte) error {
	// split to chunks
	for i := 0; i < len(buf); i += MTU {
		start := time.Now()
		_, err := peer.conn.Write(buf[i : i+MTU])
		if err != nil {
			return err
		}

		// save delivery time for controlling system load
		peer.SendDurations = append(peer.SendDurations, time.Since(start))
	}
	return nil
}

func (peer *Peer) AverageLoad() time.Duration {
	total := time.Duration(0)
	for _, d := range peer.SendDurations {
		total += d
	}
	return total / time.Duration(len(peer.SendDurations))
}

func (peer *Peer) UpdateWorkload() {
	currentLoad := peer.AverageLoad()
	if currentLoad >= LoadedNetworkThreshold {
		peer.LoadIndex *= LoadIncreaseStep
	} else if currentLoad < FreeNetworkThreshold {
		peer.LoadIndex *= LoadDecreaseStep
	}
	fmt.Printf("current workload: %v", peer.LoadIndex)
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
