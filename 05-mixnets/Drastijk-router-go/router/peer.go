package router

import (
	"errors"
	"fmt"
	"net"
	"os"
	"time"
)

func (node *Node) Listen(lstn net.Listener) {
	for {
		con, err := lstn.Accept()
		if err != nil {
			fmt.Println(err.Error())
			break
		}
		node.Connect(con)
	}
}

func (node *Node) Connect(conn net.Conn) int {
	node.boxmx.Lock()
	ndx := len(node.peers)
	node.peers = append(node.peers, &Peer{
		conn: conn,
	})
	node.boxmx.Unlock()
	go node.doWrite(ndx)
	go node.Read(ndx)
	// todo reconnect here
	return ndx
}

func (node *Node) Read(ndx int) (err error) {
	var buf []byte
	var lit byte
	var body []byte
	peer := node.peers[ndx]
	conn := peer.conn
	for conn != nil {
		buf, err = ReadBuf(buf, conn)
		//fmt.Printf("bytes pending %d\n", len(buf))
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
			node.Register(to, ndx)

		case 'M':
			var to SHA256
			copy(to[:], body[:32])
			msg := Message{to: to, body: body[32:]}
			err = node.RouteMessage(msg)
		case 'P':
			fmt.Printf("Pinged by #%d: %s\n\r", ndx, body)
			pong, _ := TLVAppend2(nil, 'O', []byte("Re: "), body)
			peer.boxmx.Lock()
			peer.outbox = append(peer.outbox, pong)
			peer.boxmx.Unlock()
		case 'O':
			fmt.Printf("Ping response from #%d: %s\n\r", ndx, body)
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
	return nil
}

func (node *Node) doWrite(ndx int) {
	var buf = make([]byte, 0, 4096)
	peer := node.peers[ndx]
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
		//fmt.Printf("sent %d bytes\n", n)
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

func (node *Node) Ping(ndx int) error {
	if ndx >= len(node.peers) || node.peers[ndx] == nil {
		return NoSuchPeer
	}
	peer := node.peers[ndx]
	msg, _ := TLVAppend(nil, 'P', []byte("Hello there!"))
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
