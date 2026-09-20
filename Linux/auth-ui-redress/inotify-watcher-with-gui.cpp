#include <cmath>
#include <cstdio>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/wait.h>

int main()
{
  int fd = inotify_init();
  if (fd < 0)
  {
    printf("inotify_init failed\n");
    return 1;
  }

  int wd = inotify_add_watch(fd, "/usr/bin/pkexec", IN_ACCESS);
  if (wd < 0)
  {
    printf("inotify_add_watch failed\n");
    return 1;
  }

  char buf[1024];
  printf("Watching pkexec...\n");

  while (true)
  {
    const int len = read(fd, buf, sizeof(buf));
    if (len < 0)
    {
      printf("read failed\n");
      return 1;
    }

    for (char *ptr = buf; ptr < buf + len; ptr++)
    {
      const inotify_event* event = (inotify_event*)ptr;
      if (event->mask & IN_ACCESS)
      {
        pid_t pid = fork();
        if (pid == 0)
        {
          execlp("./window-launcher", "./window-launcher", NULL);
          printf("Execpl failed\n");
          exit(1);
        }
        if (pid < 0)
        {
          printf("error fork\n");
          return 1;
        }

        waitpid(pid, nullptr, 0);
        break;
      }
    }
  }
}
