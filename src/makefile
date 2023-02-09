prefix = /usr/local

LED_PIN = 18 # Set to 0 for no LED, 18 for TAPR board

CFLAGS += -Wall
CXXFLAGS += -D_GLIBCXX_DEBUG -std=c++11 -Wall -Werror -fmax-errors=5
CXXFLAGS += -I/usr/include -L/opt/vc/lib -L/usr/lib/arm-linux-gnueabihf
CXXFLAGS += -Wno-psabi
LDLIBS += -lm -lbcm_host

all: wspr

mailbox.o: mailbox.c mailbox.h
	$(CC) $(CFLAGS) -c mailbox.c

configparser.o: configparser.cpp configparser.hpp
	$(CC) $(FLAGS) configparser.cpp -std=c++14

wspr: configparser.o mailbox.o wspr.hpp wspr.cpp mailbox.h configparser.h
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -DLED_PIN=$(LED_PIN) mailbox.o version.hpp version.cpp wspr.hpp wspr.cpp -owspr $(LDLIBS)

clean:
	$(RM) *.o wspr

.PHONY: install
install: wspr
	install -m 0755 wspr $(prefix)/bin

.PHONY: uninstall
uninstall:
	$(RM) $(prefix)/bin/wspr
