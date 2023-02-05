#!/bin/bash

(sha256sum $1.peaks | cut -d ' ' -f 1) > "$1.root"

