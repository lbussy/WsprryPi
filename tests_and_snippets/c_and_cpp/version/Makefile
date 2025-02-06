# -----------------------------------------------------------------------------
# MIT License
# 
# Copyright (c) 2025 Lee C. Bussy (@lbussy)
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# provided to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
# -----------------------------------------------------------------------------

NAME        = version
CXXFLAGS    += -Wall -Werror -fmax-errors=5
CXXFLAGS    += -Wno-psabi -lstdc++fs -std=c++17 -DDEBUG_MAIN_VERSION
LDLIBS      += -lm
OUT         = $(NAME)
EXE	        = $(OUT)

# Extract project details from the Git repository
PRJ := $(shell git config --local remote.origin.url | sed -n 's#.*/\([^.]*\)\.git#\1#p')
BRH := $(shell git rev-parse --abbrev-ref HEAD | sed -E 's/[^a-zA-Z0-9]/-/g')

# Determine the semantic version and append branch or build metadata if necessary
VER := $(shell git describe --tags --match "v[0-9]*" --match "[0-9]*" --abbrev=0 2>/dev/null | sed -E 's/^v?([0-9]+\.[0-9]+\.[0-9]+)/\1/')
PRE := $(shell if [ "$(BRH)" != "main" ] && [ "$(BRH)" != "master" ]; then echo "$(BRH)"; fi)
VER := $(shell if ! echo "$(VER)" | grep -q -- '-'; then [ -n "$(PRE)" ] && echo "$(VER)-$(PRE)" || echo "$(VER)"; else echo "$(VER)"; fi)
VER := $(shell if [ -z "$(git describe --tags --exact-match 2>/dev/null)" ]; then echo "$(VER)+$(shell git rev-parse --short HEAD)"; else echo "$(VER)"; fi)

# Define additional macros for the compiler
CXXFLAGS += -DMAKE_TAG=$(VER) -DMAKE_BRH=$(BRH) -DMAKE_EXE=$(EXE) -DMAKE_PRJ=$(PRJ)

.PHONY: clean ver

$(NAME): ver
	$(CXX) $(CXXFLAGS) $(NAME).cpp -o $(OUT) $(LDLIBS)

ver:
	$(eval VER:=$(shell git describe --tags --abbrev=0 --always || echo "0.0.0"))

clean:
	$(RM) *.o $(NAME)
