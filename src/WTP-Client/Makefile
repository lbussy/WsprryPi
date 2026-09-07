# SPDX-License-Identifier: MIT
# Standalone, hardware-free C++20 component; no parent or Pico runtime dependency.
CXX ?= c++
PYTHON ?= python3
BUILD_DIR ?= build
CXXFLAGS ?= -O2 -g
FLAGS = -std=c++20 -Wall -Wextra -Wpedantic -Werror $(CXXFLAGS) -Iinclude -Isrc
SOURCES = src/codec.cpp src/frame_parser.cpp src/wire.cpp src/detail/json.cpp
HEADERS = $(wildcard include/wtp/*.hpp src/detail/*.hpp)
OBJECTS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
LIBRARY = $(BUILD_DIR)/libwtp_client.a
.PHONY: all test
all: $(LIBRARY)
test: $(BUILD_DIR)/protocol_test $(BUILD_DIR)/codec_driver
	$(BUILD_DIR)/protocol_test
	$(PYTHON) -B tests/vectors_test.py $(BUILD_DIR)/codec_driver

$(BUILD_DIR)/%.o: %.cpp $(HEADERS)
	mkdir -p $(@D)
	$(CXX) $(FLAGS) -c $< -o $@

$(LIBRARY): $(OBJECTS)
	$(AR) rcs $@ $(OBJECTS)

$(BUILD_DIR)/protocol_test: tests/protocol_test.cpp $(LIBRARY) $(HEADERS)
	$(CXX) $(FLAGS) $< $(LIBRARY) $(LDFLAGS) -o $@

$(BUILD_DIR)/codec_driver: tests/codec_driver.cpp $(LIBRARY) $(HEADERS)
	$(CXX) $(FLAGS) $< $(LIBRARY) $(LDFLAGS) -o $@
