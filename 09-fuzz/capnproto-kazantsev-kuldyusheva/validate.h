#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>

#include <fstream>
#include <string>
#include <cstring>

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

bool fail(const char *where) {
  fprintf(stderr, "Malformed input at %s\n", where);
  return false;
}

bool check_str(const char *str, int len, const char *name) {
  int i = 0;
  while (i < len && str[i] != 0)
    i++;
  while (i < len && str[i] == 0)
    i++;
  if (i != len)
    return fail(name);
  return true;
}

bool validateData(std::string name) {
    struct Student student;
    int fd = open((name + ".bin").c_str(), O_RDONLY);
    if (fd == -1) {
      return fail("file");
    }
    ssize_t rd = 0;
    bool r;
    while (RECSIZE == (rd = read(fd, &student, RECSIZE))) {
      r = true;
      if (!check_str(student.name, 32, "name")) {
        return fail("name");
      }
      if (!check_str(student.login, 16, "login")) {
        return fail("login");
      }
      if (!check_str(student.group, 8, "group")) {
        return fail("group");
      }
      for (int p = 0; p < 8; p++) {
        if (student.practice[p] != 0 && student.practice[p] != 1) {
          return fail("practice");
        }
      }
      if (!check_str(student.project.repo, 59, "repo")) {
        return fail("repo");
      }
      if (student.project.mark < 0 || student.project.mark > 10)
        return fail("project mark");
      if (student.mark < 0 || student.mark > 10 || isnan(student.mark))
        return fail("mark");
    }
    if (rd != 0) {
        return fail("student");
    }
    close(fd);
    return r;
}
