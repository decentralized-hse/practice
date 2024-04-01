# Сборка и запуск тестов

Тесты написаны прямо в `main.c`.
Чтобы их собрать, сначала скомпилировать биндинги Golang в C:
```bash
$ chmod +x build.sh
$ ./build.sh
# + LIB_DIR=../lib
# + cd go
# + go build -buildmode=c-archive zipint_tlv.go
# + mv zipint_tlv.a ../lib/
# + mv zipint_tlv.h ../lib/
```

Собрать и запустить тесты, можно воспользоваться следующими командами:
```bash
$ gcc -o lww main.c lww.c types.c utils.c lib/zipint_tlv.a
$ ./lww
# ================ TEST [int64] STARTED ================
# TLV1 LEN: 7, BYTES:     0x69:05:00:00:00:00:f6
# TLV2 LEN: 8, BYTES:     0x69:06:00:00:00:00:b2:02
# DELTA12 LEN: 8, BYTES:  0x69:06:01:00:00:00:b2:02
# MERGED LEN: 8, BYTES:   0x69:06:01:00:00:00:b2:02
# MERGED_S LEN: 3, BYTES: 0x33:34:35
# STR2 LEN: 3, BYTES:     0x33:34:35
# ================ TEST [int64] FINISHED ================

# ================ TEST [string] STARTED ================
# TLV1 LEN: 17, BYTES:    0x69:0f:00:00:00:00:66:63:75:6b:0a:22:7a:69:73:22:0a
# QUOTED LEN: 17, BYTES:  0x22:66:63:75:6b:5c:6e:5c:22:7a:69:73:5c:22:5c:6e:22
# PARSED LEN: 17, BYTES:  0x73:0f:00:00:00:00:66:63:75:6b:0a:22:7a:69:73:22:0a
# UNQ LEN: 11, BYTES:     0x66:63:75:6b:0a:22:7a:69:73:22:0a
# STR1 LEN: 11, BYTES:    0x66:63:75:6b:0a:22:7a:69:73:22:0a
# PL_TLV1 LEN: 11, BYTES: 0x66:63:75:6b:0a:22:7a:69:73:22:0a
# TLV2 LEN: 17, BYTES:    0x69:0f:00:00:00:00:66:63:75:6b:0a:22:7a:61:74:22:0a
# DELTA12 LEN: 17, BYTES: 0x69:0f:01:00:00:00:66:63:75:6b:0a:22:7a:61:74:22:0a
# MERGED LEN: 17, BYTES:  0x69:0f:01:00:00:00:66:63:75:6b:0a:22:7a:61:74:22:0a
# PL_MERGED LEN: 11, BYTES:       0x66:63:75:6b:0a:22:7a:61:74:22:0a
# ================ TEST [string] FINISHED ================

# ================ TEST [float64] STARTED ================
# TLV1 LEN: 14, BYTES:    0x66:0c:00:00:00:00:6f:12:83:c0:ca:21:09:40
# ID1: 3.141500, ID2: 3.141500
# TLV2 LEN: 14, BYTES:    0x66:0c:00:00:00:00:7a:00:8b:fc:fa:21:09:40
# DELTA12 LEN: 14, BYTES: 0x66:0c:01:00:00:00:7a:00:8b:fc:fa:21:09:40
# MERGED LEN: 14, BYTES:  0x66:0c:01:00:00:00:7a:00:8b:fc:fa:21:09:40
# MER_S LEN: 8, BYTES:    0x33:2e:31:34:31:35:39:32
# STR2 LEN: 8, BYTES:     0x33:2e:31:34:31:35:39:32
# ================ TEST [float64] FINISHED ================

# ================ TEST [id64] STARTED ================
# TLV1 LEN: 11, BYTES:    0x72:09:00:00:00:00:00:20:03:00:ae
# ID1: ae00000032000, ID2: ae00000032000
# TLV2 LEN: 11, BYTES:    0x72:09:00:00:00:00:00:30:03:00:ae
# DELTA12 LEN: 11, BYTES: 0x72:09:01:00:00:00:00:30:03:00:ae
# MERGED LEN: 11, BYTES:  0x72:09:01:00:00:00:00:30:03:00:ae
# MER_S LEN: 5, BYTES:    0x61:65:2d:33:33
# STR2 LEN: 5, BYTES:     0x61:65:2d:33:33
# ================ TEST [id64] FINISHED ================
```
