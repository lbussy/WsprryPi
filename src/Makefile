# Makefile for building the WSPR project

# Allow threading by default using all available processors
MAKEFLAGS += -j$(nproc)

# TODO:
#   - Implement and test the 'test' target.
#   - Implement and test the 'lint' target using cppcheck.
#   - Implement and test the install/uninstall targets.

# Output binary name
OUT = wspr

# Executable name derived from the output binary name
EXE = $(OUT)

# Macro for LED pin number, used as a compile-time constant
LED_PIN = 18

# Installation directory for the binary
prefix = /usr/local

# Compiler settings
CXX = g++            # C++ compiler
CC = gcc             # C compiler

# Common compiler flags for warnings and error handling
COMMON_FLAGS = -Wall -Werror -fmax-errors=5

# Additional C++ compiler flags
COMM_CXX_FLAGS = -static-libgcc -Wno-psabi -lstdc++fs -std=c++17
CXXFLAGS += $(COMMON_FLAGS) $(COMM_CXX_FLAGS)

# Additional C compiler flags
CFLAGS += $(COMMON_FLAGS)

# Linker flags for required libraries
LDLIBS += -lm -lbcm_host

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
CPP_SOURCES := $(shell find . -name "*.cpp")
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
	@echo "Linking executable: $(OUT)"
	$(Q)$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@ $(LDLIBS)

# Compile C++ source files
%.o: %.cpp
	@echo "Compiling object file for $<."
	$(Q)$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# Compile C source files
%.o: %.c
	@echo "Compiling object file for $<."
	$(Q)$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Clean build artifacts
.PHONY: clean
clean:
	@echo "Cleaning old executables and object files."
	$(Q)$(RM) $(C_OBJECTS) $(CPP_OBJECTS) $(OUT) *~ *.log core dump. $(OBJECTS:.o=.d) >/dev/null 2>&1 || true

# Debug target to print detected files and variables
.PHONY: debug
debug:
	@echo "C_SOURCES:\t$(C_SOURCES)"
	@echo "CPP_SOURCES:\t$(CPP_SOURCES)"
	@echo "C_OBJECTS:\t$(C_OBJECTS)"
	@echo "CPP_OBJECTS:\t$(CPP_OBJECTS)"
	@echo "OBJECTS:\t$(OBJECTS)"

# Display available make targets
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all        Build the project."
	@echo "  clean      Remove build artifacts."
	@echo "  debug      Show detected source and object files."
#	@echo "  install    Install the binary to $(prefix)/bin."
#	@echo "  uninstall  Remove the installed binary."
#	@echo "  test       Run tests (if available)."
#	@echo "  lint       Run static analysis on the source files."

# Install the binary to the specified directory
# .PHONY: install
# install: $(OUT)
# 	@echo "Installing $(OUT) to $(prefix)/bin."
# 	$(Q)install -m 0755 $(OUT) $(prefix)/bin

# Uninstall the binary
# .PHONY: uninstall
# uninstall:
# 	@echo "Uninstalling $(OUT) from $(prefix)/bin."
# 	$(Q)$(RM) $(prefix)/bin/$(OUT)

# Test target to run unit tests (implementation pending)
#. PHONY: test
# test:
#	@echo "Running tests..."
#	$(Q)./test_suite

# Lint target for static analysis (implementation pending)
# .PHONY: lint
# lint:
#	@echo "Linting source files..."
#	$(Q)cppcheck --enable=all --inconclusive $(C_SOURCES) $(CPP_SOURCES)
