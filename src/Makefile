# This file is released under the GPL v3 License, see <https://www.gnu.org/licenses/>.
# Copyright (C) 2023-2024 Lee C. Bussy (@LBussy)
# Created for WsprryPi version 1.2.1-4c76705 [devel].

# Installation prefix
prefix = /usr/local

# Compiler settings and flags
CXXFLAGS += -Wall -Werror -fmax-errors=5 -static-libgcc
CXXFLAGS += -Wno-psabi -lstdc++fs -std=c++17
LDLIBS += -lm -lbcm_host

# Output binary name
OUT = wspr

# Get version and branch directly using shell commands
VER := $(shell git describe --tags --always --dirty="-dirty" | sed -E 's/-([0-9]+)-g([a-f0-9]+)/-\2/')
BRH := $(shell git rev-parse --abbrev-ref HEAD)

# Define source files
C_SOURCES := ini.c mailbox.c
CPP_SOURCES := config.cpp ini_reader.cpp singleton.cpp utils.cpp version.cpp wspr_message.cpp main.cpp

# Generate object files from source files
C_OBJECTS := $(C_SOURCES:.c=.o)
CPP_OBJECTS := $(CPP_SOURCES:.cpp:.o)
OBJECTS := $(C_OBJECTS) $(CPP_OBJECTS)

# Generate executables by stripping extensions from all sources
SOURCES := $(C_SOURCES) $(CPP_SOURCES)
EXECUTABLES := $(basename $(SOURCES))

# Default target
all: $(OUT)

# Rule for creating the final binary
$(OUT): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@ $(LDLIBS)

# Rule for version.o with additional macros
version.o: version.cpp
	$(CXX) $(CXXFLAGS) -DMAKE_SRC_TAG=$(VER) -DMAKE_SRC_BRH=$(BRH) -c $< -o $@

# Rule for main.o with additional macros
main.o: main.cpp
	$(CXX) $(CXXFLAGS) -DLED_PIN=18 -c $< -o $@

# Pattern rule for compiling other C++ source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Pattern rule for compiling C source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up intermediate and additional files
.PHONY: clean
clean:
	$(RM) $(C_OBJECTS) $(CPP_OBJECTS) $(EXECUTABLES) $(OUT) *~ *.log core dump.*

# Rule for building the debug version
.PHONY: debug
debug: CXXFLAGS += -DWSPR_DEBUG
debug: $(OUT)

# Install the binary to the specified prefix
.PHONY: install
install: $(OUT)
	install -m 0755 $(OUT) $(prefix)/bin

# Uninstall the binary
.PHONY: uninstall
uninstall:
	$(RM) $(prefix)/bin/$(OUT)

# Print debug information about version and branch
.PHONY: print-debug
print-debug:
	@echo VER: $(VER)
	@echo BRH: $(BRH)
