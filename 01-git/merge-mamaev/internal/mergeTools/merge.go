package mergeTools

import (
	"errors"
	"fmt"
	"log"
	"os"
	"strings"
)

func getParent(hash string) string {
	file, err := os.ReadFile(hash)
	if err != nil {
		return ""
	}
	lines := strings.Split(string(file), "\n")
	for _, line := range lines {
		if !strings.HasPrefix(line, ".parent") {
			continue
		}
		hash := strings.Split(line, "\t")[1]
		return hash
	}

	return ""
}

func getDepth(hash string) int {
	count := 0
	parent := getParent(hash)
	for parent != "" {
		parent = getParent(parent)
		count += 1
	}
	return count
}

func lca(oldHash, newHash string) string {
	lenOld := getDepth(oldHash)
	lenNew := getDepth(newHash)
	for lenOld != lenNew {
		if lenOld > lenNew {
			oldHash = getParent(oldHash)
			lenOld -= 1
		} else {
			newHash = getParent(newHash)
			lenNew -= 1
		}
	}
	for oldHash != newHash {
		oldHash = getParent(oldHash)
		newHash = getParent(newHash)
	}

	return oldHash
}

func MergeDirectories(targetHash, mergingHash string, comment []string) Commit {
	mutual := lca(targetHash, mergingHash)
	data, err := os.ReadFile(mutual)
	if err != nil {
		log.Fatal("Cannot read file of tree " + mutual)
		os.Exit(1)
	}
	initial := make(map[string]string)
	for _, line := range strings.Split(string(data), "\n") {
		if strings.HasPrefix(line, ".parent") || strings.HasPrefix(line, ".commit") || len(line) == 0 {
			continue
		}
		pair := getEntryAndHash(line)
		initial[pair.Entry] = pair.Hash

	}

	a := getUpcomingChanges(targetHash, initial)
	b := getUpcomingChanges(mergingHash, initial)

	mutualChanges, err := makeMutualChanges(&a, &b)
	if err != nil {
		log.Fatal(err)
		os.Exit(1)
	}
	fmt.Println(mutualChanges)
	content := formCommit(&mutualChanges, initial)

	commit := Commit{Parent: []string{targetHash, mergingHash}, Comment: comment, Content: content}
	return commit
}

func makeMutualChanges(target, merged *Changes) (Changes, error) {
	deletedHashMap := make(map[EntryAndHashPair]int)
	for _, pair := range merged.Deleted {
		deletedHashMap[pair] = 1
	}
	for _, pair := range target.Deleted {
		deletedHashMap[pair] = 1
	}
	deleted := make([]EntryAndHashPair, 0, len(deletedHashMap))
	for eahp := range deletedHashMap {
		deleted = append(deleted, eahp)
	}

	changedConflicts := make([][]EntryAndHashPair, 0)
	changedHashMap := make(map[string]string)
	for _, pair := range merged.Changed {
		changedHashMap[pair.Entry] = pair.Hash
	}
	for _, pair := range target.Changed {
		if existingHash, ok := changedHashMap[pair.Entry]; ok {
			if existingHash == pair.Hash {
				//If both trees have equal changes, IMHO there is no conflict
				continue
			}
			changedConflicts = append(changedConflicts, []EntryAndHashPair{
				{Entry: pair.Entry, Hash: existingHash}, pair})
		} else {
			changedHashMap[pair.Entry] = pair.Hash
		}
	}
	changed := make([]EntryAndHashPair, 0, len(changedHashMap))
	for eahp := range changedHashMap {
		changed = append(changed, EntryAndHashPair{eahp, changedHashMap[eahp]})
	}

	addedConflicts := make([][]EntryAndHashPair, 0)
	addedHashMap := make(map[string]string)
	for _, pair := range merged.Added {
		addedHashMap[pair.Entry] = pair.Hash
	}
	for _, pair := range target.Added {
		if existingHash, ok := addedHashMap[pair.Entry]; ok {
			if existingHash == pair.Hash {
				//If both trees have equal changes, IMHO there is no conflict
				continue
			}
			addedConflicts = append(addedConflicts, []EntryAndHashPair{
				{Entry: pair.Entry, Hash: existingHash}, pair})
		} else {
			addedHashMap[pair.Entry] = pair.Hash
		}
	}
	added := make([]EntryAndHashPair, 0, len(addedHashMap))
	for eahp := range addedHashMap {
		added = append(added, EntryAndHashPair{eahp, addedHashMap[eahp]})
	}

	if len(addedConflicts)+len(changedConflicts) > 0 {
		fmt.Println(buildConflictMessage(&addedConflicts, &changedConflicts))
		return Changes{}, errors.New("Fatal error in merge, aborting...")
	}

	return Changes{Deleted: deleted, Added: added, Changed: changed}, nil
}

func buildConflictMessage(changed, added *[][]EntryAndHashPair) string {
	message := "Failed to merge: there was conflicts in following files\n\n"
	if len(*changed) != 0 {
		message += "files/directories concurrently modified:\n\n"
		for _, item := range *changed {
			message += fmt.Sprintf("\t%s -- merging [%s...] -> target [%s...]\n", item[0].Entry, item[0].Hash[0:8], item[1].Hash[0:8])
		}
	}
	if len(*added) != 0 {
		message += "files/directories concurrently added:\n\n"
		for _, item := range *added {
			message += fmt.Sprintf("\t%s -- merging [%s...] -> target [%s...]\n", item[0].Entry, item[0].Hash[0:8], item[1].Hash[0:8])
		}
	}
	return message
}

func formCommit(changes *Changes, initial map[string]string) []string {

	for _, pair := range changes.Deleted {
		delete(initial, pair.Entry)
	}

	for _, pair := range changes.Changed {
		initial[pair.Entry] = pair.Hash
	}

	for _, pair := range changes.Added {
		initial[pair.Entry] = pair.Hash
	}

	final := make([]string, 0)
	for key := range initial {
		final = append(final, fmt.Sprintf("%s\t%s", key, initial[key]))
	}
	return final
}

func getUpcomingChanges(hash string, previous map[string]string) Changes {
	data, err := os.ReadFile(hash)
	if err != nil {
		os.Exit(1)
	}
	deleted := make([]EntryAndHashPair, 0)
	visited := make(map[string]int)

	changed := make([]EntryAndHashPair, 0)

	added := make([]EntryAndHashPair, 0)
	for _, line := range strings.Split(string(data), "\n") {
		if strings.HasPrefix(line, ".parent") || strings.HasPrefix(line, ".commit") || len(line) == 0 {
			continue
		}
		pair := getEntryAndHash(line)
		if previousHash, ok := previous[pair.Entry]; ok {
			visited[pair.Entry] = 1
			if pair.Hash == previousHash {
				continue
			} else {
				changed = append(changed, pair)
			}
		} else {
			added = append(added, pair)
		}
	}

	for pKey := range previous {
		if _, ok := visited[pKey]; ok {
			continue
		} else {
			deleted = append(deleted, EntryAndHashPair{Entry: pKey, Hash: previous[pKey]})
		}
	}

	return Changes{Deleted: deleted, Changed: changed, Added: added}
}

func getEntryAndHash(line string) EntryAndHashPair {
	parts := strings.Split(line, "\t")
	return EntryAndHashPair{Entry: parts[0], Hash: parts[1]}
}

type EntryAndHashPair struct {
	Entry string
	Hash  string
}

type Changes struct {
	Deleted []EntryAndHashPair
	Changed []EntryAndHashPair
	Added   []EntryAndHashPair
}

type Commit struct {
	Parent  []string
	Comment []string
	Content []string
}
