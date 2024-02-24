package main

import (
	"fmt"
	"os"
	"strings"
)

type Args struct {
	RootHash string
}

func ParseArgs() (args Args, err error) {
	if len(os.Args) < 2 {
		return Args{}, fmt.Errorf("Argument parsing error. Message: \"The number of arguments is less than 1\".")
	}
	return Args{RootHash: os.Args[1]}, nil
}

const (
	commitRootPrefix = "Root:"
	commitDatePrefix = "Date:"
)

type Commit struct {
	Revision string
	Root     string
	Date     string
	Message  string
}

func ReadCommit(rev string) (c Commit, err error) {
	data, err := os.ReadFile(rev)
	if err != nil {
		return Commit{}, fmt.Errorf("Unknown commit \"%s\". File not found.", rev)
	}

	c.Revision = rev
	lines := strings.Split(string(data), "\n")

	ok := false
	if c.Root, ok = strings.CutPrefix(lines[0], commitRootPrefix); !ok {
		return Commit{}, fmt.Errorf("Invalid commit \"%s\". First line must contain \"%s\"", rev, commitRootPrefix)
	}
	c.Root = strings.TrimSpace(c.Root)

	if c.Date, ok = strings.CutPrefix(lines[1], commitDatePrefix); !ok {
		return Commit{}, fmt.Errorf("Invalid commit \"%s\". Second line must contain \"%s\"", rev, commitDatePrefix)
	}
	c.Date = strings.TrimSpace(c.Date)

	c.Message = strings.Join(lines[2:], "\n")
	return
}

func (c Commit) String() string {
	return strings.Join([]string{
		fmt.Sprintf("commit %s", c.Revision),
		fmt.Sprintf("%s %s", commitRootPrefix, c.Root),
		fmt.Sprintf("%s %s", commitDatePrefix, c.Date),
		c.Message,
	}, "\n")
}

const (
	rootCommitPrefix = ".commit:\t"
	rootParentPrefix = ".parent/\t"
)

type Root struct {
	Parent string
	Commit string
}

func ReadRoot(hash string) (r Root, err error) {
	data, err := os.ReadFile(hash)
	if err != nil {
		return Root{}, fmt.Errorf("Unknown root file \"%s\". File not found.", hash)
	}

	for _, line := range strings.Split(string(data), "\n") {
		if strings.HasPrefix(line, rootCommitPrefix) {
			r.Commit = line[len(rootCommitPrefix):]
		}
		if strings.HasPrefix(line, rootParentPrefix) {
			r.Parent = line[len(rootParentPrefix):]
		}
	}

	return
}

func (r Root) ReadNextRoot() (*Root, error) {
	if r.Parent != "" {
		root, err := ReadRoot(r.Parent)
		return &root, err
	}
	return nil, nil
}

func (r Root) ReadCommit() (*Commit, error) {
	if r.Commit != "" {
		commit, err := ReadCommit(r.Commit)
		return &commit, err
	}
	return nil, nil
}

func ReadCommits(rootHash string) ([]Commit, error) {
	root, err := ReadRoot(rootHash)
	if err != nil {
		return nil, err
	}

	commits := []Commit{}

	for {
		commit, err := root.ReadCommit()
		if err != nil {
			return nil, err
		}

		if commit != nil {
			commits = append(commits, *commit)
		}

		nextRoot, err := root.ReadNextRoot()
		if err != nil {
			return nil, err
		}

		if nextRoot == nil {
			break
		}

		root = *nextRoot
	}

	return commits, nil
}

func PrintCommits(commits []Commit) {
	for _, commit := range commits {
		fmt.Println(commit)
	}
}

func main() {
	args, err := ParseArgs()
	if err != nil {
		panic(err)
	}

	commits, err := ReadCommits(args.RootHash)
	if err != nil {
		panic(err)
	}

	PrintCommits(commits)
}
