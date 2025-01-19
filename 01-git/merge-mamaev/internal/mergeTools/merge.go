package mergeTools

import (
	"errors"
	"fmt"
	"log"
	"os"
	"strings"
)

type Commit struct {
	Parent  []string
	Comment []string
	Content []string
}

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
		entry, hash := getEntryAndHash(line)
		initial[entry] = hash

	}

	a := GetUpcomingChanges(targetHash, initial)
	b := GetUpcomingChanges(mergingHash, initial)

	content, err := FormCommit(a, b, initial)

	if err != nil {
		log.Fatal(err)
		os.Exit(1)
	}
	commit := Commit{Parent: []string{targetHash, mergingHash}, Comment: comment, Content: content}
	return commit
}

func FormCommit(target, merged Changes, initial map[string]string) ([]string, error) {
	deleted := make(map[string]int)
	changedConflicts := make([]string, 0)
	addedConflicts := make([]string, 0)
	for _, line := range merged.Deleted {
		deleted[line] = 1
	}
	for _, line := range target.Deleted {
		deleted[line] = 1
	}

	changed := make(map[string]string)
	for _, line := range merged.Changed {
		file, hash := getEntryAndHash(line)
		changed[file] = hash

	}

	for _, line := range target.Changed {
		file, hash := getEntryAndHash(line)

		if value, ok := changed[file]; ok {
			if value == hash {
				log.Fatal("conflict")
				//continue
				//если файл конкурентно добавлен/изменен, ошибка. Как я понял, даже когда совпадает хеш

			}

			changedConflicts = append(changedConflicts, file)

		} else {
			changed[file] = hash

		}

	}

	added := make(map[string]string)
	for _, line := range merged.Added {
		file, hash := getEntryAndHash(line)
		added[file] = hash

	}
	for _, line := range target.Added {
		file, hash := getEntryAndHash(line)

		if value, ok := added[file]; ok {
			if value == hash {
				//continue
				//если файл конкурентно добавлен/изменен, ошибка. Как я понял, даже когда совпадает хеш

			}
			addedConflicts = append(addedConflicts, file)

		} else {
			added[file] = hash

		}
	}
	for value := range deleted {
		delete(initial, value)
	}

	for value := range changed {
		initial[value] = changed[value]
	}

	for value := range changed {
		initial[value] = added[value]
	}
	fmt.Println(len(changedConflicts))
	fmt.Println(addedConflicts)
	if len(addedConflicts)+len(changedConflicts) != 0 {
		return nil, errors.New(strings.Join(addedConflicts, "\n") + "\n---\n" + strings.Join(changedConflicts, "\n"))
	}
	final := make([]string, 0)
	for key := range initial {
		final = append(final, fmt.Sprintf("%s\t%s", key, initial[key]))
	}
	return final, nil
}

func GetUpcomingChanges(hash string, previous map[string]string) Changes {
	data, err := os.ReadFile(hash)
	if err != nil {
		os.Exit(1)
	}
	deleted := make([]string, 0)
	visited := make(map[string]int)

	changed := make([]string, 0)

	added := make([]string, 0)
	for _, line := range strings.Split(string(data), "\n") {
		if strings.HasPrefix(line, ".parent") || strings.HasPrefix(line, ".commit") || len(line) == 0 {
			continue
		}
		entry, hash := getEntryAndHash(line)
		if oldHash, ok := previous[entry]; ok {
			visited[entry] = 1
			if hash == oldHash {
				continue
			} else {
				changed = append(changed, fmt.Sprintf("%s\t%s", entry, hash))
			}
		} else {
			added = append(added, fmt.Sprintf("%s\t%s", entry, hash))
		}

	}

	for oldObject := range previous {
		if value, ok := previous[oldObject]; ok {
			continue
		} else {
			deleted = append(deleted, fmt.Sprintf("%s\t%s", oldObject, value))
		}
	}
	return Changes{Deleted: deleted, Changed: changed, Added: added}
}

func getEntryAndHash(line string) (string, string) {
	parts := strings.Split(line, "\t")
	return parts[0][0 : len(parts[0])-1], parts[1]
}

type Changes struct {
	Deleted []string
	Changed []string
	Added   []string
}
