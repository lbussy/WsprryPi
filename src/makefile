# Copyright (C) 2023-2024 Lee C. Bussy (@LBussy)
# Created for WsprryPi version 1.2.1-4c76705 [devel].

prefix = /usr/local

LED_PIN = -DLED_PIN=18
CXXFLAGS += -Wall -Werror -fmax-errors=5 -static-libgcc
CXXFLAGS += -Wno-psabi -lstdc++fs -std=c++17
LDLIBS += -lm -lbcm_host
OUT = wspr

# Get version and branch directly using shell commands
VER := $(shell git describe --tags --always --dirty="-dirty" | sed -E 's/-([0-9]+)-g([a-f0-9]+)/-\2/')
BRH := $(shell git rev-parse --abbrev-ref HEAD)

all: wspr

mailbox.o:
	$(CXX) $(CXXFLAGS) -c mailbox.c

wspr: mailbox.o
	$(CXX) $(CXXFLAGS) $(LED_PIN) -DMAKE_SRC_TAG="$(VER)" -DMAKE_SRC_BRH="$(BRH)" mailbox.o wspr.cpp -o $(OUT) $(LDLIBS)

clean:
	$(RM) *.o wspr

.PHONY: install
install: wspr
	install -m 0755 $(OUT) $(prefix)/bin

.PHONY: uninstall
uninstall:
	$(RM) $(prefix)/bin/wspr

print-debug:
	@echo VER: $(VER)
	@echo BRH: $(BRH)
