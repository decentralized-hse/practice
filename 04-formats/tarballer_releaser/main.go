package main

import (

	"context"
	"embed"
	"flag"
	"fmt"
	"html/template"
	"log"
	"net/http"

	"os"
	"os/exec"
	"os/signal"
	"path"
	"path/filepath"
	"strings"
	"syscall"
	"time"
)

//go:embed templates/*.html
var templateFS embed.FS

//go:embed static/*
var staticFS embed.FS

var templates map[string]*template.Template

type App struct {
	Client  *BeagleClient
	Store   *ReleaseStore
	BaseURL string
}

// Breadcrumb for navigation in the file browser.
type Breadcrumb struct {
	Name string
	Path string
}

func makeBreadcrumbs(p string) []Breadcrumb {
	p = strings.Trim(p, "/")
	if p == "" {
		return nil
	}
	parts := strings.Split(p, "/")
	crumbs := make([]Breadcrumb, len(parts))
	for i, part := range parts {
		crumbs[i] = Breadcrumb{
			Name: part,
			Path: "/" + strings.Join(parts[:i+1], "/"),
		}
	}
	return crumbs
}

// --- Handlers ---

func (a *App) handleIndex(w http.ResponseWriter, r *http.Request) {
	if r.URL.Path != "/" {
		http.NotFound(w, r)
		return
	}
	releases := a.Store.List()
	data := map[string]any{
		"Releases":  releases,
		"BeagleURL": a.Client.BaseURL,
	}
	renderTemplate(w, "index.html", data)
}

func (a *App) handleBrowse(w http.ResponseWriter, r *http.Request) {
	browsePath := strings.TrimPrefix(r.URL.Path, "/browse")
	if browsePath == "" {
		browsePath = "/"
	}
	formula := r.URL.Query().Get("formula")

	isDir := browsePath == "/" || strings.HasSuffix(browsePath, "/")

	if isDir {
		entries, err := a.Client.ListDir(browsePath, formula)
		if err != nil {
			renderError(w, "Cannot browse directory", err.Error(), http.StatusBadGateway)
			return
		}
		data := map[string]any{
			"Path":        browsePath,
			"Formula":     formula,
			"Entries":     entries,
			"Breadcrumbs": makeBreadcrumbs(browsePath),
			"IsDir":       true,
		}
		renderTemplate(w, "browse.html", data)
		return
	}

	// Could be a file or a dir without trailing slash — try file first
	content, err := a.Client.GetFile(browsePath, formula)
	if err != nil {
		// Maybe it's a directory without trailing slash
		entries, dirErr := a.Client.ListDir(browsePath+"/", formula)
		if dirErr == nil {
			data := map[string]any{
				"Path":        browsePath + "/",
				"Formula":     formula,
				"Entries":     entries,
				"Breadcrumbs": makeBreadcrumbs(browsePath),
				"IsDir":       true,
			}
			renderTemplate(w, "browse.html", data)
			return
		}
		renderError(w, "Cannot fetch file", err.Error(), http.StatusBadGateway)
		return
	}

	lines := strings.Split(string(content), "\n")
	data := map[string]any{
		"Path":        browsePath,
		"Formula":     formula,
		"FileName":    path.Base(browsePath),
		"Content":     string(content),
		"Lines":       lines,
		"Breadcrumbs": makeBreadcrumbs(browsePath),
		"IsDir":       false,
	}
	renderTemplate(w, "file.html", data)
}

func (a *App) handleReleases(w http.ResponseWriter, r *http.Request) {
	data := map[string]any{
		"Releases": a.Store.List(),
	}
	renderTemplate(w, "releases.html", data)
}

func (a *App) handleReleaseNew(w http.ResponseWriter, r *http.Request) {
	renderTemplate(w, "release_new.html", nil)
}

func (a *App) handleReleaseCreate(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}
	if err := r.ParseForm(); err != nil {
		renderError(w, "Bad request", err.Error(), http.StatusBadRequest)
		return
	}

	rel := Release{
		Name:        strings.TrimSpace(r.FormValue("name")),
		Description: strings.TrimSpace(r.FormValue("description")),
		Formula:     strings.TrimSpace(r.FormValue("formula")),
		Prefix:      strings.TrimSpace(r.FormValue("prefix")),
	}
	if rel.Prefix == "" {
		rel.Prefix = "/"
	}

	if err := a.Store.Create(rel); err != nil {
		renderError(w, "Cannot create release", err.Error(), http.StatusBadRequest)
		return
	}
	http.Redirect(w, r, "/releases/"+rel.Name, http.StatusSeeOther)
}

func (a *App) handleReleaseDetail(w http.ResponseWriter, r *http.Request) {
	name := extractReleaseName(r.URL.Path, "/releases/")
	// Strip any trailing sub-path
	if idx := strings.Index(name, "/"); idx != -1 {
		name = name[:idx]
	}

	rel, ok := a.Store.Get(name)
	if !ok {
		renderError(w, "Release not found", fmt.Sprintf("No release named %q", name), http.StatusNotFound)
		return
	}
	data := map[string]any{
		"Release": rel,
	}
	renderTemplate(w, "release_detail.html", data)
}

func (a *App) handleReleaseDelete(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}
	name := extractReleaseName(r.URL.Path, "/releases/")
	name = strings.TrimSuffix(name, "/delete")

	if err := a.Store.Delete(name); err != nil {
		renderError(w, "Cannot delete release", err.Error(), http.StatusNotFound)
		return
	}
	http.Redirect(w, r, "/releases", http.StatusSeeOther)
}

func (a *App) handleReleaseTarball(w http.ResponseWriter, r *http.Request) {
	name := extractReleaseName(r.URL.Path, "/releases/")
	name = strings.TrimSuffix(name, "/tarball")

	rel, ok := a.Store.Get(name)
	if !ok {
		http.Error(w, "Release not found", http.StatusNotFound)
		return
	}

	archiveName := rel.Name
	w.Header().Set("Content-Type", "application/gzip")
	w.Header().Set("Content-Disposition", fmt.Sprintf(`attachment; filename="%s.tar.gz"`, archiveName))

	if err := WriteTarball(w, a.Client, rel.Prefix, rel.Formula, archiveName); err != nil {
		log.Printf("tarball error for release %s: %v", name, err)
	}
}

func (a *App) handleReleaseBrowse(w http.ResponseWriter, r *http.Request) {
	// /releases/{name}/browse/...
	rest := extractReleaseName(r.URL.Path, "/releases/")
	parts := strings.SplitN(rest, "/browse", 2)
	if len(parts) < 1 {
		http.NotFound(w, r)
		return
	}
	name := parts[0]
	browsePath := "/"
	if len(parts) == 2 {
		browsePath = parts[1]
		if browsePath == "" {
			browsePath = "/"
		}
	}

	rel, ok := a.Store.Get(name)
	if !ok {
		renderError(w, "Release not found", fmt.Sprintf("No release named %q", name), http.StatusNotFound)
		return
	}

	// Rewrite the request to the browse handler with the release's formula
	q := r.URL.Query()
	q.Set("formula", rel.Formula)
	r.URL.RawQuery = q.Encode()
	r.URL.Path = "/browse" + browsePath

	a.handleBrowse(w, r)
}

func (a *App) handleAdHocTarball(w http.ResponseWriter, r *http.Request) {
	prefix := r.URL.Query().Get("prefix")
	formula := r.URL.Query().Get("formula")
	name := r.URL.Query().Get("name")
	if prefix == "" {
		prefix = "/"
	}
	if name == "" {
		name = "archive"
	}

	w.Header().Set("Content-Type", "application/gzip")
	w.Header().Set("Content-Disposition", fmt.Sprintf(`attachment; filename="%s.tar.gz"`, name))

	if err := WriteTarball(w, a.Client, prefix, formula, name); err != nil {
		log.Printf("ad-hoc tarball error: %v", err)
	}
}

// --- Routing ---

func (a *App) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	p := r.URL.Path

	switch {
	case p == "/":
		a.handleIndex(w, r)
	case strings.HasPrefix(p, "/browse"):
		a.handleBrowse(w, r)
	case p == "/releases/new":
		a.handleReleaseNew(w, r)
	case p == "/releases" && r.Method == http.MethodPost:
		a.handleReleaseCreate(w, r)
	case p == "/releases" || p == "/releases/":
		a.handleReleases(w, r)
	case strings.HasSuffix(p, "/tarball") && strings.HasPrefix(p, "/releases/"):
		a.handleReleaseTarball(w, r)
	case strings.HasSuffix(p, "/delete") && strings.HasPrefix(p, "/releases/") && r.Method == http.MethodPost:
		a.handleReleaseDelete(w, r)
	case strings.Contains(p, "/browse") && strings.HasPrefix(p, "/releases/"):
		a.handleReleaseBrowse(w, r)
	case strings.HasPrefix(p, "/releases/"):
		a.handleReleaseDetail(w, r)
	case p == "/tarball":
		a.handleAdHocTarball(w, r)
	case strings.HasPrefix(p, "/static/"):
		http.FileServer(http.FS(staticFS)).ServeHTTP(w, r)
	default:
		http.NotFound(w, r)
	}
}

// --- Helpers ---

func extractReleaseName(urlPath, prefix string) string {
	return strings.TrimPrefix(urlPath, prefix)
}

func loadTemplates(funcMap template.FuncMap) map[string]*template.Template {
	base := template.Must(
		template.New("layout").Funcs(funcMap).ParseFS(templateFS, "templates/layout.html"),
	)
	pages := []string{
		"index.html", "browse.html", "file.html",
		"releases.html", "release_new.html", "release_detail.html",
		"error.html",
	}
	result := make(map[string]*template.Template, len(pages))
	for _, page := range pages {
		clone := template.Must(base.Clone())
		t := template.Must(clone.ParseFS(templateFS, "templates/"+page))
		result[page] = t
	}
	return result
}

func renderTemplate(w http.ResponseWriter, name string, data any) {
	t, ok := templates[name]
	if !ok {
		log.Printf("template %q not found", name)
		http.Error(w, "Internal server error", http.StatusInternalServerError)
		return
	}
	w.Header().Set("Content-Type", "text/html; charset=utf-8")
	if err := t.ExecuteTemplate(w, "layout", data); err != nil {
		log.Printf("template error (%s): %v", name, err)
		http.Error(w, "Internal server error", http.StatusInternalServerError)
	}
}

func renderError(w http.ResponseWriter, title, detail string, code int) {
	w.Header().Set("Content-Type", "text/html; charset=utf-8")
	w.WriteHeader(code)
	data := map[string]any{
		"Title":  title,
		"Detail": detail,
		"Code":   code,
	}
	t, ok := templates["error.html"]
	if !ok {
		fmt.Fprintf(w, "<h1>%s</h1><p>%s</p>", title, detail)
		return
	}
	if err := t.ExecuteTemplate(w, "layout", data); err != nil {
		log.Printf("error template failed: %v", err)
		fmt.Fprintf(w, "<h1>%s</h1><p>%s</p>", title, detail)
	}
}

// --- Main ---

func main() {
	listenAddr := flag.String("listen", ":8888", "Address to listen on")

	beagleURL := flag.String("beagle", "http://127.0.0.1:8080", "be-srv base URL (when -worktree is not used)")
	storeFile := flag.String("store", "releases.json", "Path to releases JSON file")
	demo := flag.Bool("demo", false, "Start built-in demo file server instead of connecting to be-srv")
	demoPort := flag.Int("demo-port", 9999, "Port for the demo file server")
	worktree := flag.String("worktree", "", "Beagle worktree (directory with .be); starts be-srv on -beagle-port")
	beagleBin := flag.String("beagle-bin", "be-srv", "Path to be-srv (used with -worktree)")
	beaglePort := flag.Int("beagle-port", 8080, "TCP port for be-srv (used with -worktree)")
	skipBeagleCheck := flag.Bool("skip-beagle-check", false, "Do not wait for be-srv at startup")
	flag.Parse()

	funcMap := template.FuncMap{
		"add":       func(a, b int) int { return a + b },
		"trimSlash": func(s string) string { return strings.Trim(s, "/") },
		"hasPrefix": strings.HasPrefix,
	}

	templates = loadTemplates(funcMap)


	var besrvCmd *exec.Cmd
	actualBeagleURL := *beagleURL

	switch {
	case *demo:
		demoAddr := fmt.Sprintf(":%d", *demoPort)
		actualBeagleURL = fmt.Sprintf("http://127.0.0.1:%d", *demoPort)
		go startDemoServer(demoAddr)
		log.Printf("Demo server simulating be-srv at %s", actualBeagleURL)
	case *worktree != "":
		wd, err := filepath.Abs(*worktree)
		if err != nil {
			log.Fatalf("worktree path: %v", err)
		}
		cmd, err := startBeSrv(*beagleBin, wd, *beaglePort)
		if err != nil {
			log.Fatalf("be-srv: %v", err)
		}
		besrvCmd = cmd
		actualBeagleURL = fmt.Sprintf("http://127.0.0.1:%d", *beaglePort)
		log.Printf("Started be-srv in %s (pid %d) on port %d → %s", wd, cmd.Process.Pid, *beaglePort, actualBeagleURL)
	default:
		// use -beagle URL as given
	}

	client := NewBeagleClient(actualBeagleURL)
	if !*skipBeagleCheck {
		if err := waitForBeagle(client, 45*time.Second); err != nil {
			if besrvCmd != nil && besrvCmd.Process != nil {
				_ = besrvCmd.Process.Kill()
			}
			log.Fatalf("Beagle backend: %v", err)
		}
		log.Printf("be-srv reachable at %s", actualBeagleURL)
	}

	store, err := NewReleaseStore(*storeFile)
	if err != nil {
		if besrvCmd != nil && besrvCmd.Process != nil {
			_ = besrvCmd.Process.Kill()
		}
		log.Fatalf("Failed to init release store: %v", err)
	}

	app := &App{
		Client:  client,
		Store:   store,

		BaseURL: actualBeagleURL,
	}

	srv := &http.Server{
		Addr:    *listenAddr,
		Handler: app,
	}

	go func() {
		log.Printf("Tarballer/Releaser listening on %s (be-srv: %s)", *listenAddr, actualBeagleURL)
		if err := srv.ListenAndServe(); err != nil && err != http.ErrServerClosed {
			log.Fatal(err)
		}
	}()

	sigCh := make(chan os.Signal, 1)
	signal.Notify(sigCh, syscall.SIGINT, syscall.SIGTERM)
	<-sigCh
	signal.Stop(sigCh)
	log.Println("shutting down…")

	if besrvCmd != nil && besrvCmd.Process != nil {
		_ = besrvCmd.Process.Kill()
		_ = besrvCmd.Wait()
	}

	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()
	if err := srv.Shutdown(ctx); err != nil {
		log.Printf("HTTP shutdown: %v", err)
	}
}
