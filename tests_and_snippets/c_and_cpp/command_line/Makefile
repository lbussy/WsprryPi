CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-psabi -I../../../src/LCBLog/src -I../../../src/INI-Handler/src
SRC = main.cpp arg_parser.cpp ../../../src/LCBLog/src/lcblog.cpp ../../../src/INI-Handler/src/ini_file.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = test_argparser

# Detect number of processors for parallel builds
NPROC := $(shell nproc)

# Always compile in parallel
MAKEFLAGS += -j$(NPROC)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Allow parallel compilation by removing the implicit dependency on %.o rules
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean test

test: $(TARGET)
	./$(TARGET) -i ../../../config/wsprrypi.ini

clean:
	rm -f $(OBJ) $(TARGET)