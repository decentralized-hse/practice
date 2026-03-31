package collabsync

import (
	"context"
	"encoding/json"
	"fmt"
	"strings"
	"sync"

	"collaborative-excel/internal/core"

	"github.com/libp2p/go-libp2p"
	pubsub "github.com/libp2p/go-libp2p-pubsub"
	"github.com/libp2p/go-libp2p/core/host"
	"github.com/libp2p/go-libp2p/core/peer"
	ma "github.com/multiformats/go-multiaddr"
)

type Config struct {
	ListenAddress string
	Bootstrap     []string
	TopicName     string
}

type Client struct {
	ctx          context.Context
	cancel       context.CancelFunc
	host         host.Host
	topic        *pubsub.Topic
	subscription *pubsub.Subscription
	inbound      chan core.SetCellOp
	closeOnce    sync.Once
}

func NewClient(ctx context.Context, cfg Config) (*Client, error) {
	if strings.TrimSpace(cfg.ListenAddress) == "" {
		cfg.ListenAddress = "/ip4/0.0.0.0/tcp/0"
	}
	if strings.TrimSpace(cfg.TopicName) == "" {
		cfg.TopicName = "collaborative-excel"
	}

	listener, err := libp2p.New(libp2p.ListenAddrStrings(cfg.ListenAddress))
	if err != nil {
		return nil, err
	}

	pubsubService, err := pubsub.NewGossipSub(ctx, listener)
	if err != nil {
		_ = listener.Close()
		return nil, err
	}

	topic, err := pubsubService.Join(cfg.TopicName)
	if err != nil {
		_ = listener.Close()
		return nil, err
	}
	fmt.Printf("sync: joined topic=%s\n", cfg.TopicName)

	subscription, err := topic.Subscribe()
	if err != nil {
		_ = listener.Close()
		return nil, err
	}

	client := &Client{
		ctx:          ctx,
		host:         listener,
		topic:        topic,
		subscription: subscription,
		inbound:      make(chan core.SetCellOp, 32),
	}
	clientCtx, cancel := context.WithCancel(ctx)
	client.ctx = clientCtx
	client.cancel = cancel
	client.connectBootstrapPeers(ctx, cfg.Bootstrap)
	client.startReceiving()

	return client, nil
}

func (c *Client) LocalAddresses() []string {
	self := c.host.ID().String()
	addrs := make([]string, 0, len(c.host.Addrs()))
	for _, address := range c.host.Addrs() {
		addrs = append(addrs, fmt.Sprintf("%s/p2p/%s", address.String(), self))
	}
	return addrs
}

func (c *Client) LocalPeerID() string {
	return c.host.ID().String()
}

func (c *Client) Incoming() <-chan core.SetCellOp {
	return c.inbound
}

func (c *Client) Publish(op core.SetCellOp) error {
	data, err := json.Marshal(op)
	if err != nil {
		return err
	}
	return c.topic.Publish(c.ctx, data)
}

func (c *Client) Close() error {
	c.cancel()
	c.close()
	return c.host.Close()
}

func (c *Client) startReceiving() {
	go func() {
		for {
			msg, err := c.subscription.Next(c.ctx)
			if err != nil {
				c.close()
				return
			}
			if msg.ReceivedFrom == c.host.ID() {
				continue
			}

			var op core.SetCellOp
			if err := json.Unmarshal(msg.Data, &op); err != nil {
				fmt.Printf("sync: malformed inbound msg from=%s err=%v\n", msg.ReceivedFrom, err)
				continue
			}

			fmt.Printf(
				"sync: received msg from=%s cell=%s actor=%s clock=%d value=%q\n",
				msg.ReceivedFrom,
				op.CellID,
				op.ActorID,
				op.Clock,
				op.Value,
			)
			c.inbound <- op
		}
	}()
}

func (c *Client) connectBootstrapPeers(ctx context.Context, peers []string) {
	for _, raw := range peers {
		raw = strings.TrimSpace(raw)
		if raw == "" {
			continue
		}

		addr, err := ma.NewMultiaddr(raw)
		if err != nil {
			fmt.Printf("sync: invalid bootstrap address %q: %v\n", raw, err)
			continue
		}
		peerInfo, err := peer.AddrInfoFromP2pAddr(addr)
		if err != nil {
			fmt.Printf("sync: invalid bootstrap peer address %q: %v\n", raw, err)
			continue
		}
		if err := c.host.Connect(ctx, *peerInfo); err != nil {
			fmt.Printf(
				"sync: failed to connect to bootstrap peer=%s addr=%s: %v\n",
				peerInfo.ID.String(),
				addr,
				err,
			)
			continue
		}

		fmt.Printf("sync: connected to bootstrap peer=%s addr=%s\n", peerInfo.ID.String(), addr)
	}
}

func (c *Client) close() {
	c.closeOnce.Do(func() {
		close(c.inbound)
	})
}
