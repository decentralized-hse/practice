LIB_DIR = ./lib

go_binds: go/zipint_tlv.go
	go build -buildmode=c-archive go/zipint_tlv.go
	mv zipint_tlv.a $(LIB_DIR)/
	mv zipint_tlv.h $(LIB_DIR)/

CC 			= gcc
CFLAGS 	= -g

SRC = main.c nz.c types.c utils.c
INC = $(LIB_DIR)

build: go_binds
	$(CC) $(CFLAGS) $(SRC) $(LIB_DIR)/zipint_tlv.a -I$(INC) -o nz.out
