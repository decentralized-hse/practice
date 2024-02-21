#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "dirent.h"
#include "objutil.h"

#define MAX_DEPTH 16

int rm(unsigned char *root_hash, unsigned char *dir_name, unsigned char *dir_hash) {
	if (strcmp(dir_name, ".") == 0) {
		strcpy(dir_hash, root_hash);
		return 0;
	}

	uint32_t k = 0;
	for (size_t i = 0; i < strlen(dir_name); i++) {
		if (dir_name[i] == '/') {
			k++;
		}
	}
	uint32_t pos = 0, old_pos = 0, name_len = strlen(dir_name);
	strcpy(dir_hash, root_hash);
	dir_iter_t cur_dir;
	dir_ent_t *dir_entry = NULL;
	uint32_t i = 0;

	while(pos < name_len && i < k) {
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
			printf("Wrong hash");
			return -1;
		}

		strcpy(dir_hash, dir_entry->hash);
		close_dir(&cur_dir);
		i++;
	}

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

	dir_ent_t *ent = NULL;
  	uint32_t n_dir_entries = 0;
	size_t  dir_obj_len = 1;
	dirent_foreach(&cur_dir, ent) {
		if (strncmp(&dir_name[old_pos], ent->name, strlen(ent->name) - 1) != 0) {
			++n_dir_entries;
			dir_obj_len += strlen(ent->name) + HASH_LEN + 2;
		}
	}

 	dir_ent_t* materialized_dir_view = malloc(n_dir_entries * sizeof(dir_ent_t));
	reset_dir_iter(&cur_dir);
  	uint32_t ent_idx = 0;
	ent = NULL;
	dirent_foreach(&cur_dir, ent) {
		if (strncmp(&dir_name[old_pos], ent->name, strlen(ent->name) - 1) != 0) {
			materialized_dir_view[ent_idx] = *ent;
			++ent_idx;
		}
	}
	ent = NULL;
	close_dir(&cur_dir);

	unsigned char* buf = malloc(dir_obj_len + 1); /* +1 for final \0 */
	for (size_t i = 0, offset = 0; i < n_dir_entries; i++) {
		offset += sprintf(buf + offset, "%s\t%s\n", materialized_dir_view[i].name, materialized_dir_view[i].hash);
	}

	free(materialized_dir_view);
	buf[dir_obj_len - 2] = '\n';
	buf[dir_obj_len - 1] = '\0';
	int res = write_object(buf, dir_obj_len - 1, dir_hash);
	if (strlen(buf) > 0) {
		free(buf);
	}

	return res;
}

int main(int argc, char** argv) {
  	if (argc != 3) {
    	printf("Invalid arguments. Usage: ls <dir> <hash>\n");
    	return -1;
  	}

	unsigned char dir_hash[HASH_LEN];
	if (rm(argv[2], argv[1], dir_hash) == -1) {
		printf("Something went wrong");
		return -1;
	}

	printf("%s\n", dir_hash);
}