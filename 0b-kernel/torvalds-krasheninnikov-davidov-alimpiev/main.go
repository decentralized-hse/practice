package main

import (
	"bufio"
	"bytes"
	"encoding/csv"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"regexp"
	"sort"
	"strings"
	"sync"
	"sync/atomic"
	"time"
)

var (
	// Folders to IGNORE (Regex applied to relative path)
	ignoreRegex = regexp.MustCompile(`^(\.git|Documentation|LICENSES)`)

	// Author to search for (Case sensitive usually, but we can lower it if needed)
	targetAuthor = "Linus Torvalds"
	//targetAuthor = "Mikhail Krasheninnikov"

	// Output file name
	outputFilename = "linus_4_15.csv"

	snapshotDate = "28-01-2018"

	repoPath = "/Users/coww/Projects/linux"

	workers = 8
)

type FileResult struct {
	RelPath    string
	LinusLines int
	TotalLines int
	Timestamp  string
}

func main() {
	// 1. Validation
	absRepoPath, err := filepath.Abs(repoPath)
	if err != nil {
		panic(err)
	}

	start := time.Now()
	fmt.Printf("Mining repo at: %s\n", absRepoPath)
	fmt.Printf("Workers: %d | Target: %s\n", workers, targetAuthor)

	// We pass the RELATIVE path to the jobs channel
	jobs := make(chan string, 1000)
	results := make(chan FileResult, 1000)

	var processedCount uint64
	stopMonitor := make(chan bool)

	go func() {
		ticker := time.NewTicker(2 * time.Second)
		defer ticker.Stop()
		lastCount := uint64(0)

		for {
			select {
			case <-stopMonitor:
				return
			case <-ticker.C:
				current := atomic.LoadUint64(&processedCount)
				diff := current - lastCount
				rate := float64(diff) / 2.0 // files per second
				fmt.Printf("\rProcessed: %d files | Speed: %.1f files/sec | Last: %s", current, rate, time.Now().Format("15:04:05"))
				lastCount = current
			}
		}
	}()

	// 2. Start Workers
	var wg sync.WaitGroup
	for w := 0; w < workers; w++ {
		wg.Add(1)
		// We pass absRepoPath so the worker knows where to run the git command
		go worker(absRepoPath, jobs, results, &wg, &processedCount)
	}

	// 3. Start Result Collector
	var collectedData []FileResult
	resultCollectorDone := make(chan bool)
	go func() {
		for res := range results {
			if res.TotalLines > 0 {
				collectedData = append(collectedData, res)
			}
		}
		resultCollectorDone <- true
	}()

	// 4. Walk the File Tree
	go func() {
		err := filepath.Walk(absRepoPath, func(path string, info os.FileInfo, err error) error {
			if err != nil {
				fmt.Printf("path: '%s', error: '%s'\n", path, err.Error())
				return nil
			}

			// Calculate path relative to the repo root
			relPath, err := filepath.Rel(absRepoPath, path)
			if err != nil {
				return nil
			}

			if info.IsDir() {
				// Apply ignore regex to the clean relative path
				if ignoreRegex.MatchString(relPath) && relPath != "." {
					return filepath.SkipDir
				}
				return nil
			}

			// Push relative path to job queue
			jobs <- relPath
			return nil
		})
		if err != nil {
			fmt.Println("Error walking tree:", err)
		}
		close(jobs)
	}()

	// 5.1. Wait for the stop of workers
	wg.Wait()
	// 5.2. Close results since workers have stopped
	close(results)
	// 5.3. Wait until result collector moves results from channel to slice
	<-resultCollectorDone

	// 6. Sort and Save
	fmt.Println("Sorting data...")
	sort.Slice(collectedData, func(i, j int) bool {
		return collectedData[i].RelPath < collectedData[j].RelPath
	})

	writeCSV(collectedData)

	fmt.Printf("Done! Processed %d files in %s\n", len(collectedData), time.Since(start))
}

// worker sends FileResult to results if Linus Torvalds has lines in the handled file
func worker(repoRoot string, jobs <-chan string, results chan<- FileResult, wg *sync.WaitGroup, counter *uint64) {
	defer wg.Done()

	for relPath := range jobs {
		linus, total := blameFile(repoRoot, relPath)

		if linus > 0 {
			results <- FileResult{
				RelPath:    relPath,
				LinusLines: linus,
				TotalLines: total,
				Timestamp:  snapshotDate,
			}
		}

		atomic.AddUint64(counter, 1)
	}
}

func blameFile(repoRoot, relPath string) (int, int) {
	cmd := exec.Command("git", "blame", "--line-porcelain", relPath)
	cmd.Dir = repoRoot

	var out bytes.Buffer
	cmd.Stdout = &out

	// Ignore errors (binary files, submodules, etc)
	if err := cmd.Run(); err != nil {
		fmt.Printf("file: '%s', err while blaming: '%s'\n", relPath, err.Error())
		return 0, 0
	}

	scanner := bufio.NewScanner(&out)
	linusCount := 0
	totalCount := 0

	for scanner.Scan() {
		line := scanner.Text()

		// Optimization: Check prefix first
		if len(line) > 7 && line[0:7] == "author " {
			totalCount++
			if strings.Contains(line, targetAuthor) {
				linusCount++
			}
		}
	}

	return linusCount, totalCount
}

func writeCSV(data []FileResult) {
	file, err := os.Create(outputFilename)
	if err != nil {
		panic(err)
	}
	defer func() {
		if err := file.Close(); err != nil {
			fmt.Printf("error while closing final csv: %s\n", err.Error())
		}
	}()

	writer := csv.NewWriter(file)
	defer writer.Flush()

	if err := writer.Write([]string{"snapshot_date", "file_path", "linus_lines", "total_lines"}); err != nil {
		fmt.Printf("error while printing first line in csv: %s\n", err.Error())
	}

	for _, row := range data {
		err := writer.Write([]string{
			row.Timestamp,
			row.RelPath,
			fmt.Sprintf("%d", row.LinusLines),
			fmt.Sprintf("%d", row.TotalLines),
		})
		if err != nil {
			fmt.Printf("error while printing line in csv: %s\n", err.Error())
		}
	}
}
