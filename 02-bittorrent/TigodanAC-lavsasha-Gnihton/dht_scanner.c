/*
 * dht_scanner.c — BitTorrent DHT Network Crawler / Peer Scanner
 *
 * Обходит DHT-сеть, собирает IP-адреса всех участников (DHT-ноды + пиры).
 * Каждая DHT-нода — это BitTorrent-клиент с реальным IP-адресом.
 *
 * Стратегия сбора:
 *   1. get_peers по заданному info_hash — находит пиров конкретного торрента
 *   2. find_node со случайными target — обходит разные участки DHT
 *   3. Все обнаруженные IP сохраняются (и ноды, и пиры)
 *
 * Сборка (Linux):
 *   gcc -O2 -Wall -Wextra -std=c11 -o dht_scanner dht_scanner.c
 *
 * Использование:
 *   ./dht_scanner <info_hash_hex | magnet> [время_сек] [файл]
 *
 * Примеры:
 *   ./dht_scanner 08ada5a7a6183aae1e09d831df6748d566095a10 300 peers.txt
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <process.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
typedef SOCKET socket_t;
#define CLOSE_SOCKET closesocket
#define SOCKET_VALID(s) ((s) != INVALID_SOCKET)
#define getpid _getpid
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int socket_t;
#define CLOSE_SOCKET close
#define SOCKET_VALID(s) ((s) >= 0)
#define INVALID_SOCKET (-1)
#endif

#define RECV_BUF_SIZE    65536
#define MSG_BUF_SIZE     512
#define NODE_ID_LEN      20
#define COMPACT_PEER     6
#define COMPACT_NODE     26
#define DEFAULT_DURATION 300
#define QUERY_INTERVAL_MS 5

#define INITIAL_NODES    65536
#define MAX_NODES_LIMIT  2000000

#define IP_HASHSET_SIZE  (1 << 20) 
#define IP_HASHSET_MASK  (IP_HASHSET_SIZE - 1)

static const char *bootstrap_hosts[] = {
    "router.bittorrent.com",
    "dht.transmissionbt.com",
    "router.utorrent.com",
    "dht.libtorrent.org",
};
#define NUM_BOOTSTRAP (int)(sizeof(bootstrap_hosts) / sizeof(bootstrap_hosts[0]))
static const int bootstrap_port = 6881;

typedef struct {
    uint32_t *slots;    
    uint8_t  *occupied;  
    int       size;     
    int       mask;
    int       count;   
} ip_hashset_t;

static int ip_hashset_init(ip_hashset_t *hs, int size) {
    hs->slots = (uint32_t *)calloc(size, sizeof(uint32_t));
    hs->occupied = (uint8_t *)calloc(size, sizeof(uint8_t));
    if (!hs->slots || !hs->occupied) return -1;
    hs->size = size;
    hs->mask = size - 1;
    hs->count = 0;
    return 0;
}

static void ip_hashset_free(ip_hashset_t *hs) {
    free(hs->slots);
    free(hs->occupied);
}

static uint32_t ip_hash(uint32_t ip) {
    ip ^= ip >> 16;
    ip *= 0x45d9f3b;
    ip ^= ip >> 16;
    ip *= 0x45d9f3b;
    ip ^= ip >> 16;
    return ip;
}

static int ip_hashset_insert(ip_hashset_t *hs, uint32_t ip) {
    if (ip == 0) return 0;
    uint32_t idx = ip_hash(ip) & hs->mask;
    for (int i = 0; i < 64; i++) {
        uint32_t pos = (idx + i) & hs->mask;
        if (!hs->occupied[pos]) {
            hs->slots[pos] = ip;
            hs->occupied[pos] = 1;
            hs->count++;
            return 1;
        }
        if (hs->slots[pos] == ip) return 0; 
    }
    return 0;
}

typedef enum {
    BENC_STR,
    BENC_INT,
    BENC_LIST,
    BENC_DICT,
    BENC_NONE
} benc_type_t;

typedef struct benc_val {
    benc_type_t type;
    const uint8_t *str;
    int str_len;
    int64_t num;
    struct benc_val *items;
    int item_count;
} benc_val_t;

#define BENC_MAX_ITEMS 256

static int benc_parse(const uint8_t *data, int len, benc_val_t *out);

static int benc_parse_str(const uint8_t *data, int len, benc_val_t *out) {
    int i = 0;
    int slen = 0;
    while (i < len && data[i] >= '0' && data[i] <= '9') {
        slen = slen * 10 + (data[i] - '0');
        i++;
    }
    if (i == 0 || i >= len || data[i] != ':') return -1;
    i++;
    if (i + slen > len) return -1;
    out->type = BENC_STR;
    out->str = data + i;
    out->str_len = slen;
    out->items = NULL;
    out->item_count = 0;
    return i + slen;
}

static int benc_parse_int(const uint8_t *data, int len, benc_val_t *out) {
    if (len < 3 || data[0] != 'i') return -1;
    int i = 1;
    int neg = 0;
    if (i < len && data[i] == '-') { neg = 1; i++; }
    int64_t val = 0;
    while (i < len && data[i] >= '0' && data[i] <= '9') {
        val = val * 10 + (data[i] - '0');
        i++;
    }
    if (i >= len || data[i] != 'e') return -1;
    out->type = BENC_INT;
    out->num = neg ? -val : val;
    out->str = NULL;
    out->str_len = 0;
    out->items = NULL;
    out->item_count = 0;
    return i + 1;
}

static int benc_parse_list(const uint8_t *data, int len, benc_val_t *out) {
    if (len < 2 || data[0] != 'l') return -1;
    int pos = 1;
    out->type = BENC_LIST;
    out->items = (benc_val_t *)calloc(BENC_MAX_ITEMS, sizeof(benc_val_t));
    out->item_count = 0;
    out->str = NULL;
    out->str_len = 0;
    while (pos < len && data[pos] != 'e') {
        if (out->item_count >= BENC_MAX_ITEMS) break;
        int consumed = benc_parse(data + pos, len - pos, &out->items[out->item_count]);
        if (consumed <= 0) { free(out->items); out->items = NULL; return -1; }
        out->item_count++;
        pos += consumed;
    }
    if (pos >= len) { free(out->items); out->items = NULL; return -1; }
    return pos + 1;
}

static int benc_parse_dict(const uint8_t *data, int len, benc_val_t *out) {
    if (len < 2 || data[0] != 'd') return -1;
    int pos = 1;
    out->type = BENC_DICT;
    out->items = (benc_val_t *)calloc(BENC_MAX_ITEMS, sizeof(benc_val_t));
    out->item_count = 0;
    out->str = NULL;
    out->str_len = 0;
    while (pos < len && data[pos] != 'e') {
        if (out->item_count + 1 >= BENC_MAX_ITEMS) break;
        int consumed = benc_parse_str(data + pos, len - pos, &out->items[out->item_count]);
        if (consumed <= 0) { free(out->items); out->items = NULL; return -1; }
        out->item_count++;
        pos += consumed;
        consumed = benc_parse(data + pos, len - pos, &out->items[out->item_count]);
        if (consumed <= 0) { free(out->items); out->items = NULL; return -1; }
        out->item_count++;
        pos += consumed;
    }
    if (pos >= len) { free(out->items); out->items = NULL; return -1; }
    return pos + 1;
}

static int benc_parse(const uint8_t *data, int len, benc_val_t *out) {
    if (len <= 0) return -1;
    if (data[0] == 'i') return benc_parse_int(data, len, out);
    if (data[0] == 'l') return benc_parse_list(data, len, out);
    if (data[0] == 'd') return benc_parse_dict(data, len, out);
    if (data[0] >= '0' && data[0] <= '9') return benc_parse_str(data, len, out);
    return -1;
}

static void benc_free(benc_val_t *val) {
    if (val->items) {
        for (int i = 0; i < val->item_count; i++)
            benc_free(&val->items[i]);
        free(val->items);
        val->items = NULL;
    }
}

static benc_val_t *benc_dict_get(benc_val_t *dict, const char *key) {
    if (dict->type != BENC_DICT) return NULL;
    int klen = (int)strlen(key);
    for (int i = 0; i + 1 < dict->item_count; i += 2) {
        benc_val_t *k = &dict->items[i];
        if (k->type == BENC_STR && k->str_len == klen &&
            memcmp(k->str, key, klen) == 0)
            return &dict->items[i + 1];
    }
    return NULL;
}

typedef struct {
    uint32_t ip;
    uint16_t port;
    int      queried;
} node_entry_t;

static uint8_t our_node_id[NODE_ID_LEN];
static uint8_t target_hash[NODE_ID_LEN];

static ip_hashset_t ip_set;      
static ip_hashset_t node_ip_set; 

static node_entry_t *node_list = NULL;
static int node_count = 0;
static int node_capacity = 0;

static uint16_t tx_counter = 0;
static int total_responses = 0;
static int total_peers_found = 0;  
static int total_nodes_found = 0;   

static int hex_char(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int parse_info_hash(const char *hex, uint8_t *out) {
    if (strlen(hex) != 40) return -1;
    for (int i = 0; i < 20; i++) {
        int hi = hex_char(hex[i * 2]);
        int lo = hex_char(hex[i * 2 + 1]);
        if (hi < 0 || lo < 0) return -1;
        out[i] = (uint8_t)((hi << 4) | lo);
    }
    return 0;
}

static int parse_magnet(const char *magnet, uint8_t *out) {
    const char *p = strstr(magnet, "btih:");
    if (!p) return -1;
    p += 5;
    char hex[41];
    int len = 0;
    while (p[len] && p[len] != '&' && len < 40) {
        hex[len] = p[len];
        len++;
    }
    hex[len] = '\0';
    if (len == 40) return parse_info_hash(hex, out);
    fprintf(stderr, "Ошибка: поддерживается только hex info_hash (40 символов)\n");
    return -1;
}

static void generate_random_id(uint8_t *id) {
    for (int i = 0; i < NODE_ID_LEN; i++)
        id[i] = (uint8_t)(rand() & 0xFF);
}

static uint32_t node_key(uint32_t ip, uint16_t port) {
    return ip ^ ((uint32_t)port << 16) ^ ((uint32_t)port >> 16);
}

static int add_node(uint32_t ip, uint16_t port) {
    if (ip == 0 || port == 0) return 0;
    if (!ip_hashset_insert(&node_ip_set, node_key(ip, port))) return 0;
    ip_hashset_insert(&ip_set, ip);

    if (node_count >= node_capacity) {
        if (node_capacity >= MAX_NODES_LIMIT) return 0;
        int new_cap = node_capacity * 2;
        if (new_cap > MAX_NODES_LIMIT) new_cap = MAX_NODES_LIMIT;
        node_entry_t *tmp = (node_entry_t *)realloc(node_list,
                                new_cap * sizeof(node_entry_t));
        if (!tmp) return 0;
        node_list = tmp;
        node_capacity = new_cap;
    }

    node_list[node_count].ip = ip;
    node_list[node_count].port = port;
    node_list[node_count].queried = 0;
    node_count++;
    total_nodes_found++;
    return 1;
}

static void add_peer_ip(uint32_t ip) {
    if (ip == 0) return;
    if (ip_hashset_insert(&ip_set, ip))
        total_peers_found++;
}

#define APPEND(data, sz) do { \
    if (pos + (int)(sz) > buflen) return -1; \
    memcpy(buf + pos, data, sz); \
    pos += (int)(sz); \
} while (0)

static int build_get_peers(uint8_t *buf, int buflen,
                           const uint8_t *node_id,
                           const uint8_t *info_hash,
                           uint16_t txid) {
    uint8_t tb[2] = {(uint8_t)(txid >> 8), (uint8_t)(txid & 0xFF)};
    int pos = 0;
    APPEND("d", 1);
    APPEND("1:ad", 4);
    APPEND("2:id20:", 7);       APPEND(node_id, 20);
    APPEND("9:info_hash20:", 14); APPEND(info_hash, 20);
    APPEND("e", 1);
    APPEND("1:q9:get_peers", 14);
    APPEND("1:t2:", 5);         APPEND(tb, 2);
    APPEND("1:y1:q", 6);
    APPEND("e", 1);
    return pos;
}

static int build_find_node(uint8_t *buf, int buflen,
                           const uint8_t *node_id,
                           const uint8_t *target,
                           uint16_t txid) {
    uint8_t tb[2] = {(uint8_t)(txid >> 8), (uint8_t)(txid & 0xFF)};
    int pos = 0;
    APPEND("d", 1);
    APPEND("1:ad", 4);
    APPEND("2:id20:", 7);        APPEND(node_id, 20);
    APPEND("6:target20:", 11);   APPEND(target, 20);
    APPEND("e", 1);
    APPEND("1:q9:find_node", 14);
    APPEND("1:t2:", 5);          APPEND(tb, 2);
    APPEND("1:y1:q", 6);
    APPEND("e", 1);
    return pos;
}

#undef APPEND

static int process_nodes(benc_val_t *nodes_val) {
    if (!nodes_val || nodes_val->type != BENC_STR) return 0;
    int count = nodes_val->str_len / COMPACT_NODE;
    int added = 0;
    for (int i = 0; i < count; i++) {
        const uint8_t *n = nodes_val->str + i * COMPACT_NODE;
        uint32_t ip;
        uint16_t port;
        memcpy(&ip, n + 20, 4);
        memcpy(&port, n + 24, 2);
        if (add_node(ip, port)) added++;
    }
    return added;
}

static int process_values(benc_val_t *values_val) {
    if (!values_val || values_val->type != BENC_LIST) return 0;
    int added = 0;
    for (int i = 0; i < values_val->item_count; i++) {
        benc_val_t *item = &values_val->items[i];
        if (item->type != BENC_STR || item->str_len != COMPACT_PEER) continue;
        uint32_t ip;
        memcpy(&ip, item->str, 4);
        add_peer_ip(ip);
        added++;
    }
    return added;
}

static void handle_response(const uint8_t *data, int datalen) {
    benc_val_t root;
    memset(&root, 0, sizeof(root));
    if (benc_parse(data, datalen, &root) <= 0) return;
    if (root.type != BENC_DICT) { benc_free(&root); return; }

    benc_val_t *y = benc_dict_get(&root, "y");
    if (!y || y->type != BENC_STR || y->str_len != 1 || y->str[0] != 'r') {
        benc_free(&root);
        return;
    }
    total_responses++;

    benc_val_t *r = benc_dict_get(&root, "r");
    if (!r || r->type != BENC_DICT) { benc_free(&root); return; }

    benc_val_t *n = benc_dict_get(r, "nodes");
    if (n) process_nodes(n);

    benc_val_t *v = benc_dict_get(r, "values");
    if (v) process_values(v);

    benc_free(&root);
}

static int send_udp(socket_t sock, const uint8_t *data, int len,
                    uint32_t ip, uint16_t port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ip;
    addr.sin_port = port;
    return (int)sendto(sock, (const char *)data, len, 0,
                       (struct sockaddr *)&addr, sizeof(addr));
}

static uint32_t resolve_host(const char *host) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(host, NULL, &hints, &res) != 0) return 0;
    uint32_t ip = ((struct sockaddr_in *)res->ai_addr)->sin_addr.s_addr;
    freeaddrinfo(res);
    return ip;
}

static void send_get_peers_to(socket_t sock, uint32_t ip, uint16_t port) {
    uint8_t buf[MSG_BUF_SIZE];
    int len = build_get_peers(buf, sizeof(buf), our_node_id, target_hash, tx_counter++);
    if (len > 0) send_udp(sock, buf, len, ip, port);
}

static void send_find_node_to(socket_t sock, uint32_t ip, uint16_t port,
                               const uint8_t *target) {
    uint8_t buf[MSG_BUF_SIZE];
    int len = build_find_node(buf, sizeof(buf), our_node_id, target, tx_counter++);
    if (len > 0) send_udp(sock, buf, len, ip, port);
}

static void print_hash(const uint8_t *hash) {
    for (int i = 0; i < 20; i++) printf("%02x", hash[i]);
}

static void sleep_ms(int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
#endif
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr,
            "Использование: %s <info_hash | magnet> [время_сек] [файл]\n\n"
            "  info_hash  — 40 hex-символов\n"
            "  magnet     — magnet:?xt=urn:btih:...\n"
            "  время_сек  — длительность (по умолчанию %d)\n"
            "  файл       — выходной файл (по умолчанию peers.bin)\n",
            argv[0], DEFAULT_DURATION);
        return 1;
    }

    const char *input = argv[1];
    int duration = (argc >= 3) ? atoi(argv[2]) : DEFAULT_DURATION;
    const char *outfile = (argc >= 4) ? argv[3] : "peers.bin";
    if (duration <= 0) duration = DEFAULT_DURATION;

    if (strstr(input, "magnet:") || strstr(input, "MAGNET:")) {
        if (parse_magnet(input, target_hash) != 0) {
            fprintf(stderr, "Ошибка: не удалось извлечь info_hash из magnet-ссылки\n");
            return 1;
        }
    } else {
        if (parse_info_hash(input, target_hash) != 0) {
            fprintf(stderr, "Ошибка: неверный info_hash (нужно 40 hex-символов)\n");
            return 1;
        }
    }

    printf("=== DHT Network Crawler / Peer Scanner ===\n\n");
    printf("Info_hash: ");
    print_hash(target_hash);
    printf("\n");
    printf("Время сканирования: %d сек\n", duration);
    printf("Выходной файл: %s\n\n", outfile);

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "Ошибка WSAStartup\n");
        return 1;
    }
#endif

    srand((unsigned)time(NULL) ^ (unsigned)getpid());
    generate_random_id(our_node_id);

    if (ip_hashset_init(&ip_set, IP_HASHSET_SIZE) < 0 ||
        ip_hashset_init(&node_ip_set, IP_HASHSET_SIZE) < 0) {
        fprintf(stderr, "Ошибка выделения памяти для hash set\n");
        return 1;
    }

    node_capacity = INITIAL_NODES;
    node_list = (node_entry_t *)malloc(node_capacity * sizeof(node_entry_t));
    if (!node_list) {
        fprintf(stderr, "Ошибка выделения памяти для списка нод\n");
        return 1;
    }

    socket_t sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (!SOCKET_VALID(sock)) { perror("socket"); return 1; }

    struct sockaddr_in bind_addr;
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = INADDR_ANY;
    bind_addr.sin_port = 0;
    if (bind(sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
        perror("bind");
        CLOSE_SOCKET(sock);
        return 1;
    }

#ifdef _WIN32
    u_long nonblock = 1;
    ioctlsocket(sock, FIONBIO, &nonblock);
#else
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif

    int rcvbuf = 2 * 1024 * 1024;
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (const char *)&rcvbuf, sizeof(rcvbuf));
    int sndbuf = 2 * 1024 * 1024;
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (const char *)&sndbuf, sizeof(sndbuf));

    printf("Подключение к bootstrap-нодам...\n");
    uint32_t bootstrap_ips[8];
    int bootstrap_count = 0;
    for (int i = 0; i < NUM_BOOTSTRAP; i++) {
        uint32_t ip = resolve_host(bootstrap_hosts[i]);
        if (ip != 0) {
            printf("  -> %s\n", bootstrap_hosts[i]);
            bootstrap_ips[bootstrap_count++] = ip;
            send_find_node_to(sock, ip, htons(bootstrap_port), target_hash);
            send_get_peers_to(sock, ip, htons(bootstrap_port));
            for (int j = 0; j < 4; j++) {
                uint8_t random_target[NODE_ID_LEN];
                generate_random_id(random_target);
                send_find_node_to(sock, ip, htons(bootstrap_port), random_target);
            }
        }
    }

    if (bootstrap_count == 0) {
        fprintf(stderr, "Ошибка: не удалось подключиться ни к одной bootstrap-ноде\n");
        CLOSE_SOCKET(sock);
        return 1;
    }

    time_t start_time = time(NULL);
    int query_idx = 0;
    int last_printed_ips = -1;
    int last_print_time = -1;

    printf("\nОбход DHT-сети...\n\n");

    while (1) {
        int elapsed = (int)(time(NULL) - start_time);
        if (elapsed >= duration) break;

        uint8_t recv_buf[RECV_BUF_SIZE];
        struct sockaddr_in from_addr;
        socklen_t from_len;
        int received;

        for (int r = 0; r < 500; r++) {
            from_len = sizeof(from_addr);
            received = (int)recvfrom(sock, (char *)recv_buf, sizeof(recv_buf), 0,
                                     (struct sockaddr *)&from_addr, &from_len);
            if (received <= 0) break;

            ip_hashset_insert(&ip_set, from_addr.sin_addr.s_addr);
            handle_response(recv_buf, received);
        }

        int queries_sent = 0;
        while (query_idx < node_count && queries_sent < 100) {
            if (!node_list[query_idx].queried) {
                node_list[query_idx].queried = 1;
                uint32_t nip = node_list[query_idx].ip;
                uint16_t nport = node_list[query_idx].port;

                send_get_peers_to(sock, nip, nport);

                if (queries_sent % 3 == 0) {
                    uint8_t random_target[NODE_ID_LEN];
                    generate_random_id(random_target);
                    send_find_node_to(sock, nip, nport, random_target);
                } else {
                    send_find_node_to(sock, nip, nport, target_hash);
                }

                queries_sent++;
            }
            query_idx++;
        }

        if (node_count < 100 && elapsed < 10 && elapsed % 2 == 0) {
            for (int i = 0; i < bootstrap_count; i++) {
                send_find_node_to(sock, bootstrap_ips[i], htons(bootstrap_port), target_hash);
                uint8_t rt[NODE_ID_LEN];
                generate_random_id(rt);
                send_find_node_to(sock, bootstrap_ips[i], htons(bootstrap_port), rt);
            }
        }

        if (elapsed != last_print_time || ip_set.count != last_printed_ips) {
            int remaining = duration - elapsed;
            int rate = (elapsed > 0) ? ip_set.count / elapsed : 0;
            printf("\r  [%3d/%ds] IP: %d | Нод: %d | Ответов: %d | ~%d IP/сек   ",
                   elapsed, duration, ip_set.count, node_count,
                   total_responses, rate);
            fflush(stdout);
            last_printed_ips = ip_set.count;
            last_print_time = elapsed;
        }

        sleep_ms(QUERY_INTERVAL_MS);
    }

    printf("\n\n=== Результат ===\n");
    printf("Получено ответов:       %d\n", total_responses);
    printf("Обнаружено DHT-нод:     %d\n", node_count);
    printf("Уникальных IP-адресов:  %d\n", ip_set.count);

    FILE *fp = fopen(outfile, "wb");
    if (!fp) {
        perror("fopen");
        CLOSE_SOCKET(sock);
        return 1;
    }

    int written = 0;
    for (int i = 0; i < ip_set.size; i++) {
        if (ip_set.occupied[i]) {
            uint32_t ip_be = ip_set.slots[i];
            uint32_t ip_le = ((ip_be >> 24) & 0xFF)
                           | ((ip_be >> 8)  & 0xFF00)
                           | ((ip_be << 8)  & 0xFF0000)
                           | ((ip_be << 24) & 0xFF000000u);
            fwrite(&ip_le, sizeof(uint32_t), 1, fp);
            written++;
        }
    }

    fclose(fp);
    printf("Сохранено в '%s':       %d IP (бинарный, little-endian, 4 байта/IP)\n",
           outfile, written);

    free(node_list);
    ip_hashset_free(&ip_set);
    ip_hashset_free(&node_ip_set);
    CLOSE_SOCKET(sock);
#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}
