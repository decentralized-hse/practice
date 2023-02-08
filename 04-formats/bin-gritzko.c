#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

struct Student {
        // имя может быть и короче 32 байт, тогда в хвосте 000
        // имя - валидный UTF-8
        char    name[32];
        // ASCII [\w]+
        char    login[16];
        char    group[8];
        // 0/1, фактически bool
        uint8_t practice[8];
        struct {
            // URL
            char    repo[63];
            uint8_t mark;
        } project;
        // 32 bit IEEE 754 float
        float   mark; 
};

int main(int argn, char** args) {
    struct Student ivanov = {
        .name = "Иван Иванов",
        .login = "ivanov",
        .group = "ИА-345",
        .practice = {1, 1, 0, 0, 0, 0, 0, 0},
        .project = {
            .repo = "github.com/decentralized-hse/practice/tree/main/04-formats",
            .mark = 9,
        },
        .mark = 8,
    };
    int fd = open("ivanov.bin", O_CREAT|O_RDWR, S_IRWXU);
    write(fd, &ivanov, sizeof(struct Student));
    close(fd);
    return 0;
}
