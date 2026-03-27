echo "[*] Generating Private Key (private.pem)..."
openssl genpkey -algorithm Ed25519 -out private.pem

echo "[*] Extracting Public Key (public.pem)..."
openssl pkey -in private.pem -pubout -out public.pem

echo "[+] Done. Keep 'private.pem' secret!"
chmod 600 private.pem
