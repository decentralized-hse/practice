#!/bin/bash


if test -z "$1"; then
    echo "Wrong arg!"
    exit 1
fi

RESULT_FILE="$1.root"

echo "Processing $1"
echo `sha256sum $1 | sed "s/ ${1}/\n/"` > $RESULT_FILE
echo "Done. See result in $RESULT_FILE"
