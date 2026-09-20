all: keystroke-notify

CC := gcc
CFLAGS := -O2 -Wall -Wextra -pedantic -Wno-unused-parameter

BUILD_DIR := ./build
builddir := @mkdir -p $(BUILD_DIR)

keystroke-notify: keystroke-notify.c
	$(builddir)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/keystroke-notify keystroke-notify.c

clean:
	rm -rf $(BUILD_DIR)
