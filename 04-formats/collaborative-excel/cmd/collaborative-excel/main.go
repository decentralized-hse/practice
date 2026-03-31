package main

import (
	"context"
	"flag"
	"fmt"
	"strings"
	"time"

	"collaborative-excel/internal/core"
	collabsync "collaborative-excel/internal/sync"
	"collaborative-excel/internal/ui"
)

func main() {
	listenAddr := flag.String("listen", "/ip4/127.0.0.1/tcp/0", "libp2p listen address")
	bootstrap := flag.String("bootstrap", "", "comma separated bootstrap peers")
	name := flag.String("name", "", "node identifier")
	shareMode := flag.Bool("share", false, "print a ready-to-copy bootstrap command and exit")
	rows := flag.Int("rows", 20, "sheet rows")
	cols := flag.Int("cols", 12, "sheet columns")
	flag.Parse()

	actorID := strings.TrimSpace(*name)
	if actorID == "" {
		actorID = time.Now().Format("150405.000")
	}

	ctx := context.Background()
	service := core.NewService(actorID)
	updates := service.Subscribe()

	networkCfg := collabsync.Config{
		ListenAddress: *listenAddr,
		Bootstrap:     splitPeers(*bootstrap),
		TopicName:     "collaborative-excel",
	}

	networkClient, err := collabsync.NewClient(ctx, networkCfg)
	if err != nil {
		fmt.Printf("sync disabled: %v\n", err)
	} else {
		defer networkClient.Close()
	}

	if networkClient != nil {
		fmt.Printf("peer=%s\n", actorID)
		fmt.Printf("libp2p_peer=%s\n", networkClient.LocalPeerID())
		for _, addr := range networkClient.LocalAddresses() {
			fmt.Printf("addr=%s\n", addr)
		}
	}
	if *shareMode {
		if networkClient == nil {
			fmt.Println("share unavailable: sync disabled")
		} else {
			shareArg := strings.Join(buildShareAddresses(networkClient.LocalAddresses()), ",")
			fmt.Printf("share=%q\n", shareArg)
			fmt.Printf("share_cmd=go run ./cmd/collaborative-excel -rows %d -cols %d -listen \"/ip4/0.0.0.0/tcp/0\" -bootstrap %q\n", *rows, *cols, shareArg)
		}
	}

	if networkClient != nil {
		go func() {
			for update := range networkClient.Incoming() {
				fmt.Printf(
					"sync: inbound op cell=%s actor=%s clock=%d value=%q\n",
					update.CellID,
					update.ActorID,
					update.Clock,
					update.Value,
				)
				service.RemoteUpdate(update)
			}
		}()
	}

	publishLocal := func(op core.SetCellOp) {
		if networkClient == nil {
			return
		}
		if err := networkClient.Publish(op); err != nil {
			fmt.Printf("sync: failed to publish op: %v\n", err)
			return
		}
		fmt.Printf(
			"sync: published op cell=%s actor=%s clock=%d value=%q\n",
			op.CellID,
			op.ActorID,
			op.Clock,
			op.Value,
		)
	}

	ui.Run(service, *rows, *cols, publishLocal, updates)
}

func splitPeers(raw string) []string {
	fields := strings.FieldsFunc(raw, func(r rune) bool {
		return r == ',' || r == '\n' || r == '\r' || r == '\t' || r == ' '
	})
	clean := make([]string, 0, len(fields))
	for _, part := range fields {
		if trimmed := strings.TrimSpace(part); trimmed != "" {
			clean = append(clean, trimmed)
		}
	}
	return clean
}

func buildShareAddresses(addresses []string) []string {
	filtered := make([]string, 0, len(addresses))
	for _, addr := range addresses {
		trimmed := strings.TrimSpace(addr)
		if trimmed == "" {
			continue
		}
		if strings.Contains(trimmed, "/ip4/127.") ||
			strings.Contains(trimmed, "/ip6/::1") ||
			strings.Contains(trimmed, "/ip4/0.0.0.0") {
			continue
		}
		filtered = append(filtered, trimmed)
	}
	if len(filtered) == 0 {
		filtered = append(filtered, addresses...)
	}
	return filtered
}
