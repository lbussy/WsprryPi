prefix = /usr/local

LED_PIN = -DLED_PIN=18 # Set to 0 for no LED, 18 for TAPR board
CXXFLAGS += -Wall -Werror -fmax-errors=5
CXXFLAGS += -Wno-psabi -lstdc++fs -std=c++17
LDLIBS += -lm -lbcm_host
OUT = wspr

all: wspr

mailbox.o:
	$(CXX) $(CXXFLAGS) -c mailbox.c

wspr: ver mailbox.o
	$(CXX) $(CXXFLAGS) $(LED_PIN) -DMAKE_SRC_TAG="$(VER)" mailbox.o wspr.cpp -o$(OUT) $(LDLIBS)

ver:
	$(eval VER:=$(shell git describe --tags))

clean:
	$(RM) *.o wspr

.PHONY: install
install: wspr
	install -m 0755 $(OUT) $(prefix)/bin

.PHONY: uninstall
uninstall:
	$(RM) $(prefix)/bin/wspr
