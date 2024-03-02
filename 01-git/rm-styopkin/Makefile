CC = gcc
CFLAGS = -g

SRC_COMMON = common/dirent.c common/objutil.c
INC_COMMON = common/

PROGS		= rm
PROG_SRC	= $(PROGS:=.c)

$(PROGS): $(SRC_COMMON) $(INC_COMMON) $(PROG_SRC)
	$(CC) $(CFLAGS) $(SRC_COMMON) $@.c -I$(INC_COMMON) -lcrypto -o $@

build: $(PROGS)

clean:
	rm $(PROGS)