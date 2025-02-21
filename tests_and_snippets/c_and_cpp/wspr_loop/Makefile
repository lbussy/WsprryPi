# Allow threading using all available processors
MAKEFLAGS += -j$(nproc)

# Output binary name
OUT = wspr_test

# Compiler settings
CXX = g++
COMMON_FLAGS = -Wall -Werror -fmax-errors=10
COMM_CXX_FLAGS = -Wno-psabi -lstdc++fs -std=c++17
CXXFLAGS += $(COMMON_FLAGS) $(COMM_CXX_FLAGS)

# Include paths for libraries
CXXFLAGS += -I$(abspath ../../../src/LCBLog/src)    # lcblog.hpp
CXXFLAGS += -I$(abspath ../../../src/INI-Handler/src) # ini_file.hpp
CXXFLAGS += -I$(abspath ../../../src/MonitorFile/src) # monitorfile.hpp (header-only)

# Source files (local project only)
SRC = \
		main.cpp \
		ppm_ntp.cpp \
		arg_parser.cpp \
		scheduling.cpp \
		transmit.cpp \
		signal_handler.cpp

# External compiled sources (not header-only)
EXTERNAL_SRC = \
    $(abspath ../../../src/LCBLog/src/lcblog.cpp) \
    $(abspath ../../../src/INI-Handler/src/ini_file.cpp)

# Object files
OBJ = $(patsubst %.cpp, %.o, $(notdir $(SRC) $(EXTERNAL_SRC)))

# Dependency files
DEPS = $(OBJ:.o=.d)

# Default target
.PHONY: all
all: $(OUT)
	@echo "Build completed successfully."

# Link the final binary
$(OUT): $(OBJ)
	@echo "Linking: $(OUT)"
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(OUT)

# Compile C++ source files
%.o: %.cpp
	@echo "Compiling $< into $@"
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# External object rules (source-linked libraries)
lcblog.o: $(abspath ../../../src/LCBLog/src/lcblog.cpp)
	@echo "Compiling lcblog.cpp"
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

ini_file.o: $(abspath ../../../src/INI-Handler/src/ini_file.cpp)
	@echo "Compiling ini_file.cpp"
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# Dependency management
-include $(DEPS)

# Test target
.PHONY: test
test: $(OUT)
	@echo "Running test with sudo privileges..."
	sudo ./$(OUT)

# Clean target
.PHONY: clean
clean:
	@echo "Cleaning up..."
	rm -f $(OBJ) $(DEPS) $(OUT)