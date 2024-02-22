#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

struct Student {
  // имя может быть и короче 32 байт, тогда в хвосте 000
  // имя - валидный UTF-8
  char name[32];
  // ASCII [\w]+
  char login[16];
  char group[8];
  // 0/1, фактически bool
  uint8_t practice[8];
  struct {
    // URL
    char repo[59];
    uint8_t mark;
  } project;
  // 32 bit IEEE 754 float
  float mark;
};

int RECSIZE = sizeof(struct Student);

void fail(const char *where) {
  fprintf(stderr, "Malformed input at %s\n", where);
  exit(-1);
}

void check_str(const char *str, int len, const char *name) {
  int i = 0;
  while (i < len && str[i] != 0)
    i++;
  while (i < len && str[i] == 0)
    i++;
  if (i != len)
    fail(name);
}

int main(int argn, char **args) {
  for (int i = 1; i < argn; i++) {
    struct Student student;
    int fd = open(args[i], O_RDONLY);
    if (fd == -1) {
      perror(args[i]);
      return -2;
    }
    ssize_t rd = 0;
    while (RECSIZE == (rd = read(fd, &student, RECSIZE))) {
      check_str(student.name, 32, "name");
      check_str(student.login, 16, "login");
      check_str(student.group, 8, "group");
      for (int p = 0; p < 8; p++) {
        if (student.practice[p] != 0 && student.practice[p] != 1) {
          fail("practice");
        }
      }
      check_str(student.project.repo, 59, "repo");
      if (student.project.mark < 0 || student.project.mark > 10)
        fail("project mark");
      if (student.project.mark < 0 || student.project.mark > 10)
        fail("mark");
    }
    close(fd);
  }
  fprintf(stderr, "OK\n");
  return 0;
}
