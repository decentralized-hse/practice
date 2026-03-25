# BASON Codec (Go)

## Build & test

```bash
cd go
go build ./cmd/basondump
go test ./...
```

Short tests (fewer fuzz iterations): `go test ./... -short`

## Fuzz & benchmark

```bash
go test ./internal/bason/... -fuzz=FuzzDecodeBasonAll -fuzztime=30s
go test ./internal/bason/... -fuzz=FuzzJsonRoundTrip -fuzztime=30s
go test ./internal/bason/... -bench=. -benchmem
```

## basondump

```bash
go run ./cmd/basondump --help
go run ./cmd/basondump --hex file.bason
go run ./cmd/basondump --json file.bason
go run ./cmd/basondump --validate=1FF file.bason
go run ./cmd/basondump --flatten in.bason > out.bason
go run ./cmd/basondump --unflatten in.bason > out.bason
```
