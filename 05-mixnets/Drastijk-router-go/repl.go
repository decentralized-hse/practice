package main

import (
	"Drastijk/router"
	"fmt"
	"log"
	"net"
	"os"
	"strconv"
	"strings"

	"github.com/jamesruan/sodium"
	repl "github.com/openengineer/go-repl"
)

var commands = map[string]string{
	"help":     "                                  display this message",
	"add":      "n1 n2                             adds two numbers, why not?",
	"listen":   "[host][:port]                     listen on an address (TCP)",
	"connect":  "[host][:port]                     connect to an address",
	"announce": "pubkeyName                        announce a public key",
	"sendUDP":  "recPubkeyName text                send a message to recPubkeyName",
	"sendTCP":  "recPubkeyName sndPubkeyName text  send a message expects ACK",
	"keys":     "name                              generate a named pair of keys",
	"contact":  "pubkeyName pubkey                 add a contact",
	"expose":	"pubkeyName                        expose pubkey to all contacts",
	"commit":   "                                  commit changes to node DB",
	"show":     "[listen/keys/all]                 show node DB entries by filter",
	"ping":     "[host][:port]                     send a ping to a peer",
	"dump":     "[command]                         dump the command's metadata",
	"quit":     "                                  quit this program for good",
	"exit":     "                                  exit",
}

var node = router.Node{}

// implements repl.Handler interface
type MyHandler struct {
	r *repl.Repl
}

func main() {
	fmt.Println("Welcome, type \"help\" for more info")

	err := node.Init()
	if err != nil {
		fmt.Fprintln(os.Stderr, err.Error())
		return
	}

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
	} else if len(fields) == 2 && fields[0] == "dump" {
		c := fields[1]
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

func (h *MyHandler) Eval(line string) string {
	fields := strings.Fields(line)

	if len(fields) == 0 {
		return ""
	} else {
		cmd, args := fields[0], fields[1:]

		switch cmd {
		case "commit":
			_ = node.DB.Commit()
			return ""
		case "help":
			helpMessage := []byte{}
			for k, v := range commands {
				str := fmt.Sprintf("%16s %v\r\n", k, v)
				helpMessage = append(helpMessage, str...)
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
			_ = node.DB.Set('L', addr, "")
			_ = node.DB.Commit()
			go node.Listen(addr)
			return "OK"
		case "connect":
			if len(args) != 1 {
				return "Usage: connect [address][:port]"
			}
			addr := addrDefaults(args[0])
			conn, err := net.Dial("tcp", addr)
			if err != nil {
				return err.Error()
			}
			_ = node.DB.Set('C', addr, "-")
			node.DB.Commit()
			go node.Connect(addr, conn)
			fmt.Printf("\r\npeer %s connected\r\n", addr)
			return ""
		case "ping":
			if len(args) < 1 {
				return Usage(cmd)
			}
			addr := ""
			fmt.Sscanf(args[0], "%s", &addr)
			msg := strings.Join(args[1:], " ")
			err := node.Ping(addr, msg)
			if err != nil {
				return err.Error()
			}
			return ""
		case "announce":
			if len(args) != 1 {
				return Usage(cmd)
			}
			pair, err := node.LoadKeys(args[0])
			if err != nil {
				return err.Error()
			}
			err = node.Announce(pair)
			if err != nil {
				return err.Error()
			}
			return ""
		case "sendUDP":
			if len(args) != 2 {
				return Usage(cmd)
			}
			pair, err := node.LoadKeys(args[0])
			if err != nil {
				return err.Error()
			}
			err = node.SendUDP(pair.PublicKey, args[1])
			if err != nil {
				return err.Error()
			}
			return ""
		case "sendTCP":
			if len(args) != 3 {
				return Usage(cmd)
			}
			pair, err := node.LoadKeys(args[0])
			if err != nil {
				return err.Error()
			}
			pair_my, err := node.LoadKeys(args[1])
			if err != nil {
				return err.Error()
			}
			err = node.SendTCP(pair.PublicKey, pair_my.PublicKey, args[2])
			if err != nil {
				return err.Error()
			}
			return ""
		case "keys":
			if len(args) != 1 {
				return Usage(cmd)
			}
			name := args[0]
			keypair := sodium.MakeBoxKP()
			err := node.SaveKeys(name, keypair)
			if err != nil {
				return err.Error()
			}
			return ""
		case "contact":
			if len(args) != 2 {
				return Usage(cmd)
			}
			name := args[0]
			pubkey := args[1]
			if len(pubkey) != 64 {
				return "bad public key length"
			}
			_, err := node.DB.Get('K', name)
			if err == nil {
				return "the name is already known"
			}
			err = node.DB.Set('K', name, pubkey)
			if err != nil {
				return err.Error()
			}
			_ = node.DB.Commit()
			return ""
		case "expose":
			if len(args) != 1 {
				return Usage(cmd)
			}
			pair, err := node.LoadKeys(args[0])
			if err != nil {
				return err.Error()
			}
			err = node.Expose(pair)
			if err != nil {
				return err.Error()
			}
			return ""
		case "add": //7f6d324080cc5c289974f6bc121cb3316eb41bc41915d615cc6cdc07c8ad8165
			if len(args) != 2 {
				return "\"add\" expects 2 args"
			} else {
				return add(args[0], args[1])
			}
		case "quit", "exit":
			node.DB.Close()
			h.r.Quit()
			return ""
		case "show", "list":
			for i := 0; i < len(args); i++ {
				cmd := args[i]
				switch cmd {
				case "listen":
					node.ShowListenAddresses()
				case "keys":
					node.ShowKeys()
				case "all":
					node.ShowAll()
				default:
					fmt.Fprintf(os.Stdout, "no dump for command %s\r\n", cmd)
				}
			}
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
