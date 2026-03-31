# secure_sync — Merkle-tree file integrity checker

`secure_sync` builds a **Merkle tree** over a directory, writes the root hash
to a manifest file, and signs it with an **Ed25519 private key**.  On the
receiving end it re-hashes the directory, verifies the Ed25519 signature, and
compares the Merkle root — detecting any tampering or corruption.

## Artifact layout

| File | Contents |
|------|----------|
| `SHA256SUMS` | Merkle root hash of the directory (plain text) |
| `SHA256SUMS.sign` | Ed25519 binary signature over `SHA256SUMS` |

## Build

```bash
cmake -S . -B build
cmake --build build
# binary: build/secure_sync
```

Requires **OpenSSL ≥ 1.1** and a C++17 compiler.

## Key generation

```bash
./generate_keys.sh
# produces: private.pem (keep secret!), public.pem (distribute freely)
```

## Usage

### Sign a directory

```bash
./build/secure_sync --sign <dir> private.pem
```

1. Hashes every regular file under `<dir>` with SHA-256.
2. Builds a Merkle tree and writes the root hash to `<dir>/SHA256SUMS`.
3. Signs `SHA256SUMS` with the Ed25519 private key → `<dir>/SHA256SUMS.sign`.

### Verify a directory

```bash
./build/secure_sync --check <dir> public.pem
```

1. Verifies the Ed25519 signature (`SHA256SUMS.sign`) against `SHA256SUMS`.
2. Re-computes the Merkle root from the current files.
3. Compares it against the trusted root stored in `SHA256SUMS`.
4. Exits with code 0 on success, 1 on any mismatch.

## How it works

Every regular file is hashed with **SHA-256** (leaf nodes of the Merkle tree).
The hashes are sorted by relative path, concatenated, and hashed again —
producing the **Merkle root**.  The root is written to `SHA256SUMS`, which is
then signed with **Ed25519** via OpenSSL, producing `SHA256SUMS.sign`.

On verification the signature is checked first (ensuring the manifest was not
replaced), then the Merkle root is recomputed and compared.
