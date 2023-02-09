OBJS	= config.o configparser.o
SOURCE	= config.cpp configparser.cpp
HEADER	= configparser.hpp
OUT	= config
CC	 = g++
FLAGS	 = -g -c -Wall
LFLAGS	 = 

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)

config.o: config.cpp
	$(CC) $(FLAGS) config.cpp

configparser.o: configparser.cpp
	$(CC) $(FLAGS) configparser.cpp -std=c++14

clean:
	rm -f $(OBJS) $(OUT)
