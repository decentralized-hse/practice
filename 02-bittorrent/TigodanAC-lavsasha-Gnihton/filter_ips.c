/*
 * filter_ips.c — фильтрация валидных публичных IPv4 адресов
 * Компиляция: gcc -O2 -o filter_ips filter_ips.c
 * Использование: ./filter_ips < peers.txt > peers_clean.txt
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdint.h>

#define MAX_LINE 32

int is_private_or_special(uint32_t ip) {
    if ((ip & 0xFF000000) == 0x0A000000) return 1;
    if ((ip & 0xFFF00000) == 0xAC100000) return 1;
    if ((ip & 0xFFFF0000) == 0xC0A80000) return 1;
    if ((ip & 0xFF000000) == 0x7F000000) return 1;
    if ((ip & 0xF0000000) == 0xE0000000) return 1;
    return 0;
}

int main() {
    char line[MAX_LINE];
    struct in_addr addr;
    uint32_t ip_binary;
    
    while (fgets(line, sizeof(line), stdin)) {
        line[strcspn(line, "\n")] = '\0';
        if (strlen(line) == 0) continue;
        if (inet_pton(AF_INET, line, &addr) != 1) continue;
        ip_binary = ntohl(addr.s_addr);
        if (!is_private_or_special(ip_binary)) {
            printf("%s\n", line);
        }
    }
    
    return 0;
}