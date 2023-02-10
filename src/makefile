prefix = /usr/local

LED_PIN = -DLED_PIN=18 # Set to 0 for no LED, 18 for TAPR board

CFLAGS += -Wall
CXXFLAGS += -Wall -Werror -fmax-errors=5
CXXFLAGS += -Wno-psabi
LDLIBS += -lm -lbcm_host
OUT = wspr

all: wspr

mailbox.o:
	$(CC) $(CFLAGS) -c mailbox.c

wspr: mailbox.o
	$(CXX) $(CXXFLAGS) $(LED_PIN) mailbox.o wspr.cpp -o$(OUT) $(LDLIBS)

clean:
	$(RM) *.o wspr

.PHONY: install
install: wspr
	install -m 0755 wspr $(prefix)/bin

.PHONY: uninstall
uninstall:
	$(RM) $(prefix)/bin/wspr
