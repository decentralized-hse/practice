package main

import (
	"encoding/json"
	"fmt"
	"os"
	"regexp"
	"sort"
	"sync"
	"time"
)

var validName = regexp.MustCompile(`^[a-zA-Z0-9][a-zA-Z0-9._-]{0,63}$`)

type Release struct {
	Name        string    `json:"name"`
	Description string    `json:"description"`
	Formula     string    `json:"formula"`
	Prefix      string    `json:"prefix"`
	CreatedAt   time.Time `json:"created_at"`
}

type ReleaseStore struct {
	mu       sync.RWMutex
	filepath string
	releases map[string]Release
}

func NewReleaseStore(filepath string) (*ReleaseStore, error) {
	s := &ReleaseStore{
		filepath: filepath,
		releases: make(map[string]Release),
	}
	if err := s.load(); err != nil && !os.IsNotExist(err) {
		return nil, fmt.Errorf("loading releases: %w", err)
	}
	return s, nil
}

func (s *ReleaseStore) load() error {
	data, err := os.ReadFile(s.filepath)
	if err != nil {
		return err
	}
	var list []Release
	if err := json.Unmarshal(data, &list); err != nil {
		return err
	}
	for _, r := range list {
		s.releases[r.Name] = r
	}
	return nil
}

func (s *ReleaseStore) save() error {
	list := s.listLocked()
	data, err := json.MarshalIndent(list, "", "  ")
	if err != nil {
		return err
	}
	return os.WriteFile(s.filepath, data, 0o644)
}

func (s *ReleaseStore) listLocked() []Release {
	list := make([]Release, 0, len(s.releases))
	for _, r := range s.releases {
		list = append(list, r)
	}
	sort.Slice(list, func(i, j int) bool {
		return list[i].CreatedAt.After(list[j].CreatedAt)
	})
	return list
}

func (s *ReleaseStore) List() []Release {
	s.mu.RLock()
	defer s.mu.RUnlock()
	return s.listLocked()
}

func (s *ReleaseStore) Get(name string) (Release, bool) {
	s.mu.RLock()
	defer s.mu.RUnlock()
	r, ok := s.releases[name]
	return r, ok
}

func (s *ReleaseStore) Create(r Release) error {
	if !validName.MatchString(r.Name) {
		return fmt.Errorf("invalid release name %q: use alphanumeric, dots, dashes, underscores", r.Name)
	}
	s.mu.Lock()
	defer s.mu.Unlock()
	if _, exists := s.releases[r.Name]; exists {
		return fmt.Errorf("release %q already exists", r.Name)
	}
	r.CreatedAt = time.Now().UTC()
	s.releases[r.Name] = r
	return s.save()
}

func (s *ReleaseStore) Delete(name string) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	if _, exists := s.releases[name]; !exists {
		return fmt.Errorf("release %q not found", name)
	}
	delete(s.releases, name)
	return s.save()
}
