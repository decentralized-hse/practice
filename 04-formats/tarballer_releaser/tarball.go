package main

import (
	"archive/tar"
	"compress/gzip"
	"fmt"
	"io"
	"path"
	"strings"
	"time"
)

// WriteTarball streams a tar.gz archive of files from be-srv into w.
// archiveName is used as the top-level directory inside the archive.
func WriteTarball(w io.Writer, client *BeagleClient, prefix, formula, archiveName string) error {
	gw, err := gzip.NewWriterLevel(w, gzip.BestSpeed)
	if err != nil {
		return fmt.Errorf("gzip init: %w", err)
	}
	defer gw.Close()

	tw := tar.NewWriter(gw)
	defer tw.Close()

	now := time.Now()

	dirsWritten := make(map[string]bool)

	return client.Walk(prefix, formula, func(filePath string, content []byte) error {
		// Strip prefix to make paths relative inside the archive
		relPath := strings.TrimPrefix(filePath, prefix)
		relPath = strings.TrimLeft(relPath, "/")
		if relPath == "" {
			return nil
		}

		fullArchivePath := relPath
		if archiveName != "" {
			fullArchivePath = archiveName + "/" + relPath
		}

		// Ensure parent directories exist in the archive
		dir := path.Dir(fullArchivePath)
		parts := strings.Split(dir, "/")
		for i := range parts {
			d := strings.Join(parts[:i+1], "/") + "/"
			if d != "./" && !dirsWritten[d] {
				dirsWritten[d] = true
				err := tw.WriteHeader(&tar.Header{
					Typeflag: tar.TypeDir,
					Name:     d,
					Mode:     0o755,
					ModTime:  now,
				})
				if err != nil {
					return err
				}
			}
		}

		hdr := &tar.Header{
			Typeflag: tar.TypeReg,
			Name:     fullArchivePath,
			Size:     int64(len(content)),
			Mode:     0o644,
			ModTime:  now,
		}
		if err := tw.WriteHeader(hdr); err != nil {
			return fmt.Errorf("tar header for %s: %w", fullArchivePath, err)
		}
		if _, err := tw.Write(content); err != nil {
			return fmt.Errorf("tar write for %s: %w", fullArchivePath, err)
		}
		return nil
	})
}
