CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror

all: rp1_gpclk_provider_core_test

rp1_gpclk_provider_core_test: rp1_gpclk_provider_core.c rp1_gpclk_provider_core_test.c
	$(CC) $(CFLAGS) $^ -o $@

test: rp1_gpclk_provider_core_test
	./rp1_gpclk_provider_core_test

clean:
	rm -f rp1_gpclk_provider_core_test
