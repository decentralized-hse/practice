package main

import (
	"fmt"
	"log"
	"net/http"
	"strings"
)

// demoTree: fake paths for --demo.
var demoTree = map[string]string{
	"/README.md": "Demo (--demo): fake repo for Tarballer/Releaser.\n",
	"/src/a.txt": "file a\n",
	"/src/b.txt": "file b\n",
	"/docs/x.md": "# doc\n",
}

// demoDirs returns directory listing for a given path.
func demoDirs(dirPath string) string {
	if !strings.HasSuffix(dirPath, "/") {
		dirPath += "/"
	}

	seen := make(map[string]bool)
	var entries []string

	for filePath := range demoTree {
		if !strings.HasPrefix(filePath, dirPath) && dirPath != "/" {
			continue
		}

		rel := strings.TrimPrefix(filePath, dirPath)
		if dirPath == "/" {
			rel = strings.TrimPrefix(filePath, "/")
		}
		if rel == "" {
			continue
		}

		// If there's a slash, it's a subdirectory — take only the first component
		if idx := strings.Index(rel, "/"); idx != -1 {
			dirName := rel[:idx+1]
			if !seen[dirName] {
				seen[dirName] = true
				entries = append(entries, dirName)
			}
		} else {
			if !seen[rel] {
				seen[rel] = true
				entries = append(entries, rel)
			}
		}
	}

	return strings.Join(entries, "\n") + "\n"
}

func startDemoServer(addr string) {
	mux := http.NewServeMux()
	mux.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		p := r.URL.Path

		// Directory listing
		if strings.HasSuffix(p, "/") {
			listing := demoDirs(p)
			if listing == "\n" {
				http.Error(w, "not found", 404)
				return
			}
			w.Header().Set("Content-Type", "text/plain")
			fmt.Fprint(w, listing)
			return
		}

		// File content
		content, ok := demoTree[p]
		if !ok {
			http.Error(w, "not found", 404)
			return
		}
		w.Header().Set("Content-Type", "text/plain")
		w.Header().Set("Content-Length", fmt.Sprintf("%d", len(content)))
		fmt.Fprint(w, content)
	})

	if err := http.ListenAndServe(addr, mux); err != nil {
		log.Fatalf("demo server: %v", err)
	}
}
