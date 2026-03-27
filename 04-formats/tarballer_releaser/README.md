# Beagle Tarballer / Releaser

Web application for release management and tarball packaging on top of [Beagle SCM](https://github.com/gritzko/librdx).

Beagle (`be`) is an AST-aware version control system. Its HTTP server (`be-srv`) exposes repository content via a REST-like API. This application wraps that API and adds:

- **Releaser** — named versions (e.g. `v1.2.3`) with descriptions and Beagle version formulas
- **Tarballer** — on-demand `.tar.gz` archives of any release or arbitrary path/version subset
- **File Browser** — web UI to browse repo files at any branch/version

**This app does not create or modify project files inside the Beagle repository.** It only reads the tree via `be-srv`. Release names and settings are stored locally in `releases.json`. To change repo content, use the Beagle CLI (`be`, `be post`, etc.).

## Documentation

The main setup guide — where to get **`be`** and **`be-srv`**, clone this repo, prepare a worktree, run **`be-srv`** and the web UI — is **[docs/MANUAL_UI_SCENARIO.md](docs/MANUAL_UI_SCENARIO.md)**.

If `be-srv` prints **`ROCKFAIL`**, try **section B2** in that document first.

## Architecture

```
Browser  ──HTTP──▶  tarballer-releaser (:8888)
                         │
                         ├──GET /path   ──▶  be-srv (:8080)  ──▶  RocksDB
                         ├──GET /path/  ──▶  be-srv
                         └──read/write  ──▶  releases.json   (local metadata only)
```

The app is a Go HTTP server that:
1. Proxies file browsing requests to `be-srv` (read-only from the repo’s point of view)
2. Stores release metadata in a local JSON file
3. Generates tar.gz archives on-the-fly by walking `be-srv` directory listings

## Build

```bash
go build -o tarballer-releaser .
```

Requirements: **Go 1.22+**. No external Go module dependencies (stdlib only).

## Run

### With Beagle

**Option A — start `be-srv` yourself** (two terminals):

```bash
# In your Beagle worktree (must contain .be file)
be-srv 8080

# Elsewhere
./tarballer-releaser --beagle http://127.0.0.1:8080 --listen :8888
```

**Option B — one command** (this app spawns `be-srv` in the worktree):

```bash
./tarballer-releaser --worktree /path/to/beagle/worktree --beagle-port 8080
```

The worktree must contain a `.be` file. On exit (Ctrl+C) the child `be-srv` is stopped.

The HTTP contract of `be-srv` is documented in [librdx `be/HTTP.md`](https://github.com/gritzko/librdx/blob/master/be/HTTP.md) (DIR `GET /path/`, RAW `GET /path/file`, optional `?formula`).

### Demo Mode (no Beagle needed)

A built-in demo file server simulates a Beagle repository:

```bash
./tarballer-releaser --demo
```

Open http://localhost:8888 in your browser.

## CLI Flags

| Flag | Default | Description |
|------|---------|-------------|
| `--listen` | `:8888` | Address to listen on |
| `--beagle` | `http://127.0.0.1:8080` | `be-srv` base URL (when `--worktree` is not set) |
| `--worktree` | *(empty)* | Beagle worktree with `.be`; runs `be-srv` on `--beagle-port` |
| `--beagle-bin` | `be-srv` | `be-srv` executable (with `--worktree`) |
| `--beagle-port` | `8080` | TCP port for spawned `be-srv` (with `--worktree`) |
| `--skip-beagle-check` | `false` | Do not wait for `be-srv` before serving |
| `--store` | `releases.json` | Path to release metadata file |
| `--demo` | `false` | Start built-in demo file server |
| `--demo-port` | `9999` | Port for demo file server |

## Features

### Release Management

- **Create** releases with name, description, version formula, and path prefix
- **List** all releases sorted by creation date
- **Browse** files at a release's pinned version
- **Download** release as `.tar.gz`
- **Delete** releases

### Tarball Generation

- Streams `.tar.gz` directly — no temp files
- Scoped by path prefix (e.g. only `/src/`)
- Scoped by Beagle formula (branch/version selector)
- Works for named releases and ad-hoc downloads

### File Browser

- Directory listings with breadcrumb navigation
- File view with line numbers
- Version formula selector to browse at any branch/point in time

## API Endpoints

| Method | Path | Description |
|--------|------|-------------|
| GET | `/` | Home page |
| GET | `/browse/*path` | Browse directory or file |
| GET | `/releases` | List releases |
| GET | `/releases/new` | Create release form |
| POST | `/releases` | Create release |
| GET | `/releases/{name}` | Release detail |
| POST | `/releases/{name}/delete` | Delete release |
| GET | `/releases/{name}/tarball` | Download release tarball |
| GET | `/releases/{name}/browse/*path` | Browse at release version |
| GET | `/tarball?prefix=&formula=&name=` | Ad-hoc tarball |

## Beagle Version Formulas

The `formula` field in releases maps to Beagle's `?query` parameter:

| Formula | Meaning |
|---------|---------|
| `main` | All waypoints on branch `main` |
| `main&feat` | Merge of `main` and `feat` branches |
| `stamp-main` | Branch `main` up to timestamp `stamp` |
| `stamp+main` | Branch `main` after timestamp `stamp` |
| *(empty)* | All branches (default from `.be` file) |

## Project Structure

```
main.go       — entry point, routing, handlers
beagle.go     — HTTP client for be-srv (ListDir, GetFile, Walk)
besrv.go      — optional: spawn be-srv in a worktree, wait until ready
tarball.go    — streaming tar.gz writer
release.go    — release CRUD with JSON persistence
demo.go       — built-in demo file server
beagle_test.go — unit tests for the Beagle HTTP client
templates/    — HTML templates (embedded)
static/       — CSS (embedded)
docs/         — manual scenario (see MANUAL_UI_SCENARIO.md)
```
