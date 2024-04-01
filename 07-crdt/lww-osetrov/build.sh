#!/bin/bash
set -ex

LIB_DIR=../lib

cd go
go build -buildmode=c-archive zipint_tlv.go
mv zipint_tlv.a $LIB_DIR/
mv zipint_tlv.h $LIB_DIR/
