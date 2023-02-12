OBJS	 = test.o 
SOURCE	 = test.cpp
OUT	     = test
CC	     = g++
FLAGS	 = -g -c -Wall

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT)

test.o: $(SOURCE)
	$(CC) $(FLAGS) $(SOURCE)

clean:
	rm -f $(OBJS) $(OUT)
