prefix = /usr/local

LED_PIN = -DLED_PIN=18
CXXFLAGS += -Wall -Werror -fmax-errors=5  -static-libgcc
CXXFLAGS += -Wno-psabi -lstdc++fs -std=c++17
OUT = wsprrypi

all: wsprrypi

mailbox.o:
	$(CXX) $(CXXFLAGS) -c mailbox.c

wsprrypi: ver brh mailbox.o
	$(CXX) $(CXXFLAGS) $(LED_PIN) -DMAKE_SRC_TAG="$(VER)" -DMAKE_SRC_BRH="$(BRH)" mailbox.o wsprrypi.cpp -o$(OUT) $(LDLIBS)

ver:
	$(eval VER:=$(shell git describe --tags))

brh:
	$(eval BRH:=$(shell git rev-parse --abbrev-ref HEAD))

clean:
	$(RM) *.o wsprrypi

.PHONY: install
install: wsprrypi
	install -m 0755 $(OUT) $(prefix)/bin

.PHONY: uninstall
uninstall:
	$(RM) $(prefix)/bin/wsprrypi
