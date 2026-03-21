/*
 * enrich_geo.c — обогащение IP-адресов геоданными и ASN
 * Компиляция: gcc -O2 -Wall -Wextra -std=c11 -o enrich_geo enrich_geo.c -lmaxminddb
 * Использование: ./enrich_geo clean_ips.txt > enriched.jsonl
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <maxminddb.h>

#define MAX_IP_LEN 16
#define MAX_BUF 4096

void json_escape(FILE *out, const char *str) {
    if (!str) {
        fprintf(out, "null");
        return;
    }
    
    fputc('"', out);
    for (const char *p = str; *p; p++) {
        switch (*p) {
            case '"':  fputs("\\\"", out); break;
            case '\\': fputs("\\\\", out); break;
            case '\n': fputs("\\n", out); break;
            case '\r': fputs("\\r", out); break;
            case '\t': fputs("\\t", out); break;
            default:   fputc(*p, out); break;
        }
    }
    fputc('"', out);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <файл_с_IP>\n", argv[0]);
        return 1;
    }
    
    MMDB_s city_db, asn_db;
    int status;
    
    status = MMDB_open("./geodb/GeoLite2-City.mmdb", MMDB_MODE_MMAP, &city_db);
    if (status != MMDB_SUCCESS) {
        fprintf(stderr, "Ошибка открытия City DB: %s\n", MMDB_strerror(status));
        fprintf(stderr, "Убедитесь, что файл GeoLite2-City.mmdb находится в текущей директории\n");
        return 1;
    }
    
    status = MMDB_open("./geodb/GeoLite2-ASN.mmdb", MMDB_MODE_MMAP, &asn_db);
    if (status != MMDB_SUCCESS) {
        fprintf(stderr, "Ошибка открытия ASN DB: %s\n", MMDB_strerror(status));
        fprintf(stderr, "Убедитесь, что файл GeoLite2-ASN.mmdb находится в текущей директории\n");
        MMDB_close(&city_db);
        return 1;
    }
    
    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        perror("fopen");
        MMDB_close(&city_db);
        MMDB_close(&asn_db);
        return 1;
    }
    
    char ip_str[MAX_IP_LEN];
    int gai_error, mmdb_error;
    MMDB_lookup_result_s city_result, asn_result;
    MMDB_entry_data_s entry_data;
    int processed = 0;
    int with_asn = 0;
    
    while (fgets(ip_str, sizeof(ip_str), fp)) {
        ip_str[strcspn(ip_str, "\n")] = '\0';
        if (strlen(ip_str) == 0) continue;
        city_result = MMDB_lookup_string(&city_db, ip_str, &gai_error, &mmdb_error);
        if (gai_error != 0 || mmdb_error != MMDB_SUCCESS || !city_result.found_entry) {
            continue;
        }
        
        asn_result = MMDB_lookup_string(&asn_db, ip_str, &gai_error, &mmdb_error);
        printf("{");
        printf("\"ip\":\"%s\"", ip_str);
        const char *country_path[] = { "country", "names", "ru", NULL };
        if (MMDB_aget_value(&city_result.entry, &entry_data, country_path) == MMDB_SUCCESS && 
            entry_data.has_data && entry_data.type == MMDB_DATA_TYPE_UTF8_STRING) {
            printf(",\"country\":\"%.*s\"", entry_data.data_size, entry_data.utf8_string);
        } else {
            const char *country_en_path[] = { "country", "names", "en", NULL };
            if (MMDB_aget_value(&city_result.entry, &entry_data, country_en_path) == MMDB_SUCCESS &&
                entry_data.has_data && entry_data.type == MMDB_DATA_TYPE_UTF8_STRING) {
                printf(",\"country\":\"%.*s\"", entry_data.data_size, entry_data.utf8_string);
            } else {
                printf(",\"country\":null");
            }
        }
        
        const char *country_code_path[] = { "country", "iso_code", NULL };
        if (MMDB_aget_value(&city_result.entry, &entry_data, country_code_path) == MMDB_SUCCESS &&
            entry_data.has_data && entry_data.type == MMDB_DATA_TYPE_UTF8_STRING) {
            printf(",\"country_code\":\"%.*s\"", entry_data.data_size, entry_data.utf8_string);
        } else {
            printf(",\"country_code\":null");
        }
        
        const char *city_path[] = { "city", "names", "ru", NULL };
        if (MMDB_aget_value(&city_result.entry, &entry_data, city_path) == MMDB_SUCCESS &&
            entry_data.has_data && entry_data.type == MMDB_DATA_TYPE_UTF8_STRING) {
            printf(",\"city\":\"%.*s\"", entry_data.data_size, entry_data.utf8_string);
        } else {
            const char *city_en_path[] = { "city", "names", "en", NULL };
            if (MMDB_aget_value(&city_result.entry, &entry_data, city_en_path) == MMDB_SUCCESS &&
                entry_data.has_data && entry_data.type == MMDB_DATA_TYPE_UTF8_STRING) {
                printf(",\"city\":\"%.*s\"", entry_data.data_size, entry_data.utf8_string);
            } else {
                printf(",\"city\":null");
            }
        }
        
        const char *lat_path[] = { "location", "latitude", NULL };
        if (MMDB_aget_value(&city_result.entry, &entry_data, lat_path) == MMDB_SUCCESS &&
            entry_data.has_data && entry_data.type == MMDB_DATA_TYPE_DOUBLE) {
            printf(",\"latitude\":%f", entry_data.double_value);
        } else {
            printf(",\"latitude\":null");
        }
        
        const char *lon_path[] = { "location", "longitude", NULL };
        if (MMDB_aget_value(&city_result.entry, &entry_data, lon_path) == MMDB_SUCCESS &&
            entry_data.has_data && entry_data.type == MMDB_DATA_TYPE_DOUBLE) {
            printf(",\"longitude\":%f", entry_data.double_value);
        } else {
            printf(",\"longitude\":null");
        }
        
        if (asn_result.found_entry) {
            const char *asn_num_path[] = { "autonomous_system_number", NULL };
            if (MMDB_aget_value(&asn_result.entry, &entry_data, asn_num_path) == MMDB_SUCCESS &&
                entry_data.has_data) {
                if (entry_data.type == MMDB_DATA_TYPE_UINT32) {
                    printf(",\"asn\":%u", entry_data.uint32);
                    with_asn++;
                } else if (entry_data.type == MMDB_DATA_TYPE_INT32) {
                    printf(",\"asn\":%d", entry_data.int32);
                    with_asn++;
                } else {
                    printf(",\"asn\":null");
                }
            } else {
                printf(",\"asn\":null");
            }
            
            const char *asn_org_path[] = { "autonomous_system_organization", NULL };
            if (MMDB_aget_value(&asn_result.entry, &entry_data, asn_org_path) == MMDB_SUCCESS &&
                entry_data.has_data && entry_data.type == MMDB_DATA_TYPE_UTF8_STRING) {
                printf(",\"aso\":\"%.*s\"", entry_data.data_size, entry_data.utf8_string);
            } else {
                printf(",\"aso\":null");
            }
        } else {
            printf(",\"asn\":null,\"aso\":null");
        }
        printf("}\n");
        processed++;
    }

    fprintf(stderr, "Обработано IP: %d\n", processed);
    fprintf(stderr, "IP с ASN: %d (%.1f%%)\n", with_asn, (float)with_asn/processed*100);
    fclose(fp);
    MMDB_close(&city_db);
    MMDB_close(&asn_db);
    
    return 0;
}