# This file is released under the GPL v3 License, see <https://www.gnu.org/licenses/>.
# Copyright (C) 2023-2024 Lee C. Bussy (@LBussy)
# Created for WsprryPi version 1.2.1-4c76705 [devel].

prefix = /usr/local

LED_PIN = -DLED_PIN=18
CXXFLAGS += -Wall -Werror -fmax-errors=5 -static-libgcc
CXXFLAGS += -Wno-psabi -lstdc++fs -std=c++17 $(LED_PIN)
LDLIBS += -lm -lbcm_host
OUT = wspr

# Get version and branch directly using shell commands
VER := $(shell git describe --tags --always --dirty="-dirty" | sed -E 's/-([0-9]+)-g([a-f0-9]+)/-\2/')
BRH := $(shell git rev-parse --abbrev-ref HEAD)

# List of source files and their corresponding object files
SOURCES := mailbox.c main.cpp wspr_encoder.cpp utils.cpp
OBJECTS := $(SOURCES:.cpp=.o)
OBJECTS := $(OBJECTS:.c=.o)

all: $(OUT)

# Rule for creating the final binary
$(OUT): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -DMAKE_SRC_TAG="$(VER)" -DMAKE_SRC_BRH="$(BRH)" $(OBJECTS) -o $(OUT) $(LDLIBS)

# Pattern rule for compiling C++ source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Pattern rule for compiling C source files
%.o: %.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) *.o $(OUT)

.PHONY: install
install: $(OUT)
	install -m 0755 $(OUT) $(prefix)/bin

.PHONY: uninstall
uninstall:
	$(RM) $(prefix)/bin/$(OUT)

print-debug:
	@echo VER: $(VER)
	@echo BRH: $(BRH)
