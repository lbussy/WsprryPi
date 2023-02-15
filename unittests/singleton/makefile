NAME	= testsingle
OBJS	 = $(NAME).o 
SOURCE	 = $(NAME).cpp
OUT	     = $(NAME)
CC	     = g++
FLAGS	 = -g -c -Wall

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT)

$(NAME).o: $(SOURCE)
	$(CC) $(FLAGS) $(SOURCE)

clean:
	rm -f $(OBJS) $(OUT)
