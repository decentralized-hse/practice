# 01-git get written in Zig

Requires Zig version 0.11.0.

Build:
```bash
zig build-exe -O ReleaseSafe --name get-jilavyan main.zig
```
omit `-O ReleaseSafe` for debug build.

Usage:
```bash
./get-jilavyan <path> <root_hash>
```
