prefix = /usr/local

LED_PIN = 18 # Set to 0 for no LED, 18 for TAPR board

CFLAGS += -Wall
CXXFLAGS += -D_GLIBCXX_DEBUG -std=c++11 -Wall -Werror -fmax-errors=5
CXXFLAGS += -I/usr/include -L/opt/vc/lib -L/usr/lib/arm-linux-gnueabihf
CXXFLAGS += -Wno-psabi
LDLIBS += -lm -lbcm_host

all: wspr

gpiotest: version.hpp version.cpp gpio.hpp gpio.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -DLED_PIN=$(LED_PIN) version.hpp version.cpp gpio.hpp gpio.cpp -ogpiotest $(LDLIBS)

mailbox.o: mailbox.c mailbox.h
	$(CC) $(CFLAGS) -c mailbox.c

wspr: mailbox.o wspr.hpp wspr.cpp mailbox.h
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -DLED_PIN=$(LED_PIN) mailbox.o version.hpp version.cpp wspr.hpp wspr.cpp -owspr $(LDLIBS)

clean:
	$(RM) *.o wspr gpiotest

.PHONY: install
install: wspr
	install -m 0755 wspr $(prefix)/bin

.PHONY: uninstall
uninstall:
	$(RM) $(prefix)/bin/wspr
