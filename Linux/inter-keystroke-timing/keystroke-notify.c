// keystroke-notify.c
// Watch a directory (argv[1]) for IN_ACCESS events, print only when the
// accessed entry name matches argv[2] exactly. A single physical keystroke
// generates several IN_ACCESS events on the same input device close
// together (key down, key up, EV_SYN...), so we suppress reprints within
// SUPPRESS_WINDOW_SEC of the last one and treat each surviving hit as one
// keystroke.
#define _GNU_SOURCE
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <limits.h>

static double monotonic_seconds_now(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0.0;
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static void print_timestamp_hms_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    struct tm tm;
    localtime_r(&ts.tv_sec, &tm);

    long ms = ts.tv_nsec / 1000000L;
    printf("%02d:%02d:%02d.%03ld", tm.tm_hour, tm.tm_min, tm.tm_sec, ms);
}

int main(int argc, char **argv) {
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Usage: %s <directory_path> <device_name> [suppress_window_sec]\n", argv[0]);
        fprintf(stderr, "  e.g.  %s /dev/input event4 0.13\n", argv[0]);
        return 2;
    }

    const char *dir = argv[1];
    const char *device = argv[2];
    const double suppress_window_sec = argc == 4 ? atof(argv[3]) : 0.13;

    int fd = inotify_init1(IN_NONBLOCK);
    if (fd < 0) {
        perror("inotify_init1");
        return 1;
    }

    // Watch the directory, not the device node itself: we can't read the
    // node (root:input, 660), but we can watch its readable parent and
    // still get told when something inside it is accessed.
    int wd = inotify_add_watch(fd, dir, IN_ACCESS);
    if (wd < 0) {
        fprintf(stderr, "inotify_add_watch('%s') failed: %s\n", dir, strerror(errno));
        close(fd);
        return 1;
    }

    double last_print_t = -1e99;

    char buf[4096]
        __attribute__((aligned(__alignof__(struct inotify_event))));

    fprintf(stderr, "watching %s for accesses to %s (suppress window %.3fs) ...\n",
            dir, device, suppress_window_sec);

    for (;;) {
        struct pollfd pfd = { .fd = fd, .events = POLLIN };
        int pr = poll(&pfd, 1, -1);
        if (pr < 0) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }

        if (!(pfd.revents & POLLIN))
            continue;

        for (;;) {
            ssize_t len = read(fd, buf, sizeof(buf));
            if (len < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                if (errno == EINTR) continue;
                perror("read");
                goto out;
            }
            if (len == 0) break;

            for (char *ptr = buf; ptr < buf + len; ) {
                struct inotify_event *ev = (struct inotify_event *)ptr;
                const char *name = (ev->len > 0 && ev->name[0] != '\0') ? ev->name : NULL;

                if ((ev->mask & IN_ACCESS) && name && strcmp(name, device) == 0) {
                    double now = monotonic_seconds_now();
                    if (now - last_print_t >= suppress_window_sec) {
                        last_print_t = now;
                        print_timestamp_hms_ms();
                        printf("  KEYPRESS  %s/%s\n", dir, name);
                        fflush(stdout);
                    }
                }

                ptr += sizeof(struct inotify_event) + ev->len;
            }
        }
    }

out:
    inotify_rm_watch(fd, wd);
    close(fd);
    return 0;
}
