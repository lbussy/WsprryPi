OBJS	 = testconfig.o 
SOURCE	 = testconfig.cpp
OUT	     = config
CC	     = g++
FLAGS	 = -g -c -Wall
LFLAGS	 = -lstdc++

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)

config.o: $(SOURCE)
	$(CC) $(FLAGS) $(SOURCE)

clean:
	rm -f $(OBJS) $(OUT)
