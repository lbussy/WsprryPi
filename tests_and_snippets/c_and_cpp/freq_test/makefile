# Compiler
CXX = g++

# Compiler Flags
CXXFLAGS = -std=c++11 -Wall -Wextra -Wno-psabi

# Target Executable
TARGET = freq_test

# Source File
SRC = freq_test.cpp

# Build the target
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

# Clean up the generated files
clean:
	rm -f $(TARGET)
