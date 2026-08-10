obj-m := rp1_gpclk_provider.o
ifeq ($(KUNIT),1)
obj-m += rp1_gpclk_provider_kunit.o
endif
ccflags-y := -I$(src)/include
