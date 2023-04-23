#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

struct Student {
        char    name[32];
        char    login[16];
        char    group[8];
        uint8_t practice[8];
        struct {
            char    repo[59];
            uint8_t mark;
        } project;
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
    struct Student test_student = {
        .name = "asd ASD \t\n\r \\ \'\"",
        .login = "NaN",
        .group = "\\",
        .practice = {1, 1, 0, 0, 0, 0, 0, 0},
        .project = {
            .repo = ",./;'[]\\-=\u02C4\u0502\u0024",
            .mark = 9,
        },
        .mark = 8,
    };
    int fd = open("test.bin", O_CREAT|O_RDWR|O_TRUNC, S_IRWXU);
    write(fd, &ivanov, sizeof(struct Student));
    write(fd, &test_student, sizeof(struct Student));
    close(fd);
    return 0;
}
