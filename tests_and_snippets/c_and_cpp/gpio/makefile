# gcc -DLED_PIN=18 gpio.cpp -ogpio -lbcm_host

LED_PIN = -DLED_PIN=18 # Set to 0 for no LED, 18 for TAPR board

CXXFLAGS += -Wall -Werror -fmax-errors=5
LDLIBS += -lbcm_host
OUT = gpio

gpio:
	$(CXX) $(CXXFLAGS) $(LED_PIN) gpio.cpp -o$(OUT) $(LDLIBS)

clean:
	$(RM) *.o $(OUT)
