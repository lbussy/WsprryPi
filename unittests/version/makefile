NAME 		= version
CXXFLAGS 	+= -Wall -Werror -fmax-errors=5
CXXFLAGS 	+= -Wno-psabi -lstdc++fs -std=c++17
LDLIBS 		+= -lm -lbcm_host
OUT	 		= $(NAME)

$(NAME): ver
	$(CXX) $(CXXFLAGS) -DMAKE_SRC_TAG="$(VER)" $(NAME).cpp -o$(OUT) $(LDLIBS)

ver:
	$(eval VER:=$(shell git describe --tags))

clean:
	$(RM) *.o $(NAME)
