package main

import (
	"fmt"
	"io"
	"net/http"
	"net/url"
	"strings"
	"time"
)

type DirEntry struct {
	Name  string
	IsDir bool
}

type WalkFunc func(path string, content []byte) error

type BeagleClient struct {
	BaseURL    string
	HTTPClient *http.Client
}

func NewBeagleClient(baseURL string) *BeagleClient {
	return &BeagleClient{
		BaseURL: strings.TrimRight(baseURL, "/"),
		HTTPClient: &http.Client{
			Timeout: 5 * time.Minute,
		},
	}
}

// buildURL builds a request URL for be-srv (see librdx be/HTTP.md: DIR /path/, RAW /path/file, ?formula).
// path uses a leading slash (e.g. /, /src/); directories must end with / for DIR mode.
func (c *BeagleClient) buildURL(path, formula, suffix string) string {
	p := strings.TrimPrefix(path, "/")
	u := c.BaseURL + "/" + p + suffix
	if formula != "" {
		u += "?" + url.QueryEscape(formula)
	}
	return u
}

func (c *BeagleClient) ListDir(path, formula string) ([]DirEntry, error) {
	if !strings.HasSuffix(path, "/") {
		path += "/"
	}
	reqURL := c.buildURL(path, formula, "")
	resp, err := c.HTTPClient.Get(reqURL)
	if err != nil {
		return nil, fmt.Errorf("be-srv unreachable: %w", err)
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		return nil, fmt.Errorf("be-srv returned %d for %s", resp.StatusCode, reqURL)
	}

	body, err := io.ReadAll(resp.Body)
	if err != nil {
		return nil, err
	}

	var entries []DirEntry
	for _, line := range strings.Split(strings.TrimSpace(string(body)), "\n") {
		line = strings.TrimSpace(line)
		if line == "" {
			continue
		}
		if strings.HasSuffix(line, "/") {
			entries = append(entries, DirEntry{Name: line, IsDir: true})
		} else {
			entries = append(entries, DirEntry{Name: line, IsDir: false})
		}
	}
	return entries, nil
}

func (c *BeagleClient) GetFile(path, formula string) ([]byte, error) {
	path = strings.TrimRight(path, "/")
	reqURL := c.buildURL(path, formula, "")
	resp, err := c.HTTPClient.Get(reqURL)
	if err != nil {
		return nil, fmt.Errorf("be-srv unreachable: %w", err)
	}
	defer resp.Body.Close()

	if resp.StatusCode == http.StatusNotFound {
		return nil, fmt.Errorf("file not found: %s", path)
	}
	if resp.StatusCode != http.StatusOK {
		return nil, fmt.Errorf("be-srv returned %d for %s", resp.StatusCode, reqURL)
	}

	return io.ReadAll(resp.Body)
}

// Walk recursively traverses the be-srv directory tree, calling fn for each file.
func (c *BeagleClient) Walk(prefix, formula string, fn WalkFunc) error {
	return c.walkDir(prefix, formula, fn)
}

func (c *BeagleClient) walkDir(dir, formula string, fn WalkFunc) error {
	entries, err := c.ListDir(dir, formula)
	if err != nil {
		return err
	}

	for _, e := range entries {
		fullPath := strings.TrimRight(dir, "/") + "/" + e.Name
		if dir == "/" || dir == "" {
			fullPath = "/" + e.Name
		}

		if e.IsDir {
			if err := c.walkDir(fullPath, formula, fn); err != nil {
				return err
			}
		} else {
			content, err := c.GetFile(fullPath, formula)
			if err != nil {
				return fmt.Errorf("fetching %s: %w", fullPath, err)
			}
			if err := fn(fullPath, content); err != nil {
				return err
			}
		}
	}
	return nil
}

// Ping checks if be-srv is reachable (GET / — root directory listing).
func (c *BeagleClient) Ping() error {
	u := c.buildURL("/", "", "")
	resp, err := c.HTTPClient.Get(u)
	if err != nil {
		return fmt.Errorf("be-srv unreachable at %s: %w", c.BaseURL, err)
	}
	defer resp.Body.Close()
	_, _ = io.Copy(io.Discard, resp.Body)
	if resp.StatusCode != http.StatusOK {
		return fmt.Errorf("be-srv at %s returned HTTP %d", c.BaseURL, resp.StatusCode)
	}
	return nil
}
