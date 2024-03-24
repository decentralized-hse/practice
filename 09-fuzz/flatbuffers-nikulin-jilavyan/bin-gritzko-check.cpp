#include "bin-gritzko-check.h"

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

bool check_str(const char *str, int len, const char *name) {
  int i = 0;
  while (i < len && str[i] != 0)
    i++;
  while (i < len && str[i] == 0)
    i++;
  return i == len;
}

bool validate_input(const char *file_name) {
  struct Student student;
  int fd = open(file_name, O_RDONLY);
  if (fd == -1) {
    return false;
  }
  ssize_t rd = 0;
  while (RECSIZE == (rd = read(fd, &student, RECSIZE))) {
    if (!check_str(student.name, 32, "name")) {
        goto cleanup;
    }
    if (!check_str(student.login, 16, "login")) {
        goto cleanup;
    }
    if (!check_str(student.group, 8, "group")) {
        goto cleanup;
    }
    for (int p = 0; p < 8; p++) {
      if (student.practice[p] != 0 && student.practice[p] != 1) {
        goto cleanup;
      }
    }
    if (!check_str(student.project.repo, 59, "repo")) {
        goto cleanup;
    }
    if (student.project.mark < 0 || student.project.mark > 10)
      goto cleanup;
    if (student.mark < 0 || student.mark > 10)
      goto cleanup;
  }
  cleanup:
  close(fd);
  return rd == 0;
}
