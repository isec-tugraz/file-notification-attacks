all: accessor watcher

CC := gcc
CFLAGS := -O2 -Wall -Wextra -pedantic -Wno-unused-parameter

BUILD_DIR := ./build
builddir := @mkdir -p $(BUILD_DIR)

accessor: accessor.c
	$(builddir)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/accessor accessor.c

watcher: watcher.c
	$(builddir)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/watcher watcher.c

clean:
	rm -rf $(BUILD_DIR)
