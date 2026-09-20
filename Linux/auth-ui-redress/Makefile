all: inotify-watcher-with-gui window-launcher

CXX := g++
CXXFLAGS := -O2 -Wall -Wextra -pedantic -Wno-unused-parameter

LD_QT := $(shell pkg-config --cflags --libs Qt6Widgets 2>/dev/null || pkg-config --cflags --libs Qt5Widgets 2>/dev/null)

inotify-watcher-with-gui: inotify-watcher-with-gui.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

window-launcher: window-launcher.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ $(LD_QT) -fPIC

clean:
	rm -f inotify-watcher-with-gui window-launcher
