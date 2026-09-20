#define _GNU_SOURCE

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define N_ITERS 1000

int main(int argc, char* argv[])
{
    srand(time(NULL));

    if (argc != 3) {
        fprintf(stderr, "%s /path/to/test_file /path/to/write_file", argv[0]);
        return EXIT_FAILURE;
    }

    const char* test_file_path = argv[1];
    const char* write_file_path = argv[2];

    int fd = open(test_file_path, O_RDONLY);
    if (fd == -1) {
        perror("open");
    }

    struct timespec* times = (struct timespec*)malloc(N_ITERS * sizeof(struct timespec));

    volatile char bufferfly = 0x12;
    char ch;

    for (int i = 0; i < N_ITERS; ++i) {
        usleep((rand() % 1000) + 5000);

        clock_gettime(CLOCK_MONOTONIC_RAW, &times[i]);

        if (read(fd, &ch, sizeof(ch)) != 1)
            perror("read");

        bufferfly ^= ch;
    }

    close(fd);

    FILE* write_file = fopen(write_file_path, "w");
    if (write_file == NULL) {
        perror("fopen");
    }

    for (int i = 0; i < N_ITERS; ++i) {
        fprintf(write_file, " %ld.%09ld\n", times[i].tv_sec, times[i].tv_nsec);
    }

    fclose(write_file);

    return 0;

    fprintf(stderr, "output: %d\n", bufferfly);
}
