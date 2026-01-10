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

# ─────────────────────────────────────────────────────────────────────────────
# User-configurable knobs
# ─────────────────────────────────────────────────────────────────────────────

# Debug flags (add/remove as needed)
DEBUG_DEFS := -DDEBUG_WSPR -DDEBUG_WSPR_TRANSMIT
# DEBUG_DEFS += -DDEBUG_PPMMANAGER
# DEBUG_DEFS += -DDEBUG_SIGNAL_HANDLER

# C++ version
CXXVER := 17

# Prefix for executing tests, "sudo" if required (set SUDO= to disable)
SUDO ?= sudo

# INI File(s)
# INI_FILE := ../../../config/wsprrypi.ini
INI_FILE := /usr/local/etc/wsprrypi.ini

# WSPR Parameters
WSPR_PARAMS := AA0NT EM18 20 40m

# Test Frequency
TEST_FREQ := 730000

# Install location
# If you prefer the old behavior (PREFIX=/usr/local/bin), set it on the command
# line: make PREFIX=/usr/local/bin install
PREFIX ?= /usr/local/bin

# ─────────────────────────────────────────────────────────────────────────────
# Submodules
# ─────────────────────────────────────────────────────────────────────────────

SUBMODULE_SRCDIRS :=
SUBMODULE_SRCDIRS += $(wildcard ./Mailbox/src)
SUBMODULE_SRCDIRS += $(wildcard ./INI-Handler/src)
SUBMODULE_SRCDIRS += $(wildcard ./LCBLog/src)
SUBMODULE_SRCDIRS += $(wildcard ./MonitorFile/src)
SUBMODULE_SRCDIRS += $(wildcard ./PPM-Manager/src)
SUBMODULE_SRCDIRS += $(wildcard ./Signal-Handler/src)
SUBMODULE_SRCDIRS += $(wildcard ./Singleton/src)
SUBMODULE_SRCDIRS += $(wildcard ./WSPR-Message/src)
SUBMODULE_SRCDIRS += $(wildcard ./WSPR-Transmitter/src)

# ─────────────────────────────────────────────────────────────────────────────
# Project naming/versioning (from Git)
# ─────────────────────────────────────────────────────────────────────────────

PRJ := $(notdir $(shell git rev-parse --show-toplevel))
EXE_NAME := $(shell echo $(PRJ) | tr '[:upper:]' '[:lower:]')

BRH := $(shell git rev-parse --abbrev-ref HEAD 2>/dev/null | awk '{ gsub(/[^[:alnum:]]+/, "-"); sub(/-$$/, ""); print }' || echo unknown)
RAW_TAG := $(strip $(shell git describe --tags --match "v[0-9]*" --match "[0-9]*" --abbrev=0 2>/dev/null))
BASE_VER := $(if $(RAW_TAG),$(patsubst v%,%,$(RAW_TAG)),0.0.0)

PRE := $(shell if [ "$(BRH)" != "main" ] && [ "$(BRH)" != "master" ]; then echo "-$(BRH)"; fi)
VER := $(BASE_VER)$(PRE)

COMMIT := $(shell git rev-parse --short HEAD 2>/dev/null || echo "unknown")
VER := $(shell if ! git describe --tags --exact-match >/dev/null 2>&1; then echo "$(VER)+$(COMMIT)"; else echo "$(VER)"; fi)

OUT := $(strip $(EXE_NAME))
TEST_OUT := $(strip $(EXE_NAME)_debug)

# ─────────────────────────────────────────────────────────────────────────────
# Output directories
# ─────────────────────────────────────────────────────────────────────────────

OBJ_DIR_RELEASE := build/obj/release
OBJ_DIR_DEBUG   := build/obj/debug
DEP_DIR         := build/dep
BIN_DIR         := build/bin

# ─────────────────────────────────────────────────────────────────────────────
# Source discovery
# ─────────────────────────────────────────────────────────────────────────────

LOCAL_CPP_SOURCES := $(shell find . -maxdepth 1 -type f -name '*.cpp')
LOCAL_C_SOURCES   := $(shell find . -maxdepth 1 -type f -name '*.c')

SUBMODULE_C_SOURCES   := $(shell find $(SUBMODULE_SRCDIRS) -type f -name '*.c'   ! -name 'main.c')
SUBMODULE_CPP_SOURCES := $(shell find $(SUBMODULE_SRCDIRS) -type f -name '*.cpp' ! -name 'main.cpp')

C_SOURCES   := $(LOCAL_C_SOURCES)   $(SUBMODULE_C_SOURCES)
CPP_SOURCES := $(LOCAL_CPP_SOURCES) $(SUBMODULE_CPP_SOURCES)

REL_C_SOURCES   := $(patsubst ../../%,%,$(C_SOURCES))
REL_CPP_SOURCES := $(patsubst ../../%,%,$(CPP_SOURCES))

C_OBJECTS         := $(patsubst %.c,   $(OBJ_DIR_RELEASE)/%.o,$(REL_C_SOURCES))
CPP_OBJECTS       := $(patsubst %.cpp, $(OBJ_DIR_RELEASE)/%.o,$(REL_CPP_SOURCES))
C_DEBUG_OBJECTS   := $(patsubst %.c,   $(OBJ_DIR_DEBUG)/%.o,$(REL_C_SOURCES))
CPP_DEBUG_OBJECTS := $(patsubst %.cpp, $(OBJ_DIR_DEBUG)/%.o,$(REL_CPP_SOURCES))

# ─────────────────────────────────────────────────────────────────────────────
# Dependency includes
# ─────────────────────────────────────────────────────────────────────────────

DEPFILES := $(wildcard $(DEP_DIR)/**/*.d)
-include $(DEPFILES)

# ─────────────────────────────────────────────────────────────────────────────
# Toolchain
# ─────────────────────────────────────────────────────────────────────────────

CXX := g++
CC  := gcc

# ─────────────────────────────────────────────────────────────────────────────
# Parallel build sizing (based on RAM)
# ─────────────────────────────────────────────────────────────────────────────

MEM_KB := $(shell awk '/^MemTotal:/ {print $$2}' /proc/meminfo)
ONE_POINT_FIVE_GIB_KB   := $(shell expr 1536 \* 1024)
THREE_POINT_FIVE_GIB_KB := $(shell expr 3584 \* 1024)
NPROCS := $(shell nproc)

ifeq ($(shell [ $(MEM_KB) -lt $(ONE_POINT_FIVE_GIB_KB) ] && echo yes),yes)
	JOBS := 1
else ifeq ($(shell [ $(MEM_KB) -lt $(THREE_POINT_FIVE_GIB_KB) ] && echo yes),yes)
	JOBS := $(shell expr $(NPROCS) / 2)
else
	JOBS := $(NPROCS)
endif
MAKEFLAGS += -j$(JOBS)

# ─────────────────────────────────────────────────────────────────────────────
# libgpiod flags and version detection (build-time define)
# ─────────────────────────────────────────────────────────────────────────────

# Pull compiler and linker flags from pkg-config (compile flags go to *FLAGS,
# link flags go to LDFLAGS).
PKG_CFLAGS := $(strip $(shell pkg-config --cflags libgpiod libgpiodcxx 2>/dev/null))
PKG_LIBS   := $(strip $(shell pkg-config --libs   libgpiod libgpiodcxx 2>/dev/null))

# Determine major version from pkg-config. Prefer libgpiod, fall back to libgpiodcxx.
GPIOD_MODVER := $(strip $(shell pkg-config --modversion libgpiod 2>/dev/null))
ifeq ($(GPIOD_MODVER),)
GPIOD_MODVER := $(strip $(shell pkg-config --modversion libgpiodcxx 2>/dev/null))
endif
GPIOD_API_MAJOR := $(strip $(shell printf '%s\n' "$(GPIOD_MODVER)" | cut -d. -f1))
ifeq ($(GPIOD_API_MAJOR),)
GPIOD_API_MAJOR := 1
endif

# ─────────────────────────────────────────────────────────────────────────────
# Flags
# ─────────────────────────────────────────────────────────────────────────────

COMMON_FLAGS := -Wall -Werror -fmax-errors=10 -MMD -MP

CFLAGS   := $(COMMON_FLAGS) $(PKG_CFLAGS) -DGPIOD_API_MAJOR=$(GPIOD_API_MAJOR)
CXXFLAGS := -Wno-psabi -std=c++$(CXXVER) $(COMMON_FLAGS) $(PKG_CFLAGS) -DGPIOD_API_MAJOR=$(GPIOD_API_MAJOR)

# Include paths
CXXFLAGS += -I$(abspath .)
CXXFLAGS += $(foreach dir,$(SUBMODULE_SRCDIRS),-I$(abspath $(dir)))

# Build metadata defines
CXXFLAGS += -DMAKE_TAG=\"$(VER)\" -DMAKE_BRH=\"$(BRH)\" -DMAKE_EXE=\"$(OUT)\" -DMAKE_PRJ=\"$(PRJ)\"

# Debug/release variants
C_DEBUG_FLAGS     := $(CFLAGS)   -g $(DEBUG_DEFS)
C_RELEASE_FLAGS   := $(CFLAGS)   -O2
CXX_DEBUG_FLAGS   := $(CXXFLAGS) -g $(DEBUG_DEFS)
CXX_RELEASE_FLAGS := $(CXXFLAGS) -O2

# Linker flags
LDFLAGS := -lpthread -latomic $(PKG_LIBS)

# ─────────────────────────────────────────────────────────────────────────────
# Verbosity control
# ─────────────────────────────────────────────────────────────────────────────

VERBOSE ?= 0
ifeq ($(VERBOSE),1)
	Q :=
else
	Q := @
endif

# ─────────────────────────────────────────────────────────────────────────────
# Compile rules (debug)
# ─────────────────────────────────────────────────────────────────────────────

$(OBJ_DIR_DEBUG)/%.o: %.c
	$(Q)mkdir -p $(dir $@)
	$(Q)mkdir -p $(DEP_DIR)/$(dir $*)
	$(Q)echo "Compiling (debug) $< into $@"
	$(Q)$(CC) $(C_DEBUG_FLAGS) -MF $(DEP_DIR)/$*.d -c $< -o $@

$(OBJ_DIR_DEBUG)/%.o: %.cpp
	$(Q)mkdir -p $(dir $@)
	$(Q)mkdir -p $(DEP_DIR)/$(dir $*)
	$(Q)echo "Compiling (debug) $< into $@"
	$(Q)$(CXX) $(CXX_DEBUG_FLAGS) -MF $(DEP_DIR)/$*.d -c $< -o $@

define MAKE_DEBUG_SUBMOD_RULES
$(OBJ_DIR_DEBUG)/$(patsubst ../../%,%,$(1))/%.o: $(1)/%.c
	$(Q)mkdir -p $$(@D)
	$(Q)mkdir -p $(DEP_DIR)/$(dir $$*)
	$(Q)echo "Compiling (debug) $$< into $$@"
	$(Q)$(CC) $(C_DEBUG_FLAGS) -MF $(DEP_DIR)/$$*.d -c $$< -o $$@

$(OBJ_DIR_DEBUG)/$(patsubst ../../%,%,$(1))/%.o: $(1)/%.cpp
	$(Q)mkdir -p $$(@D)
	$(Q)mkdir -p $(DEP_DIR)/$(dir $$*)
	$(Q)echo "Compiling (debug) $$< into $$@"
	$(Q)$(CXX) $(CXX_DEBUG_FLAGS) -MF $(DEP_DIR)/$$*.d -c $$< -o $$@
endef
$(foreach dir,$(SUBMODULE_SRCDIRS),$(eval $(call MAKE_DEBUG_SUBMOD_RULES,$(dir))))

# Link debug binary
$(BIN_DIR)/$(TEST_OUT): $(CPP_DEBUG_OBJECTS) $(C_DEBUG_OBJECTS)
	$(Q)mkdir -p $(BIN_DIR)
	$(Q)echo "Linking debug: $(TEST_OUT) (libgpiod major: $(GPIOD_API_MAJOR))"
	$(Q)$(CXX) $(CXX_DEBUG_FLAGS) $^ -o $@ $(LDFLAGS)

# ─────────────────────────────────────────────────────────────────────────────
# Compile rules (release)
# ─────────────────────────────────────────────────────────────────────────────

$(OBJ_DIR_RELEASE)/%.o: %.c
	$(Q)mkdir -p $(dir $@)
	$(Q)mkdir -p $(DEP_DIR)/$(dir $*)
	$(Q)echo "Compiling (release) $< into $@"
	$(Q)$(CC) $(C_RELEASE_FLAGS) -MF $(DEP_DIR)/$*.d -c $< -o $@

$(OBJ_DIR_RELEASE)/%.o: %.cpp
	$(Q)mkdir -p $(dir $@)
	$(Q)mkdir -p $(DEP_DIR)/$(dir $*)
	$(Q)echo "Compiling (release) $< into $@"
	$(Q)$(CXX) $(CXX_RELEASE_FLAGS) -MF $(DEP_DIR)/$*.d -c $< -o $@

define MAKE_RELEASE_SUBMOD_RULES
$(OBJ_DIR_RELEASE)/$(patsubst ../../%,%,$(1))/%.o: $(1)/%.c
	$(Q)mkdir -p $$(@D)
	$(Q)mkdir -p $(DEP_DIR)/$(dir $$*)
	$(Q)echo "Compiling (release) $$< into $$@"
	$(Q)$(CC) $(C_RELEASE_FLAGS) -MF $(DEP_DIR)/$$*.d -c $$< -o $$@

$(OBJ_DIR_RELEASE)/$(patsubst ../../%,%,$(1))/%.o: $(1)/%.cpp
	$(Q)mkdir -p $$(@D)
	$(Q)mkdir -p $(DEP_DIR)/$(dir $$*)
	$(Q)echo "Compiling (release) $$< into $$@"
	$(Q)$(CXX) $(CXX_RELEASE_FLAGS) -MF $(DEP_DIR)/$$*.d -c $$< -o $$@
endef
$(foreach dir,$(SUBMODULE_SRCDIRS),$(eval $(call MAKE_RELEASE_SUBMOD_RULES,$(dir))))

# Link release binary
$(BIN_DIR)/$(OUT): $(CPP_OBJECTS) $(C_OBJECTS)
	$(Q)mkdir -p $(BIN_DIR)
	$(Q)echo "Linking release: $(OUT) (libgpiod major: $(GPIOD_API_MAJOR))"
	$(Q)$(CXX) $(CXX_RELEASE_FLAGS) $^ -o $@ $(LDFLAGS)

# ─────────────────────────────────────────────────────────────────────────────
# Phony targets
# ─────────────────────────────────────────────────────────────────────────────

.PHONY: clean release debug test test-tone test-oneshot gdb install uninstall lint macros help
DEBUG: debug
RELEASE: release

clean:
	$(Q)echo "Cleaning up build artifacts."
	$(Q)$(SUDO) rm -rf build

release: $(BIN_DIR)/$(OUT)
	$(Q)echo "Release build completed successfully."

debug: $(BIN_DIR)/$(TEST_OUT)
	$(Q)echo "Debug build completed successfully."

test: debug
	$(Q)echo "Running test with sudo privileges."
	$(Q)$(SUDO) ./$(BIN_DIR)/$(TEST_OUT) -D -i $(INI_FILE)

test-tone: debug
	$(Q)echo "Running test tone with sudo privileges."
	$(Q)$(SUDO) ./$(BIN_DIR)/$(TEST_OUT) -D --test-tone $(TEST_FREQ)

test-oneshot: debug
	$(Q)echo "Running test oneshot with sudo privileges."
	$(Q)$(SUDO) ./$(BIN_DIR)/$(TEST_OUT) -D -x 1 $(WSPR_PARAMS)

gdb: debug
	$(Q)echo "Running debug session."
	$(Q)$(SUDO) gdb --args ./$(BIN_DIR)/$(TEST_OUT) -D -i $(INI_FILE)

install: release
	$(Q)echo "Installing $(OUT) to $(PREFIX)."
	$(Q)$(SUDO) systemctl stop $(OUT).service || true
	$(Q)$(SUDO) bash -c 'echo > /var/log/wsprrypi/wsprrypi_log' || true
	$(Q)$(SUDO) install -d $(PREFIX)
	$(Q)$(SUDO) install -m 755 $(BIN_DIR)/$(OUT) $(PREFIX)/$(OUT)
	$(Q)$(SUDO) systemctl enable $(OUT).service || true
	$(Q)$(SUDO) systemctl start $(OUT).service || true
	$(Q)echo "Installation complete."

uninstall:
	$(Q)echo "Removing $(PREFIX)/$(OUT)."
	$(Q)$(SUDO) systemctl stop $(OUT).service || true
	$(Q)$(SUDO) systemctl disable $(OUT).service || true
	$(Q)$(SUDO) rm -f $(PREFIX)/$(OUT)

lint:
	$(Q)command -v cppcheck >/dev/null 2>&1 || { \
	  echo "Warning: cppcheck not installed."; exit 1; }
	$(Q)echo "Running cppcheck."
	$(Q)cppcheck --std=c++$(CXXVER) --enable=all --inconclusive \
	      --force --inline-suppr --quiet $(CPP_SOURCES)

macros:
	$(Q)echo "Project macros."
	$(Q)$(CXX) $(CXXFLAGS) -dM -E -x c++ /dev/null \
	      | grep -v '^#define _' || true

help:
	$(Q)echo "Available targets:"
	$(Q)echo "  release      Build optimized binary"
	$(Q)echo "  debug        Build debug binary"
	$(Q)echo "  test         Run tests"
	$(Q)echo "  gdb          Debug with gdb"
	$(Q)echo "  test-tone    Run the binary with a test tone"
	$(Q)echo "  test-oneshot Run the binary with one iteration"
	$(Q)echo "  install      Install the binary to $(PREFIX)"
	$(Q)echo "  uninstall    Remove the installed binary"
	$(Q)echo "  lint         Run static analysis"
	$(Q)echo "  macros       Show macros"
	$(Q)echo "  clean        Remove build artifacts"
	$(Q)echo "  help         This message"
