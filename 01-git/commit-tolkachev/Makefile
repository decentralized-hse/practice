CC = gcc
CFLAGS = -O3

SRC_COMMON = common/dirent.c common/objutil.c
INC_COMMON = common/

TARGET = commit
TARGET_SRC	= $(TARGET:=.c)

$(TARGET): $(TARGET_SRC)
	$(CC) $(CFLAGS) $(TARGET_SRC) -lcrypto -o $(TARGET)

build: $(TARGET)

clean:
	rm -f $(TARGET)
