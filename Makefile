CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -O2

alf_ewg: main.cpp $(wildcard *.hpp)
	$(CXX) $(CXXFLAGS) -o $@ main.cpp

run: alf_ewg
	./alf_ewg

clean:
	rm -f alf_ewg stations.txt incidents.bin
