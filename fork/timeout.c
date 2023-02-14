#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>

int childpid;

void
show_help(void)
{
	puts("Usage: timeout DURATION COMMAND [ARG]...\n"
	     "\tor:  timeout --help\n"
	     "Start COMMAND, and kill it if still running after DURATION.\n\n"
	     "DURATION is an integer with NO suffix. The DURATION is in "
	     "seconds.\n"
	     "If the command times out, then exit with status 124.\n"
	     "Otherwise, exit with the status of COMMAND.  Upon timeout, sends "
	     "the TERM signal.");
}

void
set_timeout(int timeout_seconds)
{
	timer_t timerid;
	int ret = timer_create(CLOCK_BOOTTIME, NULL, &timerid);
	if (ret != 0) {
		perror("Failed to create timer");
		exit(EXIT_FAILURE);
	}

	// set one alarm, without interval
	struct itimerspec new_value = { .it_interval.tv_sec = 0,
		                        .it_interval.tv_nsec = 0,
		                        .it_value.tv_sec = timeout_seconds,
		                        .it_value.tv_nsec = 0 };
	ret = timer_settime(timerid, 0, &new_value, NULL);
	if (ret != 0) {
		perror("Failed to set alarm on timer");
		exit(EXIT_FAILURE);
	}
}

void
sighandler_kill_child(int signum)
{
	kill(childpid, 15);  // SIGTERM
}

int
main(int argc, char *argv[])
{
	if (argc > 1 && strcmp(argv[1], "--help") == 0) {
		show_help();
		exit(EXIT_SUCCESS);
	} else if (argc < 3) {
		fputs("Try 'timeout --help' for more information.", stderr);
	}
	// timeout_seconds = strtoi()
	char *first_nondigit_char;
	long timeout_seconds = strtol(argv[1], &first_nondigit_char, 10);
	if (first_nondigit_char[0] != '\0') {  // invalid numeric argument
		fprintf(stderr, "timeout: invalid time interval ‘%s’\n", argv[1]);
		fputs("Try 'timeout --help' for more information.\n", stderr);
		exit(EXIT_FAILURE);
	}

	void *ret = signal(
	        14, sighandler_kill_child);  // Sends TERM signal to child upon SIGALRM (#14)
	if (ret == (void *) -1) {
		perror("Failed to set signal handler");
		exit(EXIT_FAILURE);
	}
	int pid = fork();
	if (pid == 0) {  // child
		execvp(argv[2], argv + 2);
		perror("failed to exec command");
	} else if (pid > 0) {  // parent
		childpid = pid;
		if (timeout_seconds > 0)
			set_timeout(timeout_seconds);
		// if child process exited, relay exit status
		int wstatus;
		wait(&wstatus);
		if (WIFEXITED(wstatus))
			exit(WEXITSTATUS(wstatus));
		else
			exit(124);
	} else {  // error
		perror("failed to create child process");
	}
	exit(EXIT_FAILURE);
}