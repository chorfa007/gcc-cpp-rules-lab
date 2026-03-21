CXX      := g++
CXXFLAGS := -std=c++26 -freflection -Wall -Wextra -O2 -g

SRCS := $(wildcard 0*.cpp)
BINS := $(patsubst %.cpp,bin/%,$(SRCS))

all: bin $(BINS)

bin:
	mkdir -p bin

bin/%: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@
	@echo "  compiled → $@"

clean:
	rm -rf bin/

.PHONY: all clean
