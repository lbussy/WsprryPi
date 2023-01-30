prefix=/usr/local

CFLAGS += -Wall
CXXFLAGS += -D_GLIBCXX_DEBUG -std=c++11 -Wall -Werror -fmax-errors=5
CXXFLAGS += -I/usr/include -L/opt/vc/lib -L/usr/lib/arm-linux-gnueabihf
CXXFLAGS += -Wno-psabi
LDLIBS += -lm -lbcm_host

all: wspr

mailbox.o: mailbox.c mailbox.h
	$(CC) $(CFLAGS) -c mailbox.c

wspr: mailbox.o wspr.cpp mailbox.h
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(PI_VERSION) mailbox.o wspr.cpp -owspr $(LDLIBS)

clean:
	$(RM) *.o wspr

.PHONY: install
install: wspr
	install -m 0755 wspr $(prefix)/bin

.PHONY: uninstall
uninstall:
	$(RM) $(prefix)/bin/wspr
