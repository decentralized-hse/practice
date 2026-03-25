package main

import (
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"
)

func TestBeagleClient_buildURL_paths(t *testing.T) {
	c := NewBeagleClient("http://127.0.0.1:8080")

	tests := []struct {
		path    string
		formula string
		want    string
	}{
		{"/", "", "http://127.0.0.1:8080/"},
		{"/src/", "", "http://127.0.0.1:8080/src/"},
		{"/README.md", "", "http://127.0.0.1:8080/README.md"},
		{"/README.md", "main", "http://127.0.0.1:8080/README.md?main"},
		{"/src/", "main&feat", "http://127.0.0.1:8080/src/?main%26feat"},
	}
	for _, tt := range tests {
		got := c.buildURL(tt.path, tt.formula, "")
		if got != tt.want {
			t.Errorf("buildURL(%q,%q) = %q, want %q", tt.path, tt.formula, got, tt.want)
		}
	}
}

func TestBeagleClient_ListDir_and_GetFile(t *testing.T) {
	mux := http.NewServeMux()
	mux.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		if r.URL.Path != "/" {
			http.NotFound(w, r)
			return
		}
		w.Header().Set("Content-Type", "text/plain")
		_, _ = w.Write([]byte("src/\nREADME.md\n"))
	})
	mux.HandleFunc("/src/", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "text/plain")
		_, _ = w.Write([]byte("main.c\n"))
	})
	mux.HandleFunc("/src/main.c", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "text/plain")
		_, _ = w.Write([]byte("int main(){}\n"))
	})

	ts := httptest.NewServer(mux)
	defer ts.Close()

	c := NewBeagleClient(ts.URL)

	ents, err := c.ListDir("/", "")
	if err != nil {
		t.Fatal(err)
	}
	if len(ents) != 2 {
		t.Fatalf("ListDir /: got %d entries, want 2", len(ents))
	}
	if !ents[0].IsDir || ents[0].Name != "src/" {
		t.Errorf("entry 0: %+v", ents[0])
	}
	if ents[1].IsDir || ents[1].Name != "README.md" {
		t.Errorf("entry 1: %+v", ents[1])
	}

	b, err := c.GetFile("/src/main.c", "")
	if err != nil {
		t.Fatal(err)
	}
	if strings.TrimSpace(string(b)) != "int main(){}" {
		t.Fatalf("GetFile: %q", b)
	}
}

func TestBeagleClient_Ping(t *testing.T) {
	ts := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.URL.Path != "/" {
			http.NotFound(w, r)
			return
		}
		w.WriteHeader(http.StatusOK)
		_, _ = w.Write([]byte("ok\n"))
	}))
	defer ts.Close()

	c := NewBeagleClient(ts.URL)
	if err := c.Ping(); err != nil {
		t.Fatal(err)
	}
}

func TestBeagleClient_Ping_nonOK(t *testing.T) {
	ts := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		http.Error(w, "nope", http.StatusInternalServerError)
	}))
	defer ts.Close()

	c := NewBeagleClient(ts.URL)
	if err := c.Ping(); err == nil {
		t.Fatal("expected error on 500")
	}
}
