# Extract project details from Git
#
# Project name from remote origin URL (fallback to 'unknown' if not available)
PRJ := $(shell git config --local remote.origin.url 2>/dev/null | sed -n 's#.*/\([^.]*\)\.git#\1#p' || echo "unknown")
# Current Git branch (sanitize non-alphanumeric characters to '-')
BRH := $(shell git rev-parse --abbrev-ref HEAD 2>/dev/null | sed -E 's/[^a-zA-Z0-9]/-/g' || echo "unknown")
# Latest Git tag (version) without leading 'v'
VER := $(shell git describe --tags --match "v[0-9]*" --match "[0-9]*" --abbrev=0 2>/dev/null | sed -E 's/^v?([0-9]+\.[0-9]+\.[0-9]+)/\1/' || echo "0.0.0")
# Append branch name if not on 'main' or 'master'
PRE := $(shell if [ "$(BRH)" != "main" ] && [ "$(BRH)" != "master" ]; then echo "-$(BRH)"; fi)
# Append branch name to version only if not already suffixed
VER := $(VER)$(PRE)
# Append commit hash if current commit is not an exact tag
COMMIT := $(shell git rev-parse --short HEAD 2>/dev/null || echo "unknown")
VER := $(shell if ! git describe --tags --exact-match >/dev/null 2>&1; then echo "$(VER)+$(COMMIT)"; else echo "$(VER)"; fi)
# Lower-case based executable name
EXE_NAME := $(shell echo $(PRJ) | tr '[:upper:]' '[:lower:]')

# Output Items
OUT := $(EXE_NAME)							# Normal release binary
TEST_OUT :=	$(EXE_NAME)_test				# Debug/test binary
# Strip whitespace from comments
OUT := $(strip $(OUT))
TEST_OUT := $(strip $(TEST_OUT))
TEST_FREQ := $(strip $(TEST_FREQ))
PREFIX := $(strip $(PREFIX))

# Output directories
OBJ_DIR_RELEASE = build/obj/release
OBJ_DIR_DEBUG   = build/obj/debug
DEP_DIR         = build/dep
BIN_DIR		 	= build/bin

# Collect source files
C_SOURCES   := $(shell find . -name "*.c")
CPP_SOURCES := $(shell find . -name "*.cpp" ! -path "./*/main.cpp")
# Collect object files
C_OBJECTS   := $(patsubst %.c,$(OBJ_DIR_RELEASE)/%.o,$(C_SOURCES))
CPP_OBJECTS := $(patsubst %.cpp,$(OBJ_DIR_RELEASE)/%.o,$(CPP_SOURCES))
# Linker Flags
LDFLAGS := -lpthread  -latomic
LDFLAGS += $(shell pkg-config --cflags --libs libgpiod)
LDFLAGS += $(shell pkg-config --libs libgpiodcxx)
# Collect dependency files
DEPFILES := $(wildcard $(DEP_DIR)/**/*.d)
# Include dependencies if they exist
-include $(DEPFILES)

# Compiler Executables
CXX = g++
CC = gcc

# Allow threading using all available processors
MAKEFLAGS := -j$(nproc)

# Compiler Flags
#
# Common flags for both C and C++ compilers
COMMON_FLAGS := -Wall -Werror -fmax-errors=10 -MMD -MP
# C++ Flags
COMM_CXX_FLAGS := -Wno-psabi -lstdc++fs -std=c++17
CXXFLAGS := $(COMMON_FLAGS) $(COMM_CXX_FLAGS)
# Include paths for libraries
# CXXFLAGS += -I$(abspath ./INI-Handler/src)
# CXXFLAGS += -I$(abspath ./LCBLog/src)
# CXXFLAGS += -I$(abspath ./MonitorFile/src)
# CXXFLAGS += -I$(abspath ./WSPR-Message/src)
# CXXFLAGS += -I$(abspath ./Singleton/src)
# CXXFLAGS += -I$(abspath ./Broadcom-Mailbox/src)
# CXXFLAGS += -I$(abspath ./PPM-Manager/src)
# Define additional macros for the compiler
CXXFLAGS += -DMAKE_TAG=\"$(VER)\" -DMAKE_BRH=\"$(BRH)\" -DMAKE_EXE=\"$(OUT)\" -DMAKE_PRJ=\"$(PRJ)\"
# Target flags
CXX_RELEASE_FLAGS := $(CXXFLAGS) -O2			# Release optimized
CXX_DEBUG_FLAGS := $(CXXFLAGS) -g -DDEBUG_BUILD	# Debug flags
# Strip whitespaces
CXX_DEBUG_FLAGS := $(strip $(CXX_DEBUG_FLAGS))
CXX_RELEASE_FLAGS := $(strip $(CXX_RELEASE_FLAGS))

# Enable verbose output if VERBOSE=1 is specified during the build
VERBOSE ?= 0
ifeq ($(VERBOSE), 1)
	Q :=
else
	Q := @
endif

##
# Compile and Link
##

# Compile C++ source files for debug
$(OBJ_DIR_DEBUG)/%.o: %.cpp
	$(Q)mkdir -p $(dir $@)
	$(Q)mkdir -p $(DEP_DIR)/$(dir $<)
	$(Q)echo "Compiling (debug) $< into $@"
	$(Q)$(CXX) $(CXX_DEBUG_FLAGS) -MF $(DEP_DIR)/$*.d -c $< -o $@

# Link the debug binary
build/bin/$(TEST_OUT): $(patsubst %.cpp,$(OBJ_DIR_DEBUG)/%.o,$(CPP_SOURCES))
	$(Q)mkdir -p $(BIN_DIR)
	$(Q)echo "Linking debug: $(TEST_OUT)"
	$(Q)$(CXX) $(CXX_DEBUG_FLAGS) $^ -o build/bin/$(TEST_OUT) $(LDFLAGS)

# Compile C++ source files (release)
$(OBJ_DIR_RELEASE)/%.o: %.cpp
	$(Q)mkdir -p $(dir $@)
	$(Q)mkdir -p $(DEP_DIR)/$(dir $<)
	$(Q)echo "Compiling (release) $< into $@"
	$(Q)$(CXX) $(CXX_RELEASE_FLAGS) -MF $(DEP_DIR)/$*.d -c $< -o $@

# Link the final binary (release)
build/bin/$(OUT): $(CPP_OBJECTS)
	$(Q)mkdir -p $(BIN_DIR)
	$(Q)echo "Linking release binary: $(OUT)"
	$(Q)$(CXX) $(CXX_RELEASE_FLAGS) $^ -o $@ $(LDFLAGS)

##
# Make Targets
##

# Force the fefault to be release (order matters in targets otherwise)
.DEFAULT_GOAL := release

# Clean dirs
.PHONY: clean
clean:
	$(Q)echo "Cleaning up build artifacts."
	$(Q)rm -rf build

# Release target
.PHONY: release
release: build/bin/$(OUT)
	$(Q)echo "Release build completed successfully."

# Debug target
.PHONY: debug
debug: build/bin/$(TEST_OUT)
	$(Q)echo "Debug build completed successfully."

# Test target
.PHONY: test
test: debug
	$(Q)echo "Running test."
	$(Q)./build/bin/$(TEST_OUT) -i $(INI_FILE)

# Show only user-defined macros
.PHONY: macros
macros:
	$(Q)echo "Defined Project Macros:"
	$(Q)$(CXX) $(CXXFLAGS) -dM -E -x c++ /dev/null | grep -v '^#define _' || true

# Lint target
.PHONY: lint
lint:
	$(Q)echo "Running static analysis with cppcheck."
	$(Q)cppcheck --platform=unix32 --std=c++17 --enable=all --inconclusive --force --inline-suppr --quiet $(CPP_SOURCES)

# Display available make targets
.PHONY: help
help:
	$(Q)echo "Available targets:"
	$(Q)echo "  all          Build the project (default: release)."
	$(Q)echo "  clean        Remove build artifacts."
	$(Q)echo "  test         Run the binary."
	$(Q)echo "  lint         Run static analysis."
	$(Q)echo "  macros       Show defined project macros."
	$(Q)echo "  debug        Build with debugging symbols."
	$(Q)echo "  release      Build optimized for production."
	$(Q)echo "  help         Show this help message."
