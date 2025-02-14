# -----------------------------------------------------------------------------
# MIT License
# 
# Copyright (c) 2025 Lee C. Bussy (@lbussy)
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# provided to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
# -----------------------------------------------------------------------------

# Makefile for building the WSPR project

# Allow threading by default using all available processors
MAKEFLAGS += -j$(nproc)

# Output binary name
OUT = wsprrypi

# Executable name derived from the output binary name
EXE = $(OUT)

# Macro for LED pin number, used as a compile-time constant
# Pin 12, GPIO18, BCM18
LED_PIN = 18

INI_FILE = /usr/local/etc/wsprrypi.ini

# Installation directory for the binary
prefix = /usr/local

# Compiler settings
CXX = g++            # C++ compiler
CC = gcc             # C compiler

# Common compiler flags for warnings and error handling
COMMON_FLAGS = -Wall -Werror -fmax-errors=10

# Additional C++ compiler flags
COMM_CXX_FLAGS = -Wno-psabi -lstdc++fs -std=c++17

# Define LED pin number as a compile-time constant
CXXFLAGS += -DLED_PIN=$(LED_PIN)
CFLAGS += -DLED_PIN=$(LED_PIN)

CXXFLAGS += -static-libgcc -static-libstdc++
CXXFLAGS += $(COMMON_FLAGS) $(COMM_CXX_FLAGS)
CXXFLAGS += -ILCBLog/src
CXXFLAGS += -IINI-Handler/src
CXXFLAGS += -IWSPR-Message/src

# Additional C compiler flags
CFLAGS += $(COMMON_FLAGS)

# Linker flags for required libraries
LDLIBS += -static

# Enable verbose output if VERBOSE=1 is specified during the build
VERBOSE ?= 0
ifeq ($(VERBOSE), 1)
	Q =
else
	Q = @
endif

# Extract project details from the Git repository
PRJ := $(shell git config --local remote.origin.url | sed -n 's#.*/\([^.]*\)\.git#\1#p')
BRH := $(shell git rev-parse --abbrev-ref HEAD | sed -E 's/[^a-zA-Z0-9]/-/g')

# Determine the semantic version and append branch or build metadata if necessary
VER := $(shell git describe --tags --match "v[0-9]*" --match "[0-9]*" --abbrev=0 2>/dev/null | sed -E 's/^v?([0-9]+\.[0-9]+\.[0-9]+)/\1/')
PRE := $(shell if [ "$(BRH)" != "main" ] && [ "$(BRH)" != "master" ]; then echo "$(BRH)"; fi)
VER := $(shell if ! echo "$(VER)" | grep -q -- '-'; then [ -n "$(PRE)" ] && echo "$(VER)-$(PRE)" || echo "$(VER)"; else echo "$(VER)"; fi)
VER := $(shell if [ -z "$(git describe --tags --exact-match 2>/dev/null)" ]; then echo "$(VER)+$(shell git rev-parse --short HEAD)"; else echo "$(VER)"; fi)

# Define additional macros for the compiler
CXXFLAGS += -DMAKE_TAG=$(VER) -DMAKE_BRH=$(BRH) -DMAKE_EXE=$(EXE) -DMAKE_PRJ=$(PRJ)

# Define source and object files
C_SOURCES := $(shell find . -name "*.c")
CPP_SOURCES := $(shell find . -name "*.cpp" ! -path "./LCBLog/src/main.cpp" ! -path "./INI-Handler/src/main.cpp" ! -path "./WSPR-Message/src/main.cpp")
C_OBJECTS := $(patsubst %.c,%.o,$(C_SOURCES))
CPP_OBJECTS := $(patsubst %.cpp,%.o,$(CPP_SOURCES))
OBJECTS := $(C_OBJECTS) $(CPP_OBJECTS)

# Include dependency files if available
-include $(OBJECTS:.o=.d)

# Default target: build the binary
.PHONY: all
all: $(OUT)
	@echo "Build completed successfully."

# Link the binary from object files
$(OUT): $(OBJECTS)
	@echo "Linking statically: $(OUT)"
	$(Q)$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@ $(LDLIBS) -static

# Compile C++ source files
%.o: %.cpp
	@echo "Compiling object file for $<."
	$(Q)$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# Compile C source files
%.o: %.c
	@echo "Compiling object file for $<."
	$(Q)$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Clean build artifacts recursively
.PHONY: clean
clean:
	@echo "Cleaning old executables, object files, and temporary files recursively."
	$(Q)$(RM) -r $(C_OBJECTS) $(CPP_OBJECTS) $(OUT) *~ *.log core dump.* $(OBJECTS:.o=.d)
	$(Q)find . -type f \( -name '*.o' -o -name '*.d' -o -name '*~' -o -name '*.log' -o -name 'core' -o -name 'dump.*' \) -exec $(RM) {} +
	$(Q)find . -type d -name 'CMakeFiles' -exec $(RM) -r {} +
	$(Q)find . -type f -name 'CMakeCache.txt' -exec $(RM) {} +

# Debug target to print target output files
.PHONY: targets
targets:
	@echo "Output Binary: $(OUT)"
	@echo "Executable Name: $(EXE)"
	@echo "Build Artifacts: $(OBJECTS)"
	@echo "Linked Libraries: $(LDLIBS)"
	@echo "Defined Macros: -DMAKE_TAG=$(VER) -DMAKE_BRH=$(BRH) -DMAKE_EXE=$(EXE) -DMAKE_PRJ=$(PRJ)"

# Test target: Build with WSPR_DEBUG and run the executable
.PHONY: test
test: clean
	@echo "Building WSPR test mode with WSPR_DEBUG enabled..."
	$(Q)$(MAKE) CXXFLAGS="$(CXXFLAGS) -DWSPR_DEBUG" CFLAGS="$(CFLAGS) -DWSPR_DEBUG" all
	@echo "Running WSPR test mode.
	$(Q)sudo ./$(OUT) -D -i $(INI_FILE)

# Show only user-defined macros from the Makefile
.PHONY: defines
defines:
	@echo "Showing user-defined macros for the C++ compiler:"
	@$(CXX) $(CXXFLAGS) -dM -E -x c++ /dev/null | grep -E 'MAKE_TAG|MAKE_BRH|MAKE_EXE|MAKE_PRJ|LED_PIN' || true
	@echo "Showing user-defined macros for the C compiler:"
	@$(CC) $(CFLAGS) -dM -E -x c /dev/null | grep -E 'MAKE_TAG|MAKE_BRH|MAKE_EXE|MAKE_PRJ|LED_PIN' || true

# Debug target to build with WSPR_DEBUG defined
.PHONY: debug
debug: CXXFLAGS += -DWSPR_DEBUG
debug: CFLAGS += -DWSPR_DEBUG
debug: clean all
	@echo "Debug build completed with WSPR_DEBUG enabled."

# Display available make targets
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all        Build the project."
	@echo "  clean      Remove build artifacts."
	@echo "  files      Show detected source and object files."
	@echo "  defines    Show all defined macros"
	@echo "  install    Install the binary to $(prefix)/bin."
	@echo "  uninstall  Remove the installed binary."
	@echo "  debug      Build the project with WSPR_DEBUG defined."
	@echo "  test       Run unit tests (if available)."
	@echo "  lint       Run static analysis on the source files."

# Install the binary and restart the service
.PHONY: install
install: $(OUT)
	@echo "Stopping wsprrypi service."
	$(Q)sudo systemctl stop wsprrypi.service
	@echo "Copying executable to /usr/local/bin/wsprrypi."
	$(Q)sudo cp $(OUT) /usr/local/bin/wsprrypi
	@echo "Changing ownership to root and setting permissions."
	$(Q)sudo chown root:root /usr/local/bin/wsprrypi
	$(Q)sudo chmod 755 /usr/local/bin/wsprrypi
	@echo "Restarting wsprrypi service."
	$(Q)sudo systemctl start wsprrypi.service
	@echo "Installation completed."

# Uninstall the binary and disable the service
.PHONY: uninstall
uninstall:
	@echo "Stopping and disabling wsprrypi service."
	$(Q)sudo systemctl stop wsprrypi.service
	$(Q)sudo systemctl disable wsprrypi.service
	@echo "Removing executable from /usr/local/bin/wsprrypi."
	$(Q)sudo rm -f /usr/local/bin/wsprrypi
	@echo "Uninstallation completed."

# Lint the source files using cppcheck
.PHONY: lint
lint:
	@echo "Running static analysis with cppcheck..."
	$(Q)cppcheck --enable=all --inconclusive --std=c++17 --force --quiet $(CPP_SOURCES) $(C_SOURCES)