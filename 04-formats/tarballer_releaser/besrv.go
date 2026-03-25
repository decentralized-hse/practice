package main

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strconv"
	"time"
)

// startBeSrv runs be-srv in worktree (must contain .be). Port is the TCP listen port.
func startBeSrv(bin, worktree string, port int) (*exec.Cmd, error) {
	be := filepath.Join(worktree, ".be")
	fi, err := os.Stat(be)
	if err != nil {
		return nil, fmt.Errorf("worktree %q: .be not found: %w", worktree, err)
	}
	if fi.IsDir() {
		return nil, fmt.Errorf("worktree %q: .be must be a file", worktree)
	}
	cmd := exec.Command(bin, strconv.Itoa(port))
	cmd.Dir = worktree
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	if err := cmd.Start(); err != nil {
		return nil, fmt.Errorf("start %s: %w", bin, err)
	}
	return cmd, nil
}

func waitForBeagle(client *BeagleClient, timeout time.Duration) error {
	deadline := time.Now().Add(timeout)
	var last error
	for time.Now().Before(deadline) {
		if err := client.Ping(); err == nil {
			return nil
		} else {
			last = err
		}
		time.Sleep(150 * time.Millisecond)
	}
	if last != nil {
		return fmt.Errorf("be-srv not ready after %v: %w", timeout, last)
	}
	return fmt.Errorf("be-srv not ready after %v", timeout)
}
