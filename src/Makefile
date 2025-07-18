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

# Debug flags
DEBUG := -DDEBUG_WSPR
DEBUG += -DDEBUG_WSPR_TRANSMIT
# DEBUG += -DDEBUG_PPMMANAGER
# DEBUG += -DDEBUG_SIGNAL_HANDLER

# C++ Version
CXXVER := 17
# Prefix for executing tests, "sudo" if required (comment out "sudo" if not)
SUDO := sudo
# INI File(s)
# INI_FILE :=   ../../../config/wsprrypi.ini # Repo INI file
INI_FILE := /usr/local/etc/wsprrypi.ini	# System INI file
# WSPR Parameters
WSPR_PARAMS := AA0NT EM18 20 40m
# Test Frequency
TEST_FREQ := 730000 # Frequency for test tone

# Add any required submodules by their src path
SUBMODULE_SRCDIRS := $(wildcard ./Mailbox/src)
SUBMODULE_SRCDIRS += $(wildcard ./INI-Handler/src)
SUBMODULE_SRCDIRS += $(wildcard ./LCBLog/src)
SUBMODULE_SRCDIRS += $(wildcard ./MonitorFile/src)
SUBMODULE_SRCDIRS += $(wildcard ./PPM-Manager/src)
SUBMODULE_SRCDIRS += $(wildcard ./Signal-Handler/src)
SUBMODULE_SRCDIRS += $(wildcard ./Singleton/src)
SUBMODULE_SRCDIRS += $(wildcard ./WSPR-Message/src)
SUBMODULE_SRCDIRS += $(wildcard ./WSPR-Transmitter/src)

# Get project name from Git
PRJ := $(notdir $(shell git rev-parse --show-toplevel))
# Lower-case based executable name
EXE_NAME := $(shell echo $(PRJ) | tr '[:upper:]' '[:lower:]')
# Current Git branch (sanitize non-alphanumeric characters to '-')
BRH := $(shell git rev-parse --abbrev-ref HEAD 2>/dev/null | awk '{ gsub(/[^[:alnum:]]+/, "-"); sub(/-$$/, ""); print }' || echo unknown )
# Ask git for the “raw” tag (or empty if it fails)
RAW_TAG := $(strip $(shell git describe --tags --match "v[0-9]*" --match "[0-9]*" --abbrev=0 2>/dev/null) )
# Strip a leading “v” if present; if RAW_TAG is empty, VER becomes “0.0.0”
VER := $(if $(RAW_TAG),$(patsubst v%,%,$(RAW_TAG)),0.0.0)
# Append branch name if not on 'main' or 'master'
PRE := $(shell if [ "$(BRH)" != "main" ] && [ "$(BRH)" != "master" ]; then echo "-$(BRH)"; fi)
# Append branch name to version only if not already suffixed
VER := $(VER)$(PRE)
# Append commit hash if current commit is not an exact tag
COMMIT := $(shell git rev-parse --short HEAD 2>/dev/null || echo "unknown")
# Get exact version tag
VER := $(shell if ! git describe --tags --exact-match >/dev/null 2>&1; then echo "$(VER)+$(COMMIT)"; else echo "$(VER)"; fi)
# Output Items
OUT := $(EXE_NAME)						# Git Repo-Based Name
TEST_OUT := $(EXE_NAME)_debug			# Git Repo-Based Name for debug

# Strip whitespace from file names
OUT := $(strip $(OUT))
TEST_OUT := $(strip $(TEST_OUT))
# Strip whitespace from INI
INI_FILE := $(strip $(INI_FILE))
# Strip whitespace from WSPR parameters
WSPR_PARAMS:= $(strip $(WSPR_PARAMS))
# Strip whitespace from test frequency
TEST_FREQ := $(strip $(TEST_FREQ))

# Install Location
PREFIX := /usr/local/bin

# Output directories
OBJ_DIR_RELEASE = build/obj/release
OBJ_DIR_DEBUG   = build/obj/debug
DEP_DIR         = build/dep
BIN_DIR		 	= build/bin

# ────────────────────────────────────────────────────────────────────────────────
# Find your project’s .c/.cpp (this will pick up ./main.cpp or ./main.c):
# Only pick up *.cpp in the project root, not in subdirectories:
LOCAL_CPP_SOURCES := $(shell find . -maxdepth 1 -type f -name '*.cpp')
LOCAL_C_SOURCES   := $(shell find . -maxdepth 1 -type f -name '*.c')

# Find in each submodule—but explicitly skip any “main” there:
SUBMODULE_C_SOURCES   := $(shell find $(SUBMODULE_SRCDIRS) -type f -name '*.c'   ! -name 'main.c')
SUBMODULE_CPP_SOURCES := $(shell find $(SUBMODULE_SRCDIRS) -type f -name '*.cpp' ! -name 'main.cpp')

# Combine them
C_SOURCES   := $(LOCAL_C_SOURCES)   $(SUBMODULE_C_SOURCES)
CPP_SOURCES := $(LOCAL_CPP_SOURCES) $(SUBMODULE_CPP_SOURCES)

# Strip any leading ../../ from the submodule paths so your object rules work:
REL_C_SOURCES   := $(patsubst ../../%,%,$(C_SOURCES))
REL_CPP_SOURCES := $(patsubst ../../%,%,$(CPP_SOURCES))

# Regenerate your object‐lists exactly as before:
C_OBJECTS         := $(patsubst %.c,   $(OBJ_DIR_RELEASE)/%.o,$(REL_C_SOURCES))
CPP_OBJECTS       := $(patsubst %.cpp, $(OBJ_DIR_RELEASE)/%.o,$(REL_CPP_SOURCES))
C_DEBUG_OBJECTS   := $(patsubst %.c,   $(OBJ_DIR_DEBUG)/%.o,$(REL_C_SOURCES))
CPP_DEBUG_OBJECTS := $(patsubst %.cpp, $(OBJ_DIR_DEBUG)/%.o,$(REL_CPP_SOURCES))
# ────────────────────────────────────────────────────────────────────────────────

# Linker Flags
LDFLAGS := -lpthread  -latomic
# Get packages for linker from PKG_CONFIG_PATH
LDFLAGS += $(shell pkg-config --cflags --libs libgpiod)
LDFLAGS += $(shell pkg-config --libs libgpiodcxx)
# Strip whitespace from LDFLAGS
LDFLAGS := $(strip $(LDFLAGS))

# Collect dependency files
DEPFILES := $(wildcard $(DEP_DIR)/**/*.d)
-include $(DEPFILES)

# Compiler Executables
CXX = g++
CC = gcc

# Compute total physical RAM in kilobytes
MEM_KB := $(shell awk '/^MemTotal:/ {print $$2}' /proc/meminfo)

# Define thresholds in kB (1.5 GiB = 1536 MiB, 3.5 GiB = 3584 MiB)
ONE_POINT_FIVE_GIB_KB   := $(shell expr 1536 \* 1024)
THREE_POINT_FIVE_GIB_KB := $(shell expr 3584 \* 1024)

# Count CPU cores
NPROCS := $(shell nproc)

# Select job count based on memory tier:
#   Below 1.5 GiB → 1 job
#   Between 1.5 and 3.5 GiB → half of nproc
#   Above 3.5 GiB → full nproc
ifeq ($(shell [ $(MEM_KB) -lt $(ONE_POINT_FIVE_GIB_KB) ] && echo yes),yes)
	JOBS := 1
else ifeq ($(shell [ $(MEM_KB) -lt $(THREE_POINT_FIVE_GIB_KB) ] && echo yes),yes)
	JOBS := $(shell expr $(NPROCS) / 2)
else
	JOBS := $(NPROCS)
endif
# Pass the chosen parallelism into Make
MAKEFLAGS += -j$(JOBS)

# Compiler Flags
#
# Common flags for both C and C++ compilers
COMMON_FLAGS := -Wall -Werror -fmax-errors=10 -MMD -MP

# C Flags
CFLAGS := $(COMMON_FLAGS)
# C Debug Flags
C_DEBUG_FLAGS := $(CFLAGS) -g $(DEBUG)	# Debug flags
# C Release Flags
C_RELEASE_FLAGS := $(CFLAGS) -O2		# Release optimized

# C++ Flags
CXXFLAGS := -Wno-psabi -lstdc++fs -std=c++$(CXXVER)
CXXFLAGS += $(COMMON_FLAGS) $(COMM_CXX_FLAGS)

# Include paths
CXXFLAGS += -I$(abspath .)
CXXFLAGS += $(foreach dir,$(SUBMODULE_SRCDIRS),-I$(abspath $(dir)))

# Define for versions
CXXFLAGS += -DMAKE_TAG=\"$(VER)\" -DMAKE_BRH=\"$(BRH)\" -DMAKE_EXE=\"$(OUT)\" -DMAKE_PRJ=\"$(PRJ)\"

# C++ Debug Flags
CXX_DEBUG_FLAGS := $(CXXFLAGS) -g $(DEBUG)	# Debug flags
# C++ Release Flags
CXX_RELEASE_FLAGS := $(CXXFLAGS) -O2		# Release optimized

# Strip whitespaces
C_DEBUG_FLAGS := $(strip $(C_DEBUG_FLAGS))
C_RELEASE_FLAGS := $(strip $(C_RELEASE_FLAGS))
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
# Compile & link rules
##

# Generic debug compile

# C code
$(OBJ_DIR_DEBUG)/%.o: %.c
	$(Q)mkdir -p $(dir $@)
	$(Q)mkdir -p $(DEP_DIR)/$(dir $*)
	$(Q)echo "Compiling (debug) $< into $@"
	$(Q)$(CC) $(C_DEBUG_FLAGS) -MF $(DEP_DIR)/$*.d -c $< -o $@

# C++ Code
$(OBJ_DIR_DEBUG)/%.o: %.cpp
	$(Q)mkdir -p $(dir $@)
	$(Q)mkdir -p $(DEP_DIR)/$(dir $*)
	$(Q)echo "Compiling (debug) $< into $@"
	$(Q)$(CXX) $(CXX_DEBUG_FLAGS) -MF $(DEP_DIR)/$*.d -c $< -o $@

# Auto‑generate debug compile rules for each C submodule
define MAKE_DEBUG_SUBMOD_RULES
$(OBJ_DIR_DEBUG)/$(patsubst ../../%,%,$(1))/%.o: $(1)/%.c
	$(Q)mkdir -p $$(@D)
	$(Q)mkdir -p $(DEP_DIR)/$(dir $$*)
	$(Q)echo "Compiling (debug) $$< into $$@"
	$(Q)$(CC)  $(C_DEBUG_FLAGS)   -MF $(DEP_DIR)/$$*.d -c $$< -o $$@

# Auto‑generate debug compile rules for each C++ submodule
$(OBJ_DIR_DEBUG)/$(patsubst ../../%,%,$(1))/%.o: $(1)/%.cpp
	$(Q)mkdir -p $$(@D)
	$(Q)mkdir -p $(DEP_DIR)/$(dir $$*)
	$(Q)echo "Compiling (debug) $$< into $$@"
	$(Q)$(CXX) $(CXX_DEBUG_FLAGS) -MF $(DEP_DIR)/$$*.d -c $$< -o $$@
endef

# Make the debug rules we created
$(foreach dir,$(SUBMODULE_SRCDIRS),$(eval $(call MAKE_DEBUG_SUBMOD_RULES,$(dir))))

# Link debug binary
build/bin/$(TEST_OUT): $(CPP_DEBUG_OBJECTS) $(C_DEBUG_OBJECTS)
	$(Q)mkdir -p $(BIN_DIR)
	$(Q)echo "Linking debug: $(TEST_OUT)"
	$(Q)$(CXX) $(CXX_DEBUG_FLAGS) $^ -o $@ $(LDFLAGS)

# Generic release Compile

# C Code
$(OBJ_DIR_RELEASE)/%.o: %.c
	$(Q)mkdir -p $(dir $@)
	$(Q)mkdir -p $(DEP_DIR)/$(dir $*)
	$(Q)echo "Compiling (release) $< into $@"
	$(Q)$(CC) $(C_RELEASE_FLAGS) -MF $(DEP_DIR)/$*.d -c $< -o $@

# C++ Code
$(OBJ_DIR_RELEASE)/%.o: %.cpp
	$(Q)mkdir -p $(dir $@)
	$(Q)mkdir -p $(DEP_DIR)/$(dir $*)
	$(Q)echo "Compiling (release) $< into $@"
	$(Q)$(CXX) $(CXX_RELEASE_FLAGS) -MF $(DEP_DIR)/$*.d -c $< -o $@

# Auto generate release compile rules for each C submodule
define MAKE_RELEASE_SUBMOD_RULES
$(OBJ_DIR_RELEASE)/$(patsubst ../../%,%,$(1))/%.o: $(1)/%.c
	$(Q)mkdir -p $$(@D)
	$(Q)mkdir -p $(DEP_DIR)/$(dir $$*)
	$(Q)mkdir -p $(DEP_DIR)/$(dir $$*)
	$(Q)echo "Compiling (release) $$< into $$@"
	$(Q)$(CC)  $(C_RELEASE_FLAGS)   -MF $(DEP_DIR)/$$*.d -c $$< -o $$@

# Auto‑generate release compile rules for each C++ submodule
$(OBJ_DIR_RELEASE)/$(patsubst ../../%,%,$(1))/%.o: $(1)/%.cpp
	$(Q)mkdir -p $$(@D)
	$(Q)mkdir -p $(DEP_DIR)/$(dir $$*)
	$(Q)echo "Compiling (release) $$< into $$@"
	$(Q)$(CXX) $(CXX_RELEASE_FLAGS) -MF $(DEP_DIR)/$$*.d -c $$< -o $$@
endef

# Make the release rules we created
$(foreach dir,$(SUBMODULE_SRCDIRS),$(eval $(call MAKE_RELEASE_SUBMOD_RULES,$(dir))))

# Link release binary
build/bin/$(OUT): $(CPP_OBJECTS) $(C_OBJECTS)
	$(Q)mkdir -p $(BIN_DIR)
	$(Q)echo "Linking release: $(OUT)"
	$(Q)$(CXX) $(CXX_RELEASE_FLAGS) $^ -o $@ $(LDFLAGS)

##
# Phony targets
##
.PHONY: clean release RELEASE debug DEBUG test test-tone test-oneshot gdb install uninstall lint macros help
DEBUG: debug
RELEASE: release

.PHONY: clean
clean:
	$(Q)echo "Cleaning up build artifacts."
	$(Q)sudo rm -rf build

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
	$(Q)echo "Running test with sudo privileges."
	$(Q)sudo ./build/bin/$(TEST_OUT) -D -i $(INI_FILE)

# Test tone target
.PHONY: test-tone
test-tone: debug
	$(Q)echo "Running test tone with sudo privileges."
	$(Q)sudo ./build/bin/$(TEST_OUT) -D --test-tone $(TEST_FREQ)

# Test one-shot target
.PHONY: test-oneshot
test-oneshot: debug
	$(Q)echo "Running test oneshot with sudo privileges."
	$(Q)sudo ./build/bin/$(TEST_OUT) -D -x 1 $(WSPR_PARAMS)

# Test target with GDB
.PHONY: gdb
gdb: debug
	$(Q)echo "Running test with sudo privileges."
	$(Q)$(SUDO) sudo gdb --args ./build/bin/$(TEST_OUT) -D -i $(INI_FILE)

# Install target (install only the release binary)
.PHONY: install
install: release
	$(Q)echo "Installing $(OUT) to $(PREFIX)."
	$(Q)sudo systemctl stop $(OUT).service || true
	$(Q)sudo bash -c 'echo > /var/log/wsprrypi/wsprrypi_log' || true
	$(Q)sudo install -d $(PREFIX)
	$(Q)sudo install -m 755 build/bin/$(OUT) $(PREFIX)/$(OUT)
	$(Q)sudo systemctl enable $(OUT).service || true
	$(Q)sudo systemctl start $(OUT).service || true
	$(Q)echo "Installation complete."

# Uninstall target
.PHONY: uninstall
uninstall:
	$(Q)echo "Removing $(PREFIX)/bin/$(OUT)."
	$(Q)sudo systemctl stop $(OUT).service || true
	$(Q)sudo systemctl disable $(OUT).service || true
	$(Q)sudo rm -f $(PREFIX)/bin/$(OUT)

lint:
	$(Q)command -v cppcheck >/dev/null 2>&1 || { \
	  echo "Warning: cppcheck not installed."; exit 1; }
	$(Q)echo "Running cppcheck…"
	$(Q)cppcheck --std=c++$(CXXVER) --enable=all --inconclusive \
	      --force --inline-suppr --quiet $(CPP_SOURCES)

macros:
	$(Q)echo "Project macros:"
	$(Q)$(CXX) $(CXXFLAGS) -dM -E -x c++ /dev/null \
	      | grep -v '^#define _' || true

help:
	$(Q)echo "Available targets:"
	$(Q)echo "  release      Build optimized binary"
	$(Q)echo "  debug        Build debug binary"
	$(Q)echo "  test         Run tests"
	$(Q)echo "  gdb          Debug with gdb"
	$(Q)echo "  test-tone    Run the binary with a test tone."
	$(Q)echo "  test-oneshot Run the binary with one iteration."
	$(Q)echo "  install      Install the binary to $(PREFIX)/bin."
	$(Q)echo "  uninstall    Remove the installed binary."
	$(Q)echo "  lint         Run static analysis."
	$(Q)echo "  macros       Show macros"
	$(Q)echo "  clean        Remove build artifacts"
	$(Q)echo "  help         This message"
