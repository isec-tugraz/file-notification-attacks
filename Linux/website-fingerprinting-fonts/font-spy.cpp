// font-spy: watch every subdirectory under /usr/share/fonts for IN_OPEN,
// and report which font file gets opened. Loading a webpage makes the
// browser open exactly the font files needed to render the text on it,
// enough on its own to fingerprint which site is being viewed.
#include <sys/inotify.h>
#include <cstdio>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <unistd.h>

#define FONT_PATH "/usr/share/fonts/"

namespace fs = std::filesystem;

int main()
{
  std::unordered_map<int, std::string> wd_to_path;

  int fd = inotify_init();
  if (fd < 0)
  {
    perror("inotify_init");
    return 1;
  }

  int wd = inotify_add_watch(fd, FONT_PATH, IN_OPEN);
  if (wd >= 0)
    wd_to_path[wd] = FONT_PATH;

  for (const auto &entry : fs::recursive_directory_iterator(FONT_PATH))
  {
    if (!entry.is_directory())
      continue;
    int w = inotify_add_watch(fd, entry.path().c_str(), IN_OPEN);
    if (w >= 0)
      wd_to_path[w] = entry.path().string();
  }

  fprintf(stderr, "watching %s for font opens, load a page in firefox now ...\n", FONT_PATH);

  char buf[4096]
      __attribute__((aligned(__alignof__(struct inotify_event))));

  for (;;)
  {
    ssize_t len = read(fd, buf, sizeof(buf));
    if (len < 0)
    {
      perror("read");
      return 1;
    }

    for (char *ptr = buf; ptr < buf + len; )
    {
      auto *event = reinterpret_cast<struct inotify_event *>(ptr);
      if (wd_to_path.count(event->wd) && event->len > 0)
      {
        printf("%s/%s\n", wd_to_path[event->wd].c_str(), event->name);
        fflush(stdout);
      }
      ptr += sizeof(struct inotify_event) + event->len;
    }
  }
}
