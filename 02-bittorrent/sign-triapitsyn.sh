#!/bin/bash

if [ $# -lt 1 ]
then
echo "Not enough arguments"
exit 1
fi

target_file="$1"
hash_file="$target_file.root"
public_key_file="$target_file.pub"
private_key_file="$target_file.sec"
signature_file="$target_file.sign"

hash=$(cat "$hash_file")
private_key=$(head -c 32 /dev/urandom | xxd -p | head -c 32)
public_key=$(echo -n "$private_key" | sha256sum | cut -f1 -d' ')
signature=$(echo -n "$hash$private_key" | sha256sum | cut -f1 -d' ')

echo "$public_key" > "$public_key_file"
echo "$private_key" > "$private_key_file"
echo "$signature" > "$signature_file"

