/*
 * statistics.c — подсчет статистики по обогащенным данным с визуализацией
 * Компиляция: gcc -O2 -Wall -Wextra -std=c11 -o statistics statistics.c
 * Использование: ./statistics < enriched.jsonl
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <math.h>

#define MAX_LINE 4096
#define MAX_ENTRIES 1000
#define MAX_FILENAME 256

typedef struct {
    char name[100];
    int count;
    float percentage;
} counter_t;

counter_t countries[MAX_ENTRIES];
int country_count = 0;

counter_t asns[MAX_ENTRIES];
int asn_count = 0;

int total_ips = 0;
int ips_with_coords = 0;

int find_or_add_counter(counter_t *counters, int *count, const char *name) {
    for (int i = 0; i < *count; i++) {
        if (strcmp(counters[i].name, name) == 0) {
            return i;
        }
    }
    if (*count < MAX_ENTRIES) {
        strncpy(counters[*count].name, name, sizeof(counters[*count].name) - 1);
        counters[*count].count = 0;
        (*count)++;
        return *count - 1;
    }
    return -1;
}

char* extract_value(char *line, const char *key, char *value, int max_len) {
    char search_key[50];
    snprintf(search_key, sizeof(search_key), "\"%s\":", key);
    char *start = strstr(line, search_key);
    if (!start) return NULL;
    start += strlen(search_key);
    while (*start == ' ') start++;
    
    if (*start == '"') {
        start++;
        char *end = strchr(start, '"');
        if (!end) return NULL;

        int len = end - start;
        if (len >= max_len) len = max_len - 1;
        strncpy(value, start, len);
        value[len] = '\0';
        return value;
    } else if (isdigit(*start) || *start == '-' || *start == 'n') {
        char *end = start;
        while (*end && *end != ',' && *end != '}') end++;
        int len = end - start;
        if (len >= max_len) len = max_len - 1;
        strncpy(value, start, len);
        value[len] = '\0';
        return value;
    }
    
    return NULL;
}

void sort_counters(counter_t *counters, int count) {
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (counters[j].count < counters[j+1].count) {
                counter_t tmp = counters[j];
                counters[j] = counters[j+1];
                counters[j+1] = tmp;
            }
        }
    }
}

void calculate_percentages(counter_t *counters, int count, int total) {
    for (int i = 0; i < count; i++) {
        counters[i].percentage = (float)counters[i].count / total * 100;
    }
}

void generate_bar_chart(counter_t *counters, int count, const char *title, 
                        const char *filename, const char *xlabel, const char *ylabel) {
    FILE *data_file = fopen("temp_data.txt", "w");
    if (!data_file) {
        printf("Ошибка создания временного файла данных\n");
        return;
    }
    
    int plot_count = count < 15 ? count : 15;
    for (int i = 0; i < plot_count; i++) {
        fprintf(data_file, "%d \"%s\" %d %.1f\n", 
                i+1, counters[i].name, counters[i].count, counters[i].percentage);
    }
    fclose(data_file);
    
    FILE *gp_script = fopen("temp_plot.gp", "w");
    if (!gp_script) {
        printf("Ошибка создания временного скрипта gnuplot\n");
        return;
    }
    
    fprintf(gp_script, "set terminal pngcairo size 1200,800 enhanced font 'Arial,10'\n");
    fprintf(gp_script, "set output '%s'\n", filename);
    fprintf(gp_script, "set title '%s' font 'Arial,14'\n", title);
    fprintf(gp_script, "set xlabel '%s'\n", xlabel);
    fprintf(gp_script, "set ylabel '%s'\n", ylabel);
    fprintf(gp_script, "set grid ytics\n");
    fprintf(gp_script, "set style fill solid 0.8\n");
    fprintf(gp_script, "set boxwidth 0.8\n");
    fprintf(gp_script, "set xtics rotate by -45\n");
    fprintf(gp_script, "set yrange [0:*]\n\n");
    fprintf(gp_script, "plot 'temp_data.txt' using 1:3:xtic(2) with boxes lc rgb '#3498db' notitle, \\\n");
    fprintf(gp_script, "     '' using 1:3:(sprintf(\"%%.1f%%%%\", $4)) with labels offset 0,1 font 'Arial,9' notitle\n");
    fclose(gp_script);
    
    system("gnuplot temp_plot.gp");
    unlink("temp_data.txt");
    unlink("temp_plot.gp");
}

void generate_pie_chart(counter_t *counters, int count, const char *title, const char *filename) {
    if (count == 0) {
        return;
    }
    
    FILE *gp = fopen("pie_script.gp", "w");
    if (!gp) {
        printf("Ошибка создания скрипта gnuplot\n");
        return;
    }

    int show_count = count < 8 ? count : 8;
    double total = 0;
    for (int i = 0; i < count; i++) {
        total += counters[i].count;
    }
    
    double others = 0;
    for (int i = show_count; i < count; i++) {
        others += counters[i].count;
    }
    
    fprintf(gp, "set terminal pngcairo size 900,700 enhanced font 'Arial,11' \n");
    fprintf(gp, "set encoding utf8\n");  
    fprintf(gp, "set output '%s'\n", filename);
    fprintf(gp, "set title '%s' font 'Arial,14'\n", title);
    fprintf(gp, "unset border\n");
    fprintf(gp, "unset tics\n");
    fprintf(gp, "unset key\n");
    fprintf(gp, "set size ratio -1\n");
    fprintf(gp, "set xrange [-1.6:1.6]\n");
    fprintf(gp, "set yrange [-1.4:1.4]\n");
    fprintf(gp, "\n");
    
    const char *colors[] = {
        "#e74c3c", "#3498db", "#2ecc71", "#f39c12",
        "#9b59b6", "#1abc9c", "#e67e22", "#34495e", "#95a5a6"
    };
    
    double angle = 0;  
    int obj_id = 1;
    int label_id = 1;
    for (int i = 0; i < show_count; i++) {
        double pct = counters[i].count / total;
        double sweep = pct * 360.0;
        double mid = angle + sweep / 2.0;
        
        fprintf(gp, "set object %d circle at 0,0 size 1 arc [%.1f:%.1f] fc rgb '%s' fs solid 1 border lc rgb '#222222' lw 0.5\n",
                obj_id++, angle, angle + sweep, colors[i % 9]);
        
        char name[256];
        int byte_pos = 0;
        int char_count = 0;
        int k = 0;
        
        while (counters[i].name[k] && byte_pos < 250) {
            unsigned char c = counters[i].name[k];
            
            if (c == '"') {
                name[byte_pos++] = '\\';
                name[byte_pos++] = '"';
                char_count++;
            } else if (c != '\'' && c != '\\' && c != '$' && c != '%') {
                int bytes_in_char = 1;
                if ((c & 0x80) == 0) {
                    bytes_in_char = 1;
                } else if ((c & 0xE0) == 0xC0) {
                    bytes_in_char = 2;
                } else if ((c & 0xF0) == 0xE0) {
                    bytes_in_char = 3;
                } else if ((c & 0xF8) == 0xF0) {
                    bytes_in_char = 4;
                }
                
                if (char_count < 10) {
                    for (int b = 0; b < bytes_in_char && counters[i].name[k]; b++) {
                        name[byte_pos++] = counters[i].name[k++];
                    }
                    char_count++;
                    continue;
                } else {
                    name[byte_pos++] = '.';
                    name[byte_pos++] = '.';
                    name[byte_pos++] = '.';
                    break;
                }
            }
            k++;
        }
        name[byte_pos] = '\0';
        fprintf(gp, "set label %d \"%s\\n%.1f%%\" at 0.65*cos(%.3f*pi/180),0.65*sin(%.3f*pi/180) center font 'Arial,9' textcolor rgb '#000000'\n",
                label_id++, name, pct * 100.0, mid, mid);
        angle += sweep;
    }
    
    if (others > 0) {
        double pct = others / total;
        double sweep = pct * 360.0;
        double mid = angle + sweep / 2.0;
        
        fprintf(gp, "set object %d circle at 0,0 size 1 arc [%.1f:%.1f] fc rgb '%s' fs solid 1 border lc rgb '#222222' lw 0.5\n",
                obj_id++, angle, angle + sweep, colors[8]);
        
        fprintf(gp, "set label %d \"Другие\\n%.1f%%\" at 0.65*cos(%.3f*pi/180),0.65*sin(%.3f*pi/180) center font 'Arial,9' textcolor rgb '#000000'\n",
                label_id++, pct * 100.0, mid, mid);
    }
    
    fprintf(gp, "\nplot NaN notitle\n");
    fclose(gp);

    FILE *wrapper = fopen("run_gnuplot.sh", "w");
    if (wrapper) {
        fprintf(wrapper, "#!/bin/bash\n");
        fprintf(wrapper, "export LANG=ru_RU.UTF-8\n");
        fprintf(wrapper, "export LC_ALL=ru_RU.UTF-8\n");
        fprintf(wrapper, "gnuplot pie_script.gp\n");
        fclose(wrapper);
        system("chmod +x run_gnuplot.sh");
        system("./run_gnuplot.sh 2>/dev/null");
        unlink("run_gnuplot.sh");
    } else {
        system("export LANG=ru_RU.UTF-8; export LC_ALL=ru_RU.UTF-8; gnuplot pie_script.gp 2>/dev/null");
    }
    
    unlink("pie_script.gp");
}

void print_text_report() {
    printf("\n");
    printf("========================================\n");
    printf("ОТЧЕТ ПО СКАНИРОВАНИЮ BIT TORRENT РОЯ\n");
    printf("========================================\n\n");
    
    printf("Всего обработано IP: %d\n", total_ips);
    printf("IP с координатами: %d (%.1f%%)\n\n", 
           ips_with_coords, (float)ips_with_coords/total_ips*100);
    
    printf("--- ТОП-10 СТРАН ---\n");
    for (int i = 0; i < (country_count < 10 ? country_count : 10); i++) {
        printf("%2d. %-20s %d (%.1f%%)\n", 
               i+1, countries[i].name, countries[i].count,
               countries[i].percentage);
    }
    
    printf("\n--- ТОП-10 ПРОВАЙДЕРОВ (ASO) ---\n");
    for (int i = 0; i < (asn_count < 10 ? asn_count : 10); i++) {
        printf("%2d. %-30s %d (%.1f%%)\n", 
               i+1, asns[i].name, asns[i].count,
               asns[i].percentage);
    }
}

int main() {
    char line[MAX_LINE];
    char country[100];
    char asn[100];
    char lat_str[50];
    
    while (fgets(line, sizeof(line), stdin)) {
        total_ips++;

        if (extract_value(line, "latitude", lat_str, sizeof(lat_str))) {
            if (strcmp(lat_str, "null") != 0) {
                ips_with_coords++;
            }
        }
        
        if (extract_value(line, "country", country, sizeof(country))) {
            if (strcmp(country, "null") != 0) {
                int idx = find_or_add_counter(countries, &country_count, country);
                if (idx >= 0) countries[idx].count++;
            }
        }
        
        if (extract_value(line, "aso", asn, sizeof(asn))) {
            if (strcmp(asn, "null") != 0) {
                int idx = find_or_add_counter(asns, &asn_count, asn);
                if (idx >= 0) asns[idx].count++;
            }
        }
    }
    
    sort_counters(countries, country_count);
    sort_counters(asns, asn_count);
    calculate_percentages(countries, country_count, total_ips);
    calculate_percentages(asns, asn_count, total_ips);
    print_text_report();
    
    system("mkdir -p plots");
    printf("\n--- ГЕНЕРАЦИЯ ГРАФИКОВ ---\n");
    generate_bar_chart(countries, country_count, 
                      "Топ стран по количеству пиров BitTorrent", 
                      "plots/countries_bar.png", 
                      "Страна", "Количество пиров");
    generate_bar_chart(asns, asn_count, 
                      "Топ провайдеров (ASO) по количеству пиров BitTorrent", 
                      "plots/asns_bar.png", 
                      "Провайдер", "Количество пиров");
    generate_pie_chart(countries, country_count,
                      "Распределение пиров по странам",
                      "plots/countries_pie.png");
    generate_pie_chart(asns, asn_count,
                      "Распределение пиров по провайдерам",
                      "plots/asns_pie.png");
    
    
    FILE *html = fopen("plots/report.html", "w");
    if (html) {
        fprintf(html, "<!DOCTYPE html>\n");
        fprintf(html, "<html>\n<head>\n");
        fprintf(html, "<title>BitTorrent Peers Statistics Report</title>\n");
        fprintf(html, "<style>\n");
        fprintf(html, "body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }\n");
        fprintf(html, ".container { max-width: 1200px; margin: 0 auto; }\n");
        fprintf(html, "h1 { color: #333; text-align: center; }\n");
        fprintf(html, ".stats { background: white; padding: 20px; border-radius: 10px; margin: 20px 0; }\n");
        fprintf(html, ".chart-container { display: flex; flex-wrap: wrap; justify-content: center; gap: 20px; }\n");
        fprintf(html, ".chart-item { background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }\n");
        fprintf(html, "img { max-width: 100%%; height: auto; }\n");
        fprintf(html, "table { width: 100%%; border-collapse: collapse; margin: 20px 0; }\n");
        fprintf(html, "th, td { padding: 10px; text-align: left; border-bottom: 1px solid #ddd; }\n");
        fprintf(html, "th { background: #3498db; color: white; }\n");
        fprintf(html, "tr:hover { background: #f5f5f5; }\n");
        fprintf(html, "</style>\n");
        fprintf(html, "</head>\n<body>\n");
        fprintf(html, "<div class='container'>\n");
        
        fprintf(html, "<h1>📊 BitTorrent Peers Statistics Report</h1>\n");
        
        fprintf(html, "<div class='stats'>\n");
        fprintf(html, "<h2>Общая статистика</h2>\n");
        fprintf(html, "<p>Всего IP: <strong>%d</strong></p>\n", total_ips);
        fprintf(html, "<p>IP с координатами: <strong>%d (%.1f%%)</strong></p>\n", 
                ips_with_coords, (float)ips_with_coords/total_ips*100);
        fprintf(html, "</div>\n");
        
        fprintf(html, "<div class='stats'>\n");
        fprintf(html, "<h2>Топ стран</h2>\n");
        fprintf(html, "<table>\n");
        fprintf(html, "<tr><th>#</th><th>Страна</th><th>Количество</th><th>Процент</th></tr>\n");
        for (int i = 0; i < (country_count < 20 ? country_count : 20); i++) {
            fprintf(html, "<tr><td>%d</td><td>%s</td><td>%d</td><td>%.1f%%</td></tr>\n", 
                    i+1, countries[i].name, countries[i].count, countries[i].percentage);
        }
        fprintf(html, "</table>\n");
        fprintf(html, "<h2>Топ провайдеров</h2>\n");
        fprintf(html, "<table>\n");
        fprintf(html, "<tr><th>#</th><th>Провайдер</th><th>Количество</th><th>Процент</th></tr>\n");
        for (int i = 0; i < (asn_count < 20 ? asn_count : 20); i++) {
            fprintf(html, "<tr><td>%d</td><td>%s</td><td>%d</td><td>%.1f%%</td></tr>\n", 
                    i+1, asns[i].name, asns[i].count, asns[i].percentage);
        }
        fprintf(html, "</table>\n");
        fprintf(html, "</div>\n");
        
        
        fprintf(html, "<h2>Визуализация</h2>\n");
        fprintf(html, "<div class='chart-container'>\n");
        fprintf(html, "<div class='chart-item'><h3>Страны (столбцы)</h3><img src='countries_bar.png' alt='Countries Bar Chart'></div>\n");
        fprintf(html, "<div class='chart-item'><h3>Страны (круговая)</h3><img src='countries_pie.png' alt='Countries Pie Chart'></div>\n");
        fprintf(html, "<div class='chart-item'><h3>Провайдеры (столбцы)</h3><img src='asns_bar.png' alt='ASNs Bar Chart'></div>\n");
        fprintf(html, "<div class='chart-item'><h3>Провайдеры (круговая)</h3><img src='asns_pie.png' alt='ASNs Pie Chart'></div>\n");
        fprintf(html, "</div>\n");
        
        fprintf(html, "</div>\n");
        fprintf(html, "</body>\n</html>\n");
        fclose(html);
        printf("HTML отчет сохранен: plots/report.html\n");
    }
    printf("Для просмотра отчета откройте plots/report.html в браузере\n");
    
    return 0;
}