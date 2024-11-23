CXX = g++
CXXFLAGS = -Wall -Wextra -g -pedantic -I./simlib/src
LDFLAGS = -L./simlib/src -l:simlib.a

.PHONY: all
all: build_simlib tomato_preserver

build_simlib:
	@echo "Running build_simlib.sh..."
	@./build_simlib.sh

tomato_preserver: main.cpp
	$(CXX) $(CXXFLAGS) -o tomato_preserver main.cpp $(LDFLAGS)

clean:
	rm -f tomato_preserver output.txt
