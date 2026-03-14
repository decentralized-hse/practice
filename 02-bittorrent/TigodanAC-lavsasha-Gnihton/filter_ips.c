/*
 * filter_ips.c — фильтрация валидных публичных IPv4 адресов
 * Компиляция: gcc -O2 -o filter_ips filter_ips.c
 * Использование: ./filter_ips peers.bin > clean_ips.txt
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdint.h>

#define MAX_LINE 32

long get_file_size(FILE *fp) {
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    return size;
}

int is_private_or_special(uint32_t ip) {
    if ((ip & 0xFF000000) == 0x0A000000) return 1;     
    if ((ip & 0xFFF00000) == 0xAC100000) return 1;     
    if ((ip & 0xFFFF0000) == 0xC0A80000) return 1;    
    if ((ip & 0xFF000000) == 0x7F000000) return 1;      
    if ((ip & 0xF0000000) == 0xE0000000) return 1;   
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Использование: %s <бинарный_файл>\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    long file_size = get_file_size(fp);
    if (file_size <= 0 || file_size % 4 != 0) {
        fprintf(stderr, "Ошибка: размер файла %ld байт не кратен 4\n", file_size);
        fclose(fp);
        return 1;
    }

    int count = file_size / 4;
    fprintf(stderr, "Размер файла: %ld байт, ожидается %d IP\n", file_size, count);

    uint32_t *ips = (uint32_t *)malloc(count * sizeof(uint32_t));
    if (!ips) {
        fprintf(stderr, "Ошибка выделения памяти\n");
        fclose(fp);
        return 1;
    }

    size_t read_count = fread(ips, sizeof(uint32_t), count, fp);
    if (read_count != count) {
        fprintf(stderr, "Ошибка чтения: прочитано %zu из %d\n", read_count, count);
        free(ips);
        fclose(fp);
        return 1;
    }

    fclose(fp);
    int unique_count = 0;
    fprintf(stderr, "\n Обработка IP...\n");

    for (int i = 0; i < count; i++) {
        uint32_t ip_le = ips[i];
        uint32_t ip_host = ((ip_le >> 24) & 0xFF)
                         | ((ip_le >> 8)  & 0xFF00)
                         | ((ip_le << 8)  & 0xFF0000)
                         | ((ip_le << 24) & 0xFF000000u);

        if (is_private_or_special(ip_host)) {
            continue;
        }

        int dup = 0;
        for (int j = 0; j < i; j++) {
            uint32_t prev_host = ((ips[j] >> 24) & 0xFF)
                               | ((ips[j] >> 8)  & 0xFF00)
                               | ((ips[j] << 8)  & 0xFF0000)
                               | ((ips[j] << 24) & 0xFF000000u);
            if (prev_host == ip_host) {
                dup = 1;
                break;
            }
        }

        if (!dup) {
            struct in_addr addr;
            addr.s_addr = htonl(ip_host); 
            printf("%s\n", inet_ntoa(addr));
            unique_count++;
        }

        if (i > 0 && i % 10000 == 0) {
            fprintf(stderr, "  Обработано %d/%d...\n", i, count);
        }
    }

    fprintf(stderr, "\n Найдено уникальных публичных IP: %d\n", unique_count);
    fprintf(stderr, "   Всего IP в файле: %d\n", count);
    fprintf(stderr, "   Отсеяно: %d (приватные/дубликаты)\n", count - unique_count);

    free(ips);
    return 0;
}