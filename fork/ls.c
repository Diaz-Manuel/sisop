#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

void
getftype(mode_t d_type, char *type)
{
	switch (d_type & S_IFMT) {
	case S_IFREG:
		*type = '-';
		break;
	case S_IFBLK:
		*type = 'b';
		break;
	case S_IFCHR:
		*type = 'c';
		break;
	case S_IFDIR:
		*type = 'd';
		break;
	case S_IFLNK:
		*type = 'l';
		break;
	case S_IFIFO:
		*type = 'p';
		break;
	case S_IFSOCK:
		*type = 's';
		break;
	default:  // should never reach here
		*type = ' ';
		break;
	}
}

char * /* Return ls(1)-style string for file permissions mask */
filePermStr(mode_t perm)
{
	// source: https://man7.org/tlpi/code/online/dist/files/file_perms.c.html
	static char str[10];
	snprintf(str,
	         10,
	         "%c%c%c%c%c%c%c%c%c",
	         (perm & S_IRUSR) ? 'r' : '-',
	         (perm & S_IWUSR) ? 'w' : '-',
	         (perm & S_IXUSR) ? ((perm & S_ISUID) ? 's' : 'x')
	                          : ((perm & S_ISUID) ? 'S' : '-'),
	         (perm & S_IRGRP) ? 'r' : '-',
	         (perm & S_IWGRP) ? 'w' : '-',
	         (perm & S_IXGRP) ? ((perm & S_ISGID) ? 's' : 'x')
	                          : ((perm & S_ISGID) ? 'S' : '-'),
	         (perm & S_IROTH) ? 'r' : '-',
	         (perm & S_IWOTH) ? 'w' : '-',
	         (perm & S_IXOTH) ? ((perm & S_ISVTX) ? 't' : 'x')
	                          : ((perm & S_ISVTX) ? 'T' : '-'));
	return str;
}

int
main(int argc, char *argv[])
{
	DIR *directory = opendir(".");
	if (directory == NULL) {
		perror("opendir failed to open current dir");
		exit(EXIT_FAILURE);
	}
	struct dirent *entry;
	struct stat stats;
	char type;
	char *mode;
	char date[13];
	char link_buffer[PATH_MAX + 1];
	ssize_t bytes_read = 0;
	int error = 0;

	while ((entry = readdir(directory))) {
		lstat(entry->d_name, &stats);
		getftype(stats.st_mode, &type);
		mode = filePermStr(stats.st_mode);

		struct tm *tm_time =
		        localtime(&stats.st_mtim.tv_sec);  // linux post 2.6
		// struct tm *tm_time = localtime(&stats.st_mtime); // mac and linux pre 2.6
		strftime(date, 13, "%b %d %R", tm_time);
		if (type == 'l') {  // file is a symlink
			bytes_read =
			        readlink(entry->d_name, link_buffer, PATH_MAX + 1);
			if (bytes_read == -1) {
				error = errno;  // return error on ls termination
				perror("readlink failed reading symlink file");
				continue;  // don't print line on error
			}
			link_buffer[bytes_read] = '\0';
			printf("%c%s\t%ld\t%d\t%d\t%ld\t%s\t%s -> %s\n",
			       type,
			       mode,
			       stats.st_nlink,
			       stats.st_uid,
			       stats.st_gid,
			       stats.st_size,
			       date,
			       entry->d_name,
			       link_buffer);
		} else {
			printf("%c%s\t%ld\t%d\t%d\t%ld\t%s\t%s\n",
			       type,
			       mode,
			       stats.st_nlink,
			       stats.st_uid,
			       stats.st_gid,
			       stats.st_size,
			       date,
			       entry->d_name);
		}
	}
	closedir(directory);
	if (error != 0)
		return error;
	return EXIT_SUCCESS;
}
