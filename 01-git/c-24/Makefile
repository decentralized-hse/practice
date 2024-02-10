CC = gcc
CFLAGS = -O3

SRC_COMMON = common/dirent.c common/objutil.c
INC_COMMON = common/

# space-separated list of compiled binaries
# to add new command, add it's name to this list and write name in $(name).c
PROGS			= example
PROG_SRC	= $(PROGS:=.c)

$(PROGS): $(SRC_COMMON) $(INC_COMMON) $(PROG_SRC)
	$(CC) $(CFLAGS) $(SRC_COMMON) $@.c -I$(INC_COMMON) -o $@

build: $(PROGS)

clean:
	rm $(PROG)