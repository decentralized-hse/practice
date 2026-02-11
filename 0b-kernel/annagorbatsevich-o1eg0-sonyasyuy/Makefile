CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
PREFIX   ?= /usr/local

BIN      = diff-analyzer
SRC      = diff-analyzer.cpp

.PHONY: all clean install uninstall help test

all: $(BIN)

$(BIN): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $<

install: $(BIN)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(BIN)            $(DESTDIR)$(PREFIX)/bin/
	install -m 755 kernel-fetch.sh   $(DESTDIR)$(PREFIX)/bin/kernel-fetch

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(BIN)
	rm -f $(DESTDIR)$(PREFIX)/bin/kernel-fetch

clean:
	rm -f $(BIN)

# Quick smoke test with a mock numstat
test: $(BIN)
	@echo "--- smoke test (pipe mode) ---"
	@printf '150\t30\tdrivers/gpu/drm/foo.c\n\
80\t10\tdrivers/net/bar.c\n\
500\t200\tarch/x86/kernel/baz.c\n\
-\t-\tsome/binary.bin\n\
5\t1\tMakefile\n\
20\t3\tDocumentation/admin-guide/blah.rst\n' \
	| ./$(BIN) -d1 -
	@echo ""
	@echo "--- depth=2 test ---"
	@printf '150\t30\tdrivers/gpu/drm/foo.c\n\
80\t10\tdrivers/net/bar.c\n\
500\t200\tarch/x86/kernel/baz.c\n' \
	| ./$(BIN) -d2 -
	@echo ""
	@echo "All tests passed."

help:
	@echo "Targets:"
	@echo "  all       Build diff-analyzer (default)"
	@echo "  install   Install to $(PREFIX)/bin"
	@echo "  uninstall Remove from $(PREFIX)/bin"
	@echo "  test      Run smoke tests"
	@echo "  clean     Remove built binary"
	@echo ""
	@echo "Variables:"
	@echo "  CXX       C++ compiler   ($(CXX))"
	@echo "  PREFIX    Install prefix  ($(PREFIX))"