prefix = /usr/local

LED_PIN = 18 # Is set to 0 for no LED, default for TAPR board in target builds

CFLAGS   += -Wall
CXXFLAGS += -D_GLIBCXX_DEBUG -std=c++11 -Wall -Werror -fmax-errors=5
CXXFLAGS += -I/usr/include -L/opt/vc/lib -L/usr/lib/arm-linux-gnueabihf
CXXFLAGS += -Wno-psabi
LDLIBS   += -lm -lbcm_host

all: wspr tapr

gpiotest: version.hpp version.cpp gpio.hpp gpio.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -DLED_PIN=$(LED_PIN) version.hpp version.cpp gpio.hpp gpio.cpp -ogpiotest $(LDLIBS)

mailbox.o: mailbox.c mailbox.h
	$(CC) $(CFLAGS) -c mailbox.c

wspr: LED_PIN = 0 # Set to 0 for no LED, 18 for TAPR board
wspr: mailbox.o wspr.hpp wspr.cpp mailbox.h
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -DLED_PIN=$(LED_PIN) mailbox.o version.hpp version.cpp wspr.hpp wspr.cpp -owspr $(LDLIBS)

tapr: mailbox.o wspr.hpp wspr.cpp mailbox.h
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -DLED_PIN=$(LED_PIN) mailbox.o version.hpp version.cpp wspr.hpp wspr.cpp -owspr-tapr $(LDLIBS)

clean:
	$(RM) *.o wspr wspr-tapr gpiotest

.PHONY: install
install: wspr
	install -m 4755 wspr $(prefix)/bin
	install -m 4755 wspr-tapr $(prefix)/bin

.PHONY: uninstall
uninstall:
	$(RM) $(prefix)/bin/wspr
	$(RM) $(prefix)/bin/wspr-tapr
