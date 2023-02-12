# g++ -std=c++17 -lstdc++fs filetime.cpp -o filetime

GXXFLAGS += -Wall -Werror -fmax-errors=5
GXXFLAGS += -lstdc++fs -std=c++17
OUT = monitor

filetime:
	g++ $(GXXFLAGS) filemonitor.cpp -o $(OUT)

clean:
	$(RM) *.o $(OUT)
