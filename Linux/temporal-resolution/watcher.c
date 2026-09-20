#define _GNU_SOURCE

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define N_ITERS 1000

#define INOT_EVENT_SIZE (sizeof(struct inotify_event))
#define INOT_EVENT_BUF_LEN (1024 * (INOT_EVENT_SIZE + 16))

static volatile sig_atomic_t stop = 0;

static void handle_signal(int sig)
{
	(void)sig;
	stop = 1;
}

int main(int argc, char *argv[])
{
	if (argc != 3) {
		fprintf(stderr, "Usage: %s /path/to/test_file /path/to/write_file\n",
				argv[0]);
		return EXIT_FAILURE;
	}

	const char *test_file_path = argv[1];
	const char *write_file_path = argv[2];

	char buffer[INOT_EVENT_BUF_LEN];

	struct timespec *times =
		(struct timespec *)malloc(N_ITERS * sizeof(struct timespec));
	if (!times) {
		perror("malloc");
		return EXIT_FAILURE;
	}

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handle_signal;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	int fd = inotify_init();
	if (fd < 0) {
		perror("inotify_init");
		free(times);
		return EXIT_FAILURE;
	}

	int wd = inotify_add_watch(fd, test_file_path, IN_ACCESS);
	if (wd < 0) {
		perror("inotify_add_watch");
		close(fd);
		free(times);
		return EXIT_FAILURE;
	}

	int iter = 0;

	while (iter < N_ITERS && !stop) {
		int length = read(fd, buffer, INOT_EVENT_BUF_LEN);
		if (length < 0) {
			if (stop)
				break;
			perror("read");
			break;
		}

		int i = 0;
		while (i < length && iter < N_ITERS) {
			struct inotify_event *event = (struct inotify_event *)&buffer[i];
			if (event->mask & IN_ACCESS) {
				clock_gettime(CLOCK_MONOTONIC_RAW, &times[iter]);
				iter++;
			}
			i += INOT_EVENT_SIZE + event->len;
		}
	}

	inotify_rm_watch(fd, wd);
	close(fd);

	FILE *write_file = fopen(write_file_path, "w");
	if (write_file == NULL) {
		perror("fopen");
		free(times);
		return EXIT_FAILURE;
	}

	for (int i = 0; i < iter; ++i) {
		fprintf(write_file, "%ld.%09ld\n",
				times[i].tv_sec, times[i].tv_nsec);
	}

	fclose(write_file);
	free(times);

	return EXIT_SUCCESS;
}
