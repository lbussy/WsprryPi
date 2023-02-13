NAME		= filemonitor
GXXFLAGS 	+= -Wall -Werror -fmax-errors=5
GXXFLAGS 	+= -lstdc++fs -std=c++17
OUT 		= $(NAME)

$(NAME):
	g++ $(GXXFLAGS) $(NAME).cpp -o $(OUT)

clean:
	$(RM) *.o $(OUT)
