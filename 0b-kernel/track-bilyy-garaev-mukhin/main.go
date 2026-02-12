package main

import (
	"bufio"
	"fmt"
	"os"
	"os/exec"
	"sort"
	"strconv"
	"strings"
	"text/tabwriter"
)

type FileStats struct {
	Path        string
	AccumLOC    int
	NetLOC      int
	SizeFirst   int64
	SizeLast    int64
	BinaryDelta int64
}

type DirStats struct {
	Dir         string
	Files       int
	AccumLOC    int64
	NetLOC      int64
	BinaryDelta int64
}

func main() {
	if len(os.Args) < 2 {
		fmt.Fprintf(os.Stderr, "usage: %s <repo-path>\n", os.Args[0])
		os.Exit(1)
	}
	repo := os.Args[1]

	fmt.Fprintln(os.Stderr, "finding root...")
	rootOut, err := runGit(repo, "rev-list", "--max-parents=0", "HEAD")
	if err != nil {
		fatal("root", err)
	}
	roots := strings.Fields(rootOut)
	if len(roots) == 0 {
		fatal("no root", fmt.Errorf("empty"))
	}
	root := roots[0]
	if len(root) > 12 {
		root = root[:12]
	}
	fmt.Fprintln(os.Stderr, "root:", root)

	fmt.Fprintln(os.Stderr, "accum loc...")
	accum := collectAccum(repo)
	fmt.Fprintln(os.Stderr, "net loc...")
	net := collectNet(repo, roots[0])
	fmt.Fprintln(os.Stderr, "sizes...")
	headSizes := collectSizes(repo, "HEAD")
	rootSizes := collectSizes(repo, roots[0])

	stats := merge(accum, net, headSizes, rootSizes)
	sort.Slice(stats, func(i, j int) bool { return stats[i].AccumLOC > stats[j].AccumLOC })

	printTop(stats, 30)
	printDirs(stats)
	printSummary(stats)
}

func collectAccum(repo string) map[string]int {
	cmd := exec.Command("git", "log", "--numstat", "--format=", "--no-renames")
	cmd.Dir = repo
	out, _ := cmd.StdoutPipe()
	cmd.Start()

	m := make(map[string]int)
	sc := bufio.NewScanner(out)
	buf := make([]byte, 0, 512*1024)
	sc.Buffer(buf, 1024*1024)
	for sc.Scan() {
		line := sc.Text()
		if line == "" {
			continue
		}
		add, del, path, ok := parseNumstat(line)
		if !ok {
			continue
		}
		m[path] += add + del
	}
	cmd.Wait()
	return m
}

func collectNet(repo, root string) map[string]int {
	cmd := exec.Command("git", "diff", "--numstat", "--no-renames", root, "HEAD")
	cmd.Dir = repo
	out, _ := cmd.StdoutPipe()
	cmd.Start()

	m := make(map[string]int)
	sc := bufio.NewScanner(out)
	for sc.Scan() {
		add, del, path, ok := parseNumstat(sc.Text())
		if !ok {
			continue
		}
		m[path] = add + del
	}
	cmd.Wait()
	return m
}

func collectSizes(repo, ref string) map[string]int64 {
	out, err := runGit(repo, "ls-tree", "-r", "-l", ref)
	if err != nil {
		fatal("ls-tree", err)
	}
	m := make(map[string]int64)
	sc := bufio.NewScanner(strings.NewReader(out))
	for sc.Scan() {
		line := sc.Text()
		i := strings.IndexByte(line, '\t')
		if i < 0 {
			continue
		}
		path := line[i+1:]
		f := strings.Fields(line[:i])
		if len(f) < 4 {
			continue
		}
		size, err := strconv.ParseInt(f[3], 10, 64)
		if err != nil {
			continue
		}
		m[path] = size
	}
	return m
}

func merge(accum, net map[string]int, headS, rootS map[string]int64) []FileStats {
	var list []FileStats
	for path, lastSize := range headS {
		firstSize := rootS[path]
		delta := lastSize - firstSize
		if delta < 0 {
			delta = -delta
		}
		list = append(list, FileStats{
			Path:        path,
			AccumLOC:    accum[path],
			NetLOC:      net[path],
			SizeFirst:   firstSize,
			SizeLast:    lastSize,
			BinaryDelta: delta,
		})
	}
	return list
}

func printTop(stats []FileStats, n int) {
	if n > len(stats) {
		n = len(stats)
	}
	fmt.Println()
	fmt.Println("--- top files by accum loc ---")
	tw := tabwriter.NewWriter(os.Stdout, 0, 4, 2, ' ', 0)
	fmt.Fprintln(tw, "file\taccum\tnet\tratio\tsize_first\tsize_last\tdelta")
	for i := 0; i < n; i++ {
		s := stats[i]
		path := s.Path
		if len(path) > 50 {
			path = "..." + path[len(path)-47:]
		}
		fmt.Fprintf(tw, "%s\t%d\t%d\t%s\t%s\t%s\t%s\n",
			path, s.AccumLOC, s.NetLOC, ratio(s.AccumLOC, s.NetLOC),
			bytesFmt(s.SizeFirst), bytesFmt(s.SizeLast), bytesFmt(s.BinaryDelta))
	}
	tw.Flush()
}

func printDirs(stats []FileStats) {
	byDir := make(map[string]*DirStats)
	for _, s := range stats {
		d := topDir(s.Path)
		if byDir[d] == nil {
			byDir[d] = &DirStats{Dir: d}
		}
		byDir[d].Files++
		byDir[d].AccumLOC += int64(s.AccumLOC)
		byDir[d].NetLOC += int64(s.NetLOC)
		byDir[d].BinaryDelta += s.BinaryDelta
	}
	var dirs []*DirStats
	for _, ds := range byDir {
		dirs = append(dirs, ds)
	}
	sort.Slice(dirs, func(i, j int) bool { return dirs[i].AccumLOC > dirs[j].AccumLOC })

	fmt.Println()
	fmt.Println("--- by directory ---")
	tw := tabwriter.NewWriter(os.Stdout, 0, 4, 2, ' ', 0)
	fmt.Fprintln(tw, "dir\tfiles\taccum\tnet\tratio\tdelta")
	for _, ds := range dirs {
		fmt.Fprintf(tw, "%s\t%d\t%d\t%d\t%s\t%s\n",
			ds.Dir, ds.Files, ds.AccumLOC, ds.NetLOC,
			ratio(int(ds.AccumLOC), int(ds.NetLOC)), bytesFmt(ds.BinaryDelta))
	}
	tw.Flush()
}

func printSummary(stats []FileStats) {
	var totalAccum, totalNet int64
	var totalDelta int64
	maxRatio := 0.0
	maxFile := ""

	for _, s := range stats {
		totalAccum += int64(s.AccumLOC)
		totalNet += int64(s.NetLOC)
		totalDelta += s.BinaryDelta
		if s.NetLOC > 0 {
			r := float64(s.AccumLOC) / float64(s.NetLOC)
			if r > maxRatio {
				maxRatio = r
				maxFile = s.Path
			}
		}
	}

	fmt.Println()
	fmt.Println("--- summary ---")
	fmt.Printf("files: %d\n", len(stats))
	fmt.Printf("accum loc: %d\n", totalAccum)
	fmt.Printf("net loc: %d\n", totalNet)
	fmt.Printf("ratio: %s\n", ratio(int(totalAccum), int(totalNet)))
	fmt.Printf("binary delta: %s\n", bytesFmt(totalDelta))
	if maxFile != "" {
		fmt.Printf("max ratio file: %s (%.1fx)\n", maxFile, maxRatio)
	}
}

func runGit(repo string, args ...string) (string, error) {
	cmd := exec.Command("git", args...)
	cmd.Dir = repo
	out, err := cmd.Output()
	if err != nil {
		return "", err
	}
	return string(out), nil
}

func parseNumstat(line string) (add, del int, path string, ok bool) {
	parts := strings.SplitN(line, "\t", 3)
	if len(parts) != 3 {
		return 0, 0, "", false
	}
	if parts[0] == "-" || parts[1] == "-" {
		return 0, 0, "", false
	}
	a, e1 := strconv.Atoi(parts[0])
	d, e2 := strconv.Atoi(parts[1])
	if e1 != nil || e2 != nil {
		return 0, 0, "", false
	}
	return a, d, parts[2], true
}

func topDir(path string) string {
	i := strings.IndexByte(path, '/')
	if i < 0 {
		return path
	}
	return path[:i]
}

func ratio(a, b int) string {
	if b == 0 {
		if a == 0 {
			return "-"
		}
		return "inf"
	}
	return fmt.Sprintf("%.2fx", float64(a)/float64(b))
}

func bytesFmt(n int64) string {
	if n >= 1<<30 {
		return fmt.Sprintf("%.1fG", float64(n)/(1<<30))
	}
	if n >= 1<<20 {
		return fmt.Sprintf("%.1fM", float64(n)/(1<<20))
	}
	if n >= 1<<10 {
		return fmt.Sprintf("%.1fK", float64(n)/(1<<10))
	}
	return fmt.Sprintf("%d", n)
}

func fatal(msg string, err error) {
	fmt.Fprintf(os.Stderr, "error %s: %v\n", msg, err)
	os.Exit(1)
}
