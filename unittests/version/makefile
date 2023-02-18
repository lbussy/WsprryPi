# Project Name (MAKE_SRC_NAM): git remote -v  | sed -rn '1s#.*/(.*)\.git.*#\1#p'
# Version (MAKE_SRC_TAG): git describe --tags
# Revision/Hash (MAKE_SRC_REV): git rev-parse --short HEAD
# Branch (MAKE_SRC_BRH): git rev-parse --abbrev-ref HEAD

NAME 		= version
CXXFLAGS 	+= -Wall -Werror -fmax-errors=5
CXXFLAGS 	+= -Wno-psabi -lstdc++fs -std=c++17
LDLIBS 		+= -lm -lbcm_host
OUT	 		= $(NAME)

all: $(NAME)

$(NAME):
	$(CXX) $(CXXFLAGS) $(NAME).cpp -o$(OUT) $(LDLIBS)

clean:
	$(RM) *.o $(NAME)
