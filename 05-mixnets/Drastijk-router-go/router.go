package main

import (
	"Drastijk/router"
	"encoding/hex"
	"fmt"
	"github.com/jamesruan/sodium"
	repl "github.com/openengineer/go-repl"
	"log"
	"net"
	"os"
	"strconv"
	"strings"
)

var commands = map[string]string{
	"help":     "display this message",
	"listen":   "[host][:port]	network listen (TCP)",
	"connect":  "[host][:port]	connect to a node",
	"announce": "[keyfile]		announce a public key",
	"keygen":   "keyfile			generate a pair of keys",
	"ping":     "[ndx]			send a ping to a peer",
	"quit":     "				quit this program for good",
	"exit":     "				exit",
}

var node = router.Node{}

// implements repl.Handler interface
type MyHandler struct {
	r *repl.Repl
}

func main() {
	fmt.Println("Welcome, type \"help\" for more info")

	h := &MyHandler{}
	h.r = repl.NewRepl(h)

	// start the terminal loop
	if err := h.r.Loop(); err != nil {
		log.Fatal(err)
	}
}

func (h *MyHandler) Prompt() string {
	return "> "
}

func (h *MyHandler) Tab(buffer string) string {
	if len(buffer) == 0 {
		return ""
	}
	fields := strings.Fields(buffer)
	if len(fields) == 1 {
		c := fields[0]
		for cmd, _ := range commands {
			if len(c) < len(cmd) && strings.HasPrefix(cmd, c) {
				return cmd[len(c):] + " "
			}
		}
	}
	return ""
}

func addrDefaults(addr string) string {
	i := strings.IndexByte(addr, ':')
	if i == -1 {
		addr = addr + ":8999"
	} else if i == 0 {
		addr = "0.0.0.0" + addr
	}
	return addr
}

func Usage(cmd string) string {
	return "Usage: " + cmd + " " + commands[cmd]
}

func SaveKeys(fn string, keypair sodium.BoxKP) error {
	file, err := os.Create(fn + ".keys")
	if err != nil {
		return err
	}
	txt := make([]byte, keypair.PublicKey.Length()*2+keypair.SecretKey.Length()*2+2)
	hex.Encode(txt, keypair.PublicKey.Bytes)
	txt[keypair.PublicKey.Length()*2] = '\n'
	hex.Encode(txt, keypair.SecretKey.Bytes[keypair.PublicKey.Length()*2:])
	txt[keypair.PublicKey.Length()*2+keypair.SecretKey.Length()*2+1] = '\n'
	_, err = file.Write(txt)
	if err != nil {
		return err
	}
	return file.Close()
}

func LoadKeys(fn string) (keypair sodium.BoxKP, err error) {
	file, err := os.Open(fn + ".keys")
	if err != nil {
		return
	}
	var pub string
	var sec string
	_, err = fmt.Fscan(file, &pub, &sec)
	if err != nil {
		return
	}
	keypair.PublicKey.Bytes = make([]byte, len(pub)/2)
	keypair.SecretKey.Bytes = make([]byte, len(sec)/2)
	_, err = hex.Decode(keypair.PublicKey.Bytes, []byte(pub))
	if err != nil {
		return
	}
	_, err = hex.Decode(keypair.SecretKey.Bytes, []byte(sec))
	if err != nil {
		return
	}
	err = file.Close()
	return
}

func (h *MyHandler) Eval(line string) string {
	fields := strings.Fields(line)

	if len(fields) == 0 {
		return ""
	} else {
		cmd, args := fields[0], fields[1:]

		switch cmd {
		case "help":
			helpMessage := []byte{}
			for k, v := range commands {
				helpMessage = append(helpMessage, k...)
				helpMessage = append(helpMessage, '\t')
				helpMessage = append(helpMessage, v...)
				helpMessage = append(helpMessage, '\n')
			}
			return string(helpMessage)
		case "listen":
			addr := ""
			if len(args) == 0 {
				addr = "0.0.0.0:8999"
			} else if len(args) > 1 {
				return "Usage: listen [address][:port]"
			} else {
				addr = addrDefaults(args[0])
			}
			listener, err := net.Listen("tcp", addr)
			if err != nil {
				return err.Error()
			}
			go node.Listen(listener)
			return "OK"
		case "connect":
			if len(args) != 1 {
				return "Usage: connect [address][:port]"
			}
			addr := addrDefaults(args[0])
			con, err := net.Dial("tcp", addr)
			if err != nil {
				return err.Error()
			}
			ndx := node.Connect(con)
			fmt.Printf("peer %d connected: %s\n", ndx, addr)
			return ""
		case "ping":
			if len(args) != 1 {
				return Usage(cmd)
			}
			ndx := 0
			fmt.Sscanf(args[0], "%d", &ndx)
			err := node.Ping(ndx)
			if err != nil {
				return err.Error()
			}
			return ""
		case "announce":
			if len(args) != 1 {
				return Usage(cmd)
			}
			pair, err := LoadKeys(args[0])
			if err != nil {
				return err.Error()
			}
			node.Announce(pair)
			return ""
		case "keygen":
			if len(args) != 1 {
				return Usage(cmd)
			}
			fn := args[0]
			keypair := sodium.MakeBoxKP()
			err := SaveKeys(fn, keypair)
			if err != nil {
				return err.Error()
			}
			return ""
		case "add":
			if len(args) != 2 {
				return "\"add\" expects 2 args"
			} else {
				return add(args[0], args[1])
			}
		case "quit", "exit":
			h.r.Quit()
			return ""
		default:
			return fmt.Sprintf("unrecognized command \"%s\"", cmd)
		}
	}
}

func add(a_ string, b_ string) string {
	a, err := strconv.Atoi(a_)
	if err != nil {
		return "first arg is not an integer"
	}

	b, err := strconv.Atoi(b_)
	if err != nil {
		return "second arg is not an integer"
	}

	return strconv.Itoa(a + b)
}
