OBJS	= example.o configparser.o
SOURCE	= example.cpp configparser.cpp
HEADER	= configparser.hpp
OUT	= example
CC	 = g++
FLAGS	 = -g -c -Wall
LFLAGS	 = 

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)

example.o: example.cpp
	$(CC) $(FLAGS) example.cpp

configparser.o: configparser.cpp
	$(CC) $(FLAGS) configparser.cpp -std=c++14


clean:
	rm -f $(OBJS) $(OUT)