#include <sys/mman.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

bool
file_exists(char *filename)
{
	struct stat buffer;
	return (stat(filename, &buffer) == 0);
}

int
copy_file(int source_file, int target_file, size_t filesize)
{
	void *map_failed = (void *) -1;
	fputs("copy_file\n", stderr);

	void *source_map =
	        mmap(0, filesize, PROT_READ, MAP_SHARED, source_file, 0);
	if (source_map == map_failed) {
		perror("mmap failed allocating space for source_file");
		exit(4);
	}

	// resize target file
	ftruncate(target_file, filesize);
	void *target_map =
	        mmap(0, filesize, PROT_WRITE, MAP_SHARED, target_file, 0);
	if (target_map == map_failed) {
		perror("mmap failed allocating space for target_file");
		exit(4);
	}
	// write to target file
	memcpy(target_map, source_map, filesize);

	munmap(source_map, filesize);
	munmap(target_map, filesize);
	close(source_file);
	close(target_file);
	return 0;
}

int
main(int argc, char *argv[])
{
	if (argc != 3) {
		fputs("usage: cp source_file target_file\n", stderr);
		exit(1);
	}

	// if source_file doesnt exist, fail
	int source_file = open(argv[1], O_RDONLY);
	if (source_file < 0) {
		fprintf(stderr, "cp: Error opening file %s", argv[1]);
		exit(2);
	}

	// if target_file exists, fail
	if (file_exists(argv[2])) {
		fprintf(stderr,
		        "cp: Error target file %s already exists\n",
		        argv[2]);
		exit(3);
	}
	int target_file = open(argv[2], O_RDWR | O_CREAT, 0644);

	// find file size
	struct stat file_stats;
	stat(argv[1], &file_stats);
	return copy_file(source_file, target_file, file_stats.st_size);
}
