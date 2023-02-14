#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>


void printPrimeAndFork(int inputPipe,
                       unsigned long *prime_number,
                       int *outputPipe,
                       int *pid);
void sieve(int inputPipe);

void
printPrimeAndFork(int inputPipe,
                  unsigned long *prime_number,
                  int *outputPipe,
                  int *pid)
{
	if (read(inputPipe, prime_number, sizeof(long)) <= 0) {
		close(inputPipe);
		exit(EXIT_FAILURE);
	}
	printf("primo %lu\n", *prime_number);
	if (pipe(outputPipe) != 0) {
		close(inputPipe);
		exit(EXIT_FAILURE);
	}
	fflush(stdout);
	*pid = fork();
	if (*pid < 0) {
		close(inputPipe);
		close(outputPipe[0]);
		close(outputPipe[1]);
		exit(EXIT_FAILURE);
	}
}

void
sieve(int inputPipe)
{
	unsigned long prime_number;
	unsigned long current_number;
	int pid = 0;
	int outputPipe[2];
	printPrimeAndFork(inputPipe, &prime_number, outputPipe, &pid);

	while (pid == 0) {
		// keep creating children until the last prime was found
		close(inputPipe);
		close(outputPipe[1]);
		inputPipe = outputPipe[0];
		// printPrimeAndFork waits for parent to write
		// if parent closes the pipe, child will exit, ending the loop
		printPrimeAndFork(inputPipe, &prime_number, outputPipe, &pid);
	}
	close(outputPipe[0]);
	while (read(inputPipe, &current_number, sizeof(long)) > 0) {
		if (current_number % prime_number != 0) {
			ssize_t write_val = write(outputPipe[1],
			                          &current_number,
			                          sizeof(long));
			if (write_val <= 0) {
				break;
			}
		}
	}
	close(outputPipe[1]);
	close(inputPipe);
	wait(NULL);
}

int
main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Invalid argument count\n");
		exit(EXIT_FAILURE);
	}
	unsigned long n = strtoul(argv[1], NULL, 10);
	int pipeToSieve[2];
	if (pipe(pipeToSieve) != 0) {
		exit(EXIT_FAILURE);
	}
	int pid = fork();
	if (pid < 0) {
		close(pipeToSieve[0]);
		close(pipeToSieve[1]);
		exit(EXIT_FAILURE);
	} else if (pid > 0) {
		close(pipeToSieve[0]);
		for (unsigned long i = 2; i <= n; i++) {
			ssize_t write_val =
			        write(pipeToSieve[1], &i, sizeof(long));
			if (write_val <= 0) {
				close(pipeToSieve[1]);
				exit(EXIT_FAILURE);
			}
		}
		close(pipeToSieve[1]);
		wait(NULL);
	} else {
		close(pipeToSieve[1]);
		sieve(pipeToSieve[0]);
	}
	exit(EXIT_SUCCESS);
}