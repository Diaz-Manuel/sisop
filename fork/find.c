#define _GNU_SOURCE
#include <sys/types.h>
#include <getopt.h>
#include <dirent.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

void find(int, const char *, char *(const char *, const char *), const char *);

int
main(int argc, char *argv[])
{
	const char *searched_string;
	char *(*search_fn)(const char *, const char *);

	if (argc == 2) {
		search_fn = &strstr;
		searched_string = argv[1];
	} else if (argc == 3) {
		int optional_arg = getopt(argc, argv, "i");
		if (optional_arg != 'i' ||
		    getopt(argc, argv, "") != -1)  // takes exactly one optional arg
		{
			exit(EXIT_FAILURE);
		}
		search_fn = &strcasestr;
		searched_string = argv[2];
	} else {
		fprintf(stderr, "cantidad inválida de argumentos");
		exit(EXIT_FAILURE);
	}

	DIR *directory = opendir(".");
	if (directory == NULL) {
		perror("error con opendir");
		exit(EXIT_FAILURE);
	}
	int directory_fd = dirfd(directory);
	find(directory_fd, ".", *search_fn, searched_string);
}

void
find(int directory_fd,
     const char *path,
     char *search_fn(const char *, const char *),
     const char *searched_str)
{
	DIR *directory = fdopendir(directory_fd);
	char subdir_path[PATH_MAX + 1];
	struct dirent *entry;
	while ((entry = readdir(directory))) {
		if (strcmp(entry->d_name, ".") == 0 ||
		    strcmp(entry->d_name, "..") == 0) {
			continue;
		}
		if (entry->d_type == DT_REG || entry->d_type == DT_DIR) {
			// ver si el nombre del archivo/directorio contiene el string buscado
			if (search_fn(entry->d_name, searched_str) != NULL) {
				printf("%s/%s\n", path, entry->d_name);
			}
		}
		if (entry->d_type == DT_DIR) {
			// llamar recursivamente para cada subdirectorio
			int subdir_fd = openat(directory_fd,
			                       entry->d_name,
			                       O_DIRECTORY | O_RDONLY);
			snprintf(subdir_path,
			         PATH_MAX + 1,
			         "%s/%s",
			         path,
			         entry->d_name);
			find(subdir_fd, subdir_path, search_fn, searched_str);
		}
		// no seguir symlinks
	}
	closedir(directory);
}