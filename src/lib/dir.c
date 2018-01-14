/* =============================================================================
 * Pure64 -- a 64-bit OS/software loader written in Assembly for x86-64 systems
 * Copyright (C) 2008-2017 Return Infinity -- see LICENSE.TXT
 * =============================================================================
 */

#include <pure64/dir.h>
#include <pure64/file.h>
#include <pure64/path.h>

#include "misc.h"

#include <stdlib.h>
#include <string.h>

void pure64_dir_init(struct pure64_dir *dir) {
	dir->name_size = 0;
	dir->subdir_count = 0;
	dir->file_count = 0;
	dir->name = NULL;
	dir->subdirs = NULL;
	dir->files = NULL;
}

void pure64_dir_free(struct pure64_dir *dir) {

	free(dir->name);

	for (uint64_t i = 0; i < dir->subdir_count; i++)
		pure64_dir_free(&dir->subdirs[i]);

	for (uint64_t i = 0; i < dir->file_count; i++)
		pure64_file_free(&dir->files[i]);

	free(dir->subdirs);
	free(dir->files);

	dir->name = NULL;
	dir->subdirs = NULL;
	dir->files = NULL;
}

int pure64_dir_add_file(struct pure64_dir *dir, const char *name) {

	int err;
	struct pure64_file *files;
	uint64_t files_size;

	if (pure64_dir_name_exists(dir, name))
		return -1;

	files_size = dir->file_count + 1;
	files_size *= sizeof(dir->files[0]);

	files = dir->files;

	files = realloc(files, files_size);
	if (files == NULL) {
		return -1;
	}

	pure64_file_init(&files[dir->file_count]);

	err = pure64_file_set_name(&files[dir->file_count], name);
	if (err != 0) {
		pure64_file_free(&files[dir->file_count]);
		return -1;
	}

	dir->files = files;
	dir->file_count++;

	return 0;
}

int pure64_dir_add_subdir(struct pure64_dir *dir, const char *name) {

	int err;
	struct pure64_dir *subdirs;
	uint64_t subdirs_size;

	if (pure64_dir_name_exists(dir, name))
		return -1;

	subdirs_size = dir->subdir_count + 1;
	subdirs_size *= sizeof(dir->subdirs[0]);

	subdirs = dir->subdirs;

	subdirs = realloc(subdirs, subdirs_size);
	if (subdirs == NULL) {
		return -1;
	}

	pure64_dir_init(&subdirs[dir->subdir_count]);

	err = pure64_dir_set_name(&subdirs[dir->subdir_count], name);
	if (err != 0) {
		pure64_dir_free(&subdirs[dir->subdir_count]);
		return -1;
	}

	dir->subdirs = subdirs;
	dir->subdir_count++;

	return 0;
}

int pure64_dir_export(struct pure64_dir *dir, FILE *out) {

	int err = 0;
	err |= encode_uint64(dir->name_size, out);
	err |= encode_uint64(dir->subdir_count, out);
	err |= encode_uint64(dir->file_count, out);
	if (err != 0)
		return -1;

	if (fwrite(dir->name, 1, dir->name_size, out) != dir->name_size)
		return -1;

	for (uint64_t i = 0; i < dir->subdir_count; i++) {
		err = pure64_dir_export(&dir->subdirs[i], out);
		if (err != 0)
			return -1;
	}

	for (uint64_t i = 0; i < dir->file_count; i++) {
		err = pure64_file_export(&dir->files[i], out);
		if (err != 0)
			return -1;
	}

	return 0;
}

int pure64_dir_import(struct pure64_dir *dir, FILE *in) {

	int err = 0;
	err |= decode_uint64(&dir->name_size, in);
	err |= decode_uint64(&dir->subdir_count, in);
	err |= decode_uint64(&dir->file_count, in);
	if (err != 0)
		return -1;

	dir->name = malloc(dir->name_size + 1);
	dir->subdirs = malloc(dir->subdir_count * sizeof(dir->subdirs[0]));
	dir->files = malloc(dir->file_count * sizeof(dir->files[0]));
	if ((dir->name == NULL)
	 || (dir->subdirs == NULL)
	 || (dir->files == NULL)) {
		free(dir->name);
		free(dir->subdirs);
		free(dir->files);
		return -1;
	}

	if (fread(dir->name, 1, dir->name_size, in) != dir->name_size)
		return -1;

	dir->name[dir->name_size] = 0;

	for (uint64_t i = 0; i < dir->subdir_count; i++)
		pure64_dir_init(&dir->subdirs[i]);

	for (uint64_t i = 0; i < dir->file_count; i++)
		pure64_file_init(&dir->files[i]);

	for (uint64_t i = 0; i < dir->subdir_count; i++) {
		err = pure64_dir_import(&dir->subdirs[i], in);
		if (err != 0)
			return -1;
	}

	for (uint64_t i = 0; i < dir->file_count; i++) {
		err = pure64_file_import(&dir->files[i], in);
		if (err != 0)
			return -1;
	}

	return 0;
}

int pure64_dir_make_subdir(struct pure64_dir *root, const char *path_str) {

	int err;
	const char *name;
	unsigned int name_count;
	unsigned int subdir_count;
	unsigned int i;
	unsigned int j;
	struct pure64_path path;
	struct pure64_dir *parent_dir;
	struct pure64_dir *subdir;

	pure64_path_init(&path);

	err = pure64_path_parse(&path, path_str);
	if (err != 0) {
		pure64_path_free(&path);
		return -1;
	}

	err = pure64_path_normalize(&path);
	if (err != 0) {
		pure64_path_free(&path);
		return -1;
	}

	parent_dir = root;

	name_count = pure64_path_get_name_count(&path);

	if (name_count == 0) {
		pure64_path_free(&path);
		return -1;
	}

	for (i = 0; i < (name_count - 1); i++) {

		name = pure64_path_get_name(&path, i);
		if (name == NULL) {
			pure64_path_free(&path);
			return -1;
		}

		subdir_count = parent_dir->subdir_count;

		for (j = 0; j < subdir_count; j++) {
			subdir = &parent_dir->subdirs[j];
			if (subdir == NULL) {
				continue;
			} else if (strcmp(subdir->name, name) == 0) {
				parent_dir = subdir;
				break;
			}
		}

		if (j >= subdir_count) {
			/* not found */
			pure64_path_free(&path);
			return -1;
		}
	}

	if (i != (name_count - 1)) {
		pure64_path_free(&path);
		return -1;
	}

	err = pure64_dir_add_subdir(parent_dir, path.name_array[i].data);
	if (err != 0) {
		pure64_path_free(&path);
		return -1;
	}

	pure64_path_free(&path);

	return 0;
}

int pure64_dir_make_file(struct pure64_dir *root, const char *path_str) {

	int err;
	const char *name;
	unsigned int name_count;
	unsigned int subdir_count;
	unsigned int i;
	unsigned int j;
	struct pure64_path path;
	struct pure64_dir *parent_dir;
	struct pure64_dir *subdir;

	pure64_path_init(&path);

	err = pure64_path_parse(&path, path_str);
	if (err != 0) {
		pure64_path_free(&path);
		return -1;
	}

	err = pure64_path_normalize(&path);
	if (err != 0) {
		pure64_path_free(&path);
		return -1;
	}

	parent_dir = root;

	name_count = pure64_path_get_name_count(&path);

	if (name_count == 0) {
		pure64_path_free(&path);
		return -1;
	}

	for (i = 0; i < (name_count - 1); i++) {

		name = pure64_path_get_name(&path, i);
		if (name == NULL) {
			pure64_path_free(&path);
			return -1;
		}

		subdir_count = parent_dir->subdir_count;

		for (j = 0; j < subdir_count; j++) {
			subdir = &parent_dir->subdirs[j];
			if (subdir == NULL) {
				continue;
			} else if (strcmp(subdir->name, name) == 0) {
				parent_dir = subdir;
				break;
			}
		}

		if (j >= subdir_count) {
			/* not found */
			pure64_path_free(&path);
			return -1;
		}
	}

	if (i != (name_count - 1)) {
		pure64_path_free(&path);
		return -1;
	}

	err = pure64_dir_add_file(parent_dir, path.name_array[i].data);
	if (err != 0) {
		pure64_path_free(&path);
		return -1;
	}

	pure64_path_free(&path);

	return 0;
}

struct pure64_dir * pure64_dir_open_subdir(struct pure64_dir *root, const char *path_string) {

	int err;
	unsigned int i;
	unsigned int j;
	const char *name;
	unsigned int name_count;
	unsigned int subdir_count;
	struct pure64_path path;
	struct pure64_dir *parent_dir;

	pure64_path_init(&path);

	err = pure64_path_parse(&path, path_string);
	if (err != 0) {
		pure64_path_free(&path);
		return NULL;
	}

	err = pure64_path_normalize(&path);
	if (err != 0) {
		pure64_path_free(&path);
		return NULL;
	}

	parent_dir = root;

	name_count = pure64_path_get_name_count(&path);

	for (i = 0; i < name_count; i++) {
		name = pure64_path_get_name(&path, i);
		if (name == NULL) {
			pure64_path_free(&path);
			return NULL;
		}

		subdir_count = parent_dir->subdir_count;
		for (j = 0; j < subdir_count; j++) {
			if (strcmp(parent_dir->subdirs[j].name, name) == 0) {
				parent_dir = &parent_dir->subdirs[j];
				break;
			}
		}

		if (j >= subdir_count) {
			pure64_path_free(&path);
			return NULL;
		}
	}

	pure64_path_free(&path);

	return parent_dir;
}

struct pure64_file * pure64_dir_open_file(struct pure64_dir *root, const char *path_string) {

	int err;
	unsigned int i;
	unsigned int j;
	const char *name;
	unsigned int subdir_count;
	unsigned int name_count;
	struct pure64_path path;
	struct pure64_dir *parent_dir;

	pure64_path_init(&path);

	err = pure64_path_parse(&path, path_string);
	if (err != 0) {
		pure64_path_free(&path);
		return NULL;
	}

	err = pure64_path_normalize(&path);
	if (err != 0) {
		pure64_path_free(&path);
		return NULL;
	}

	parent_dir = root;

	name_count = pure64_path_get_name_count(&path);

	if (name_count == 0) {
		/* there must be at least one
		 * entry name in the path */
		return NULL;
	}

	for (i = 0; i < (name_count - 1); i++) {
		name = pure64_path_get_name(&path, i);
		if (name == NULL) {
			pure64_path_free(&path);
			return NULL;
		}

		subdir_count = parent_dir->subdir_count;
		for (j = 0; j < subdir_count; j++) {
			if (strcmp(parent_dir->subdirs[j].name, name) == 0) {
				parent_dir = &parent_dir->subdirs[j];
				break;
			}
		}

		if (j >= subdir_count) {
			pure64_path_free(&path);
			return NULL;
		}
	}

	name = pure64_path_get_name(&path, i);
	if (name == NULL) {
		/* This shouldn't happen, so
		 * this check is a precaution */
		return NULL;
	}

	/* 'name' is now the basename of the file. */

	for (j = 0; j < parent_dir->file_count; j++) {
		if (strcmp(parent_dir->files[j].name, name) == 0) {
			pure64_path_free(&path);
			return &parent_dir->files[j];
		}
	}

	/* file not found */

	pure64_path_free(&path);

	return NULL;
}

bool pure64_dir_name_exists(const struct pure64_dir *dir, const char *name) {

	uint64_t i;

	for (i = 0; i < dir->file_count; i++) {
		if (strcmp(dir->files[i].name, name) == 0)
			return true;
	}

	for (i = 0; i < dir->subdir_count; i++) {
		if (strcmp(dir->subdirs[i].name, name) == 0)
			return true;
	}

	return false;
}

int pure64_dir_set_name(struct pure64_dir *dir, const char *name) {

	char *tmp_name;
	uint64_t name_size;

	name_size = strlen(name);

	tmp_name = malloc(name_size + 1);
	if (tmp_name == NULL)
		return -1;

	memcpy(tmp_name, name, name_size);

	tmp_name[name_size] = 0;

	free(dir->name);

	dir->name = tmp_name;
	dir->name_size = name_size;

	return 0;
}
