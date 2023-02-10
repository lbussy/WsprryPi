CFLAGS += -Wall
CXXFLAGS += -Wall -Werror -fmax-errors=5
CXXFLAGS += -I/usr/include -L/opt/vc/lib -L/usr/lib/arm-linux-gnueabihf 
CXXFLAGS += -Wno-psabi
LDLIBS += -lm -lbcm_host

all: filetime

filetime: filetime.cpp
	gcc -lstdc++fs filetime.cpp -o filetime

clean:
	$(RM) *.o filetime
