#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

int
main(void)
{
	int pipeToChild[2];
	int pipeToParent[2];

	if (pipe(pipeToChild) != 0) {
		fprintf(stderr, "Pipe1 creation failed\n");
		return EXIT_FAILURE;
	}
	if (pipe(pipeToParent) != 0) {
		close(pipeToChild[0]);
		close(pipeToChild[1]);
		fprintf(stderr,
		        "pipeToParent creation failed, pipeToChild closed\n");
		return EXIT_FAILURE;
	}
	printf("Hola, soy PID %d:\n", getpid());
	printf("  - primer pipe me devuelve: [%d, %d]\n",
	       pipeToChild[0],
	       pipeToChild[1]);
	printf("  - segundo pipe me devuelve: [%d, %d]\n",
	       pipeToParent[0],
	       pipeToParent[1]);
	size_t msg_length = sizeof(long);
	long buffer;
	int pid = fork();
	if (pid < 0) {
		// fork failed
		close(pipeToChild[0]);
		close(pipeToChild[1]);
		close(pipeToParent[0]);
		close(pipeToParent[1]);
		return EXIT_FAILURE;

	} else if (pid == 0) {
		// child process
		close(pipeToChild[1]);
		close(pipeToParent[0]);
		ssize_t read_value = read(pipeToChild[0], &buffer, msg_length);
		if (read_value == -1) {
			close(pipeToChild[0]);
			close(pipeToParent[1]);
			return EXIT_FAILURE;
		}
		printf("\nDonde fork me devuelve %d:\n", pid);
		printf("  - getpid me devuelve: %d\n", getpid());
		printf("  - getppid me devuelve: %d\n", getppid());
		printf("  - recibo valor %ld vía fd=%d\n", buffer, pipeToChild[0]);
		close(pipeToChild[0]);
		ssize_t write_value = write(pipeToParent[1], &buffer, msg_length);
		printf("  - reenvío valor en fd=%d y termino\n", pipeToParent[1]);
		close(pipeToParent[1]);
		return write_value != -1 ? EXIT_SUCCESS : EXIT_FAILURE;

	} else {
		// parent process
		close(pipeToChild[0]);
		close(pipeToParent[1]);
		srandom(time(NULL));
		long number = random();
		printf("\nDonde fork me devuelve %d:\n", pid);
		printf("  - getpid me devuelve: %d\n", getpid());
		printf("  - getppid me devuelve: %d\n", getppid());
		printf("  - random me devuelve: %ld\n", number);
		printf("  - envío valor %ld a través de fd=%d\n",
		       number,
		       pipeToChild[1]);
		if (write(pipeToChild[1], &number, msg_length) == -1) {
			// write failed
			close(pipeToChild[1]);
			close(pipeToParent[0]);
			fprintf(stderr, "writing to child failed\n");
			return EXIT_FAILURE;
		};

		ssize_t read_value = read(pipeToParent[0], &buffer, msg_length);
		close(pipeToChild[1]);
		close(pipeToParent[0]);
		if (read_value == -1) {
			fprintf(stderr, "reading from child failed\n");
			wait(NULL);
			return EXIT_FAILURE;
		} else {
			printf("\nHola, de nuevo PID %d:\n", getpid());
			printf("  - recibí valor %ld vía fd=%d\n",
			       buffer,
			       pipeToParent[0]);
			wait(NULL);
			return EXIT_SUCCESS;
		}
	}
}