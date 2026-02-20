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
	Path               string
	AccumLOC           int
	NetLOC             int
	NetLOCRootMid      int
	NetLOCMidHead      int
	SizeFirst          int64
	SizeMid            int64
	SizeLast           int64
	BinaryDelta        int64
	BinaryDeltaRootMid int64
	BinaryDeltaMidHead int64
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

	mid := getMiddleCommit(repo, roots[0])
	if mid != "" {
		fmt.Fprintln(os.Stderr, "mid:", mid)
	}

	fmt.Fprintln(os.Stderr, "accum loc...")
	accum := collectAccum(repo)
	fmt.Fprintln(os.Stderr, "net loc root -> HEAD...")
	net := collectNetBetween(repo, roots[0], "HEAD")
	var netRootMid, netMidHead map[string]int
	if mid != "" {
		fmt.Fprintln(os.Stderr, "net loc root -> mid, mid -> HEAD...")
		netRootMid = collectNetBetween(repo, roots[0], mid)
		netMidHead = collectNetBetween(repo, mid, "HEAD")
	}
	fmt.Fprintln(os.Stderr, "sizes...")
	headSizes := collectSizes(repo, "HEAD")
	rootSizes := collectSizes(repo, roots[0])
	var midSizes map[string]int64
	if mid != "" {
		midSizes = collectSizes(repo, mid)
	}

	stats := merge(accum, net, netRootMid, netMidHead, headSizes, rootSizes, midSizes)
	sort.Slice(stats, func(i, j int) bool { return stats[i].AccumLOC > stats[j].AccumLOC })

	printTop(stats, 30)
	printDirs(stats)
	printDeltaVsLines(stats, 25)
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

func getMiddleCommit(repo, root string) string {
	out, err := runGit(repo, "rev-list", "--first-parent", root+"..HEAD")
	if err != nil || out == "" {
		return ""
	}
	commits := strings.Split(strings.TrimSpace(out), "\n")
	if len(commits) < 2 {
		return ""
	}
	mid := commits[len(commits)/2]
	if len(mid) > 12 {
		mid = mid[:12]
	}
	return mid
}

func collectNetBetween(repo, from, to string) map[string]int {
	cmd := exec.Command("git", "diff", "--numstat", "--no-renames", from, to)
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

func merge(accum, net map[string]int, netRootMid, netMidHead map[string]int, headS, rootS map[string]int64, midS map[string]int64) []FileStats {
	var list []FileStats
	for path, lastSize := range headS {
		firstSize := rootS[path]
		delta := lastSize - firstSize
		if delta < 0 {
			delta = -delta
		}
		midSize := int64(0)
		deltaRootMid := int64(0)
		deltaMidHead := int64(0)
		if midS != nil {
			midSize = midS[path]
			d1 := midSize - firstSize
			if d1 < 0 {
				d1 = -d1
			}
			deltaRootMid = d1
			d2 := lastSize - midSize
			if d2 < 0 {
				d2 = -d2
			}
			deltaMidHead = d2
		}
		list = append(list, FileStats{
			Path:               path,
			AccumLOC:           accum[path],
			NetLOC:             net[path],
			NetLOCRootMid:      at(netRootMid, path),
			NetLOCMidHead:      at(netMidHead, path),
			SizeFirst:          firstSize,
			SizeMid:            midSize,
			SizeLast:           lastSize,
			BinaryDelta:        delta,
			BinaryDeltaRootMid: deltaRootMid,
			BinaryDeltaMidHead: deltaMidHead,
		})
	}
	return list
}

func at(m map[string]int, key string) int {
	if m == nil {
		return 0
	}
	return m[key]
}

func printTop(stats []FileStats, n int) {
	if n > len(stats) {
		n = len(stats)
	}
	fmt.Println()
	fmt.Println("--- top files by accum loc ---")
	tw := tabwriter.NewWriter(os.Stdout, 0, 4, 2, ' ', 0)
	fmt.Fprintln(tw, "file\taccum\tnet\tratio\tsize_first\tsize_mid\tsize_last\tdelta\tdelta_mid\tbytes/line")
	for i := 0; i < n; i++ {
		s := stats[i]
		path := s.Path
		if len(path) > 50 {
			path = "..." + path[len(path)-47:]
		}
		fmt.Fprintf(tw, "%s\t%d\t%d\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
			path, s.AccumLOC, s.NetLOC, ratio(s.AccumLOC, s.NetLOC),
			bytesFmt(s.SizeFirst), bytesFmt(s.SizeMid), bytesFmt(s.SizeLast),
			bytesFmt(s.BinaryDelta), bytesFmt(s.BinaryDeltaMidHead),
			bytesPerLine(s.BinaryDelta, s.NetLOC))
	}
	tw.Flush()
}

func bytesPerLine(deltaBytes int64, netLOC int) string {
	if netLOC <= 0 {
		return "-"
	}
	return fmt.Sprintf("%.1f", float64(deltaBytes)/float64(netLOC))
}

func printDeltaVsLines(stats []FileStats, n int) {
	var withLOC []FileStats
	for _, s := range stats {
		if s.NetLOC > 0 {
			withLOC = append(withLOC, s)
		}
	}
	sort.Slice(withLOC, func(i, j int) bool {
		bi := float64(withLOC[i].BinaryDelta) / float64(withLOC[i].NetLOC)
		bj := float64(withLOC[j].BinaryDelta) / float64(withLOC[j].NetLOC)
		return bi > bj
	})
	if n > len(withLOC) {
		n = len(withLOC)
	}
	fmt.Println()
	fmt.Println("--- per-file: byte delta vs net LOC (bytes per line, top by B/L) ---")
	tw := tabwriter.NewWriter(os.Stdout, 0, 4, 2, ' ', 0)
	fmt.Fprintln(tw, "file\tnet_LOC\tdelta_bytes\tbytes/line")
	for i := 0; i < n; i++ {
		s := withLOC[i]
		path := s.Path
		if len(path) > 50 {
			path = "..." + path[len(path)-47:]
		}
		bpl := float64(s.BinaryDelta) / float64(s.NetLOC)
		fmt.Fprintf(tw, "%s\t%d\t%s\t%.1f\n", path, s.NetLOC, bytesFmt(s.BinaryDelta), bpl)
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
	var totalDelta, totalDeltaRootMid, totalDeltaMidHead int64
	var totalNetRootMid, totalNetMidHead int64
	maxRatio := 0.0
	maxFile := ""

	for _, s := range stats {
		totalAccum += int64(s.AccumLOC)
		totalNet += int64(s.NetLOC)
		totalDelta += s.BinaryDelta
		totalDeltaRootMid += s.BinaryDeltaRootMid
		totalDeltaMidHead += s.BinaryDeltaMidHead
		totalNetRootMid += int64(s.NetLOCRootMid)
		totalNetMidHead += int64(s.NetLOCMidHead)
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
	fmt.Printf("net loc (root -> HEAD): %d\n", totalNet)
	fmt.Printf("ratio: %s\n", ratio(int(totalAccum), int(totalNet)))
	fmt.Printf("binary delta (root -> HEAD): %s\n", bytesFmt(totalDelta))
	if totalNetRootMid > 0 || totalDeltaRootMid > 0 {
		fmt.Println()
		fmt.Println("--- middle point (root -> mid, mid -> HEAD) ---")
		fmt.Printf("net loc root -> mid: %d\n", totalNetRootMid)
		fmt.Printf("net loc mid -> HEAD: %d\n", totalNetMidHead)
		fmt.Printf("binary delta root -> mid: %s\n", bytesFmt(totalDeltaRootMid))
		fmt.Printf("binary delta mid -> HEAD: %s\n", bytesFmt(totalDeltaMidHead))
		if totalNetRootMid > 0 {
			fmt.Printf("bytes/line root -> mid: %.1f\n", float64(totalDeltaRootMid)/float64(totalNetRootMid))
		}
		if totalNetMidHead > 0 {
			fmt.Printf("bytes/line mid -> HEAD: %.1f\n", float64(totalDeltaMidHead)/float64(totalNetMidHead))
		}
	}
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
