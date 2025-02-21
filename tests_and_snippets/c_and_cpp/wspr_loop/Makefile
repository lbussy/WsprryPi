# Allow threading using all available processors
MAKEFLAGS += -j$(nproc)

# Output binary name
OUT = wspr_test

# Executable name derived from the output binary name
EXE = $(OUT)

# INI file for test target
INI_FILE = /usr/local/etc/wsprrypi.ini

# Installation directory for the binary
prefix = /usr/local

# Compiler settings
CXX = g++
CC = gcc

# Common compiler flags for warnings and error handling
COMMON_FLAGS = -Wall -Werror -fmax-errors=10
COMM_CXX_FLAGS = -Wno-psabi -lstdc++fs -std=c++17
CXXFLAGS += $(COMMON_FLAGS) $(COMM_CXX_FLAGS)

# Include paths for libraries
CXXFLAGS += -I$(abspath ./INI-Handler/src)
CXXFLAGS += -I$(abspath ./LCBLog/src)
CXXFLAGS += -I$(abspath ./MonitorFile/src)
CXXFLAGS += -I$(abspath ./WSPR-Message/src)

# Enable verbose output if VERBOSE=1 is specified during the build
VERBOSE ?= 0
ifeq ($(VERBOSE), 1)
	Q =
else
	Q = @
endif

# Extract project details from Git
PRJ := $(shell git config --local remote.origin.url | sed -n 's#.*/\([^.]*\)\.git#\1#p')
BRH := $(shell git rev-parse --abbrev-ref HEAD | sed -E 's/[^a-zA-Z0-9]/-/g')
VER := $(shell git describe --tags --match "v[0-9]*" --match "[0-9]*" --abbrev=0 2>/dev/null | sed -E 's/^v?([0-9]+\.[0-9]+\.[0-9]+)/\1/')
PRE := $(shell if [ "$(BRH)" != "main" ] && [ "$(BRH)" != "master" ]; then echo "$(BRH)"; fi)
VER := $(shell if ! echo "$(VER)" | grep -q -- '-'; then [ -n "$(PRE)" ] && echo "$(VER)-$(PRE)" || echo "$(VER)"; else echo "$(VER)"; fi)
VER := $(shell if [ -z "$(git describe --tags --exact-match 2>/dev/null)" ]; then echo "$(VER)+$(shell git rev-parse --short HEAD)"; else echo "$(VER)"; fi)

# Define additional macros for the compiler
CXXFLAGS += -DMAKE_TAG="$(VER)" -DMAKE_BRH="$(BRH)" -DMAKE_EXE="$(EXE)" -DMAKE_PRJ="$(PRJ)"

# Find source files dynamically
C_SOURCES := $(shell find . -name "*.c")
CPP_SOURCES := $(shell find . -name "*.cpp" ! -path "./*/main.cpp" 2>/dev/null || echo "")
C_OBJECTS := $(patsubst %.c,build/obj/%.o,$(C_SOURCES))
CPP_OBJECTS := $(patsubst %.cpp,build/obj/%.o,$(CPP_SOURCES))
OBJECTS := $(C_OBJECTS) $(CPP_OBJECTS)

# Dependency files
DEPS = $(OBJECTS:.o=.d)

# Default target
.PHONY: all
all: build_dirs release

# Ensure required build directories exist
.PHONY: build_dirs
build_dirs:
	@mkdir -p build/bin build/obj build/dep

# Release target
.PHONY: release
release: CXXFLAGS += -O2
release: $(OUT)
	@echo "Release build completed successfully."

# Debug target
.PHONY: debug
debug: CXXFLAGS += -g -DDEBUG
debug: $(OUT)
	@echo "Debug build completed successfully."

# Link the final binary
$(OUT): $(OBJECTS)
	@mkdir -p build/bin
	@echo "Linking: $(OUT)"
	$(Q)$(CXX) $(CXXFLAGS) $(OBJECTS) -o build/bin/$(OUT)

# Compile C++ source files
build/obj/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling $< into $@"
	$(Q)$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# Compile C source files
build/obj/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "Compiling $< into $@"
	$(Q)$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Dependency management
-include $(DEPS)

# Test target
.PHONY: test
test: $(OUT)
	@echo "Running test with sudo privileges."
	sudo ./build/bin/$(OUT) -i $(INI_FILE)

# Clean target
.PHONY: clean
clean:
	@echo "Cleaning up build artifacts."
	@[ -d build ] && rm -rf build || true

# Install target
.PHONY: install
install: $(OUT)
	@echo "Installing $(OUT) to $(prefix)/bin."
	$(Q)sudo install -m 755 build/bin/$(OUT) $(prefix)/bin/$(OUT)

# Uninstall target
.PHONY: uninstall
uninstall:
	@echo "Removing $(prefix)/bin/$(OUT)."
	$(Q)sudo rm -f $(prefix)/bin/$(OUT)

# Show only user-defined macros
.PHONY: defines
defines:
	@echo "Defined Macros:"
	@$(CXX) $(CXXFLAGS) -dM -E -x c++ /dev/null | grep -E 'MAKE_TAG|MAKE_BRH|MAKE_EXE|MAKE_PRJ' || true

# Lint target
.PHONY: lint
lint:
	@echo "Running static analysis with cppcheck."
	$(Q)cppcheck --platform=unix32 --std=c++17 --enable=all --inconclusive --force --inline-suppr --quiet \
		-I./LCBLog/src -I./INI-Handler/src -I./MonitorFile/src -I./WSPR-Message/src $(CPP_SOURCES)

# Display available make targets
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all        Build the project (default: release)."
	@echo "  clean      Remove build artifacts."
	@echo "  test       Run the binary."
	@echo "  install    Install the binary to $(prefix)/bin."
	@echo "  uninstall  Remove the installed binary."
	@echo "  lint       Run static analysis."
	@echo "  defines    Show defined macros."
	@echo "  debug      Build with debugging symbols."
	@echo "  release    Build optimized for production."
	@echo "  help       Show this help message."
