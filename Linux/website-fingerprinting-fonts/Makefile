all: font-spy

CXX := g++
CXXFLAGS := -O2 -Wall -Wextra -pedantic -Wno-unused-parameter -std=c++17

font-spy: font-spy.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f font-spy
