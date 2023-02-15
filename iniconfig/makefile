# Testing: make clean && make && ./config

NAME	= iniconfig
OBJS 	= $(NAME).o 
SOURCE	= $(NAME).cpp
OUT	    = $(NAME)
CC	    = g++
FLAGS	= -g -c -Wall
LFLAGS	= -lstdc++

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)

$(NAME).o: $(SOURCE)
	$(CC) $(FLAGS) $(SOURCE)

clean:
	rm -f $(OBJS) $(OUT)
