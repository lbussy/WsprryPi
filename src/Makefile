# Macro for LED pin number, used as a compile-time constant
LED_PIN = 18

# Installation directory for the binary
prefix = /usr/local

# Compiler settings
CXX = g++                          # C++ compiler
CC = gcc                           # C compiler
CFLAGS += -Wall -Werror -fmax-errors=5   # Common C flags for strict warnings
CXXFLAGS += -Wall -Werror -fmax-errors=5 -static-libgcc # Common C++ flags
CXXFLAGS += -Wno-psabi -lstdc++fs -std=c++17            # Additional C++ flags
LDLIBS += -lm -lbcm_host            # Linker flags for math and BCM library

# Output binary name
OUT = wspr

# Executable name derived from OUT
EXE = $(OUT)

# Project name derived from the Git repository name
PRJ := $(shell git config --local remote.origin.url | sed -n 's#.*/\([^.]*\)\.git#\1#p')

# Get the current branch name and sanitize it by replacing non-alphanumeric characters with hyphens
BRH := $(shell git rev-parse --abbrev-ref HEAD | sed -E 's/[^a-zA-Z0-9]/-/g')

# Extract the semantic version from the latest Git tag, handling tags with or without a "v" prefix
VER := $(shell git describe --tags --match "v[0-9]*" --match "[0-9]*" --abbrev=0 2>/dev/null | sed -E 's/^v?([0-9]+\.[0-9]+\.[0-9]+)/\1/')

# Use the branch name as a pre-release identifier if it is not "main" or "master"
PRE := $(shell if [ "$(BRH)" != "main" ] && [ "$(BRH)" != "master" ]; then echo "$(BRH)"; fi)

# Append pre-release identifier to the version if it doesn't already have one
VER := $(shell if ! echo "$(VER)" | grep -q -- '-'; then \
            if [ -n "$(PRE)" ]; then \
                echo "$(VER)-$(PRE)"; \
            else \
                echo "$(VER)"; \
            fi; \
        else \
            echo "$(VER)"; \
        fi)

# Append build metadata (Git commit hash) if not on an exact tagged commit
VER := $(shell if [ -z "$(git describe --tags --exact-match 2>/dev/null)" ]; then \
            echo "$(VER)+$(shell git rev-parse --short HEAD)"; \
        else \
            echo "$(VER)"; \
        fi)

# Pass version, branch, executable, and project macros globally
CXXFLAGS += -DMAKE_TAG=$(VER) -DMAKE_BRH=$(BRH) -DMAKE_EXE=$(EXE) -DMAKE_PRJ=$(PRJ)

# Define source files for C and C++
C_SOURCES := ini.c mailbox.c
CPP_SOURCES := config.cpp ini_reader.cpp lcblog.cpp singleton.cpp utils.cpp version.cpp monitorfile.cpp wspr_message.cpp main.cpp

# Generate object files for the sources
C_OBJECTS := $(C_SOURCES:.c=.o)
CPP_OBJECTS := $(CPP_SOURCES:.cpp=.o)
OBJECTS := $(C_OBJECTS) $(CPP_OBJECTS)

# Default target to build the binary
all: $(OUT)

# Rule to link all object files into the final binary
$(OUT): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@ $(LDLIBS)

# Custom rule for compiling main.o with additional macros
main.o: main.cpp
	$(CXX) $(CXXFLAGS) -DLED_PIN=$(LED_PIN) -c $< -o $@

# Pattern rule for compiling C++ source files into object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Pattern rule for compiling C source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target to remove all intermediate and output files
.PHONY: clean
clean:
	$(RM) $(C_OBJECTS) $(CPP_OBJECTS) $(OUT) *~ *.log core dump.
	$(RM) $(C_SOURCES:.c=) $(CPP_SOURCES:.cpp=)

# Install target to copy the binary to the installation prefix
.PHONY: install
install: $(OUT)
	install -m 0755 $(OUT) $(prefix)/bin

# Uninstall target to remove the binary from the installation prefix
.PHONY: uninstall
uninstall:
	$(RM) $(prefix)/bin/$(OUT)
