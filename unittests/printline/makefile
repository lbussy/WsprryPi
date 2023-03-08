NAME 		= printline
CXXFLAGS 	+= -Wall -Werror -fmax-errors=5
CXXFLAGS 	+= -Wno-psabi -lstdc++fs -std=c++17
LDLIBS 		+= -lm -lbcm_host
OUT	 		= $(NAME)

all: $(NAME)

$(NAME):
	$(CXX) $(CXXFLAGS) $(NAME).cpp -o$(OUT) $(LDLIBS)

clean:
	$(RM) *.o $(NAME)
