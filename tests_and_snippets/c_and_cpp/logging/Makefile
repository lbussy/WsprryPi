# -----------------------------------------------------------------------------
# @file Makefile
# @brief Build system for the lcblog library and associated tests.
#
# This Makefile automates the process of building the lcblog static library, 
# compiling the test executable with debug flags enabled, and running static 
# code analysis using cppcheck.
#
# @targets
# - `all`: Builds the static library `liblcblog.a`.
# - `test`: Builds the test executable `lcblog_test` with debug flags enabled.
# - `debug`: Builds the static library with debugging symbols enabled.
# - `lint`: Runs cppcheck for static code analysis on the source files.
# - `clean`: Cleans up build artifacts (object files, library, test executable).
# - `help`: Displays available make targets and their descriptions.
# -----------------------------------------------------------------------------
 
# Target names
NAME        = lcblog
LIB_NAME    = lib$(NAME).a
TEST_NAME   = $(NAME)_test
TEST_LOGS	= test_log.*

# Compiler flags
CXX         = g++
CXXFLAGS    += -Wall -Werror -fmax-errors=5
CXXFLAGS    += -Wno-psabi -std=c++17 -MMD -MP

# Debug flags for testing
DEBUGFLAGS  = -DDEBUG_MAIN_LCBLOG

# Linker flags (if needed)
LDLIBS      =

# Source files
SRC         = $(NAME).cpp
OBJ         = $(SRC:.cpp=.o)
DEP         = $(OBJ:.o=.d)

# Default build target: creates the static library
all: $(LIB_NAME)

# Create the static library
$(LIB_NAME): $(OBJ)
	ar rcs $@ $(OBJ)

# Build the test executable with debug enabled
test: CXXFLAGS += $(DEBUGFLAGS)
test: $(TEST_NAME)

# Create the test executable
$(TEST_NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(TEST_NAME) $(LDLIBS)

# Compile source files into object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	$(RM) $(OBJ) $(DEP) $(LIB_NAME) $(TEST_NAME) $(TEST_LOGS)

# Include dependency files
-include $(DEP)

# Debug build: compiles the library with debug flags enabled
debug: CXXFLAGS += -g
debug: $(LIB_NAME)

# Lint target: performs static code analysis with cppcheck
lint:
	cppcheck --enable=all --inconclusive --max-configs=100 --std=c++17 --language=c++ $(SRC)

# Help target: displays available make targets and their descriptions
help:
	@echo "Available Make targets:"
	@echo "  all        - Build the static library."
	@echo "  test       - Build the test executable with debug flags."
	@echo "  debug      - Build the static library with debugging symbols."
	@echo "  lint       - Run cppcheck for static code analysis."
	@echo "  clean      - Clean up build artifacts (object files, library, etc.)."
	@echo "  help       - Display this help message."