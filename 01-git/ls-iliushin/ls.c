#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "dirent.h"
#include "objutil.h"

#define MAX_DEPTH 16

int get_dir_hash(unsigned char *root_hash, unsigned char *dir_name, unsigned char *dir_hash) {
	if (strcmp(dir_name, ".") == 0) {
		strcpy(dir_hash, root_hash);
		return 0;
	}

	uint32_t pos = 0, old_pos = 0, name_len = strlen(dir_name);
	strcpy(dir_hash, root_hash);
	dir_iter_t cur_dir;
	dir_ent_t *dir_entry = NULL;

	do {
		open_dir_obj_by_hash(dir_hash, &cur_dir);

		old_pos = pos;
		while (dir_name[pos] != '\0') {
			if (dir_name[pos] == '/') {
				dir_name[pos] = '\0';
				pos += 1;
				break;
			}
			pos += 1;
		}

		dir_entry = find_in_dir(&cur_dir, &dir_name[old_pos]);
		if (dir_entry == NULL) {
			return -1;
		}

		strcpy(dir_hash, dir_entry->hash);
		close_dir(&cur_dir);
	}
	while (pos < name_len);

	return 0;
}

int ls(const unsigned char *dir_hash, uint32_t depth) {
	dir_ent_t *dir_entry = NULL;
	dir_iter_t cur_dir;
	unsigned char print_buf[MAX_NAME_LEN + MAX_DEPTH];
	uint32_t name_len;

	open_dir_obj_by_hash(dir_hash, &cur_dir);
	for (uint32_t i = 0; i < depth; i += 1) {
		print_buf[i] = '\t';
	}

	dirent_foreach(&cur_dir, dir_entry) {
		name_len = strlen(dir_entry->name) - 1;
		for (uint32_t i = 0; i < name_len; i++) {
			print_buf[depth + i] = dir_entry->name[i];
		}
		print_buf[depth + name_len] = '\0';

		printf("%s\n", print_buf);
		if (is_dir(dir_entry)) {
			ls(dir_entry->hash, depth + 1);
		}
	}

	close_dir(&cur_dir);

	return 0;
}

int main(int argc, char** argv) {
  	if (argc != 3) {
    	printf("Invalid arguments. Usage: ls <dir> <hash>\n");
    	return -1;
  	}

	unsigned char dir_hash[HASH_LEN];
	if (get_dir_hash(argv[2], argv[1], dir_hash) == -1) {
		return -1;
	}

  	return ls(dir_hash, 0);
}
