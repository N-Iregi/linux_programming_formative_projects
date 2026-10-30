#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#define REPORT_INTERVAL 5

/* global flag variables shared between the main program and a signal handler*/
volatile sig_atomic_t status_flag;
volatile sig_atomic_t toggle_flag;
volatile sig_atomic_t stop_flag;

/**
 * handle_signal - sets a flag for SIGUSR1, SIGUSR2 or SIGINT
 * @sig: the signal number received
 *
 * Only flags are set here. printf() is not async-signal-safe,
 * so all printing is done later in the main loop.
 */
void handle_signal(int sig)
{
	if (sig == SIGUSR1)
		status_flag = 1;
	else if (sig == SIGUSR2)
		toggle_flag = 1;
	else if (sig == SIGINT)
		stop_flag = 1;
}

/**
 * handle_term - handles SIGTERM with an immediate exit
 * @sig: the signal number received (unused)
 *
 * write() and _exit() are async-signal-safe, so they can be
 * called directly from a handler. No cleanup is done here.
 */
void handle_term(int sig)
{
	const char msg[] = "\n[Server Controller] EMERGENCY TERMINATION\n";
	ssize_t ret;

	(void)sig;
	ret = write(STDOUT_FILENO, msg, sizeof(msg) - 1);
	(void)ret;
	_exit(1);
}

/**
 * setup_signals - registers all signal handlers with sigaction()
 *
 * Return: 0 on success, -1 on failure
 */
int setup_signals(void)
{
	struct sigaction sa, st;

	sa.sa_handler = handle_signal;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	st.sa_handler = handle_term;
	sigemptyset(&st.sa_mask);
	st.sa_flags = 0;

	if (sigaction(SIGUSR1, &sa, NULL) == -1)
		return (-1);
	if (sigaction(SIGUSR2, &sa, NULL) == -1)
		return (-1);
	if (sigaction(SIGINT, &sa, NULL) == -1)
		return (-1);
	if (sigaction(SIGTERM, &st, NULL) == -1)
		return (-1);
	return (0);
}

/**
 * process_flags - acts on any flags set by the signal handlers
 * @maintenance: pointer to the state (0 = NORMAL, 1 = MAINTENANCE)
 */
void process_flags(int *maintenance)
{
	if (status_flag)
	{
		status_flag = 0;
		printf("[Server Controller] STATUS: %s | PID: %d\n",
		       *maintenance ? "MAINTENANCE" : "NORMAL", getpid());
	}
	if (toggle_flag)
	{
		toggle_flag = 0;
		*maintenance = !(*maintenance);
		printf("[Server Controller] Mode changed to %s\n",
		       *maintenance ? "MAINTENANCE" : "NORMAL");
	}
}

/**
 * main - runs the monitoring loop until SIGINT or SIGTERM arrives
 *
 * Return: 0 on success, 1 if signal setup fails
 */
int main(void)
{
	int maintenance = 0;
	int ticks = 0;

	/* output piped to tee is fully buffered by default. log would show nothing until exit */
	setvbuf(stdout, NULL, _IOLBF, 0); /* Data is flushed whenever a newline character is encountered */
	if (setup_signals() == -1)
	{
		perror("sigaction");
		return (1);
	}
	printf("[Server Controller] Started. PID: %d\n", getpid());

	while (!stop_flag)
	{
		sleep(1);
		ticks++;
		process_flags(&maintenance);
		if (ticks >= REPORT_INTERVAL)
		{
			ticks = 0;
			if (maintenance)
				printf("[Server Controller] In maintenance mode...\n");
			else
				printf("[Server Controller] Service running normally...\n");
		}
	}
	printf("[Server Controller] SIGINT received. Shutting down...\n");
	printf("[Server Controller] Cleanup complete. Goodbye.\n");
	return (0);
}
