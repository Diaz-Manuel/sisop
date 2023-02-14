#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef NARGS
#define NARGS 4
#endif

void fork_exec(char *, char **);

void
fork_exec(char *exec_filename, char **exec_args)
{
	int pid = fork();
	if (pid == 0) {
		// child process
		execvp(exec_filename, exec_args);
		exit(EXIT_FAILURE);
	} else if (pid < 0) {
		exit(EXIT_FAILURE);
	}
	// parent process
	wait(NULL);
}

int
main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Invalid argument count\n");
		exit(EXIT_FAILURE);
	}
	char *exec_args[NARGS + 2] = { 0 };
	exec_args[0] = argv[1];
	int exec_argc = 1;

	char *buffers[NARGS] = { 0 };
	size_t buffers_size[NARGS] = { 0 };
	ssize_t read_chars;
	char *current_buffer = buffers[0];
	while ((read_chars = getline(&current_buffer,
	                             &buffers_size[exec_argc - 1],
	                             stdin)) != -1) {
		if (current_buffer[read_chars - 1] ==
		    '\n')  // the last line might not end with \n
		{
			current_buffer[read_chars - 1] =
			        '\0';  // replace the terminating \n
		}
		exec_args[exec_argc] = current_buffer;
		++exec_argc;
		current_buffer = buffers[exec_argc - 1];
		if (exec_argc <= NARGS) {
			continue;
		}
		exec_argc = 1;
		fork_exec(argv[1], exec_args);
	}
	if (exec_argc > 1) {
		exec_args[exec_argc] = 0;
		fork_exec(argv[1], exec_args);
	}

	for (int i = 0; i < NARGS; i++) {
		free(buffers[i]);
	}
	return EXIT_SUCCESS;
}
