# Compiler and Flags
CXX := g++
CXXFLAGS := -Wall -Wextra -pedantic -std=c++17 -fPIC
LDFLAGS := -shared
LIB_NAME := libwsprrypi.so
TARGET := wp_server  # Updated output binary name

# Source and Object Files
SRC := wp_server.cpp lcblog.cpp  # Include lcblog.cpp for logging
OBJ := $(SRC:.cpp=.o)

# Default target - Build shared library
all: $(LIB_NAME)

# Compile shared library
$(LIB_NAME): $(OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^

# Compile object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Test target (Compiles wp_server with DEBUG_MAIN_WPSERVER flag)
test: CXXFLAGS += -DDEBUG_MAIN_WPSERVER
test: $(TARGET)
	./$(TARGET)

# Final executable target
$(TARGET): main.o wp_server.o lcblog.o  # Updated target name
	$(CXX) -o $@ $^

main.o: main.cpp wp_server.hpp lcblog.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

lcblog.o: lcblog.cpp lcblog.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Clean compiled files
clean:
	rm -f $(OBJ) main.o lcblog.o $(LIB_NAME) $(TARGET)