# Smart-Sensor-Simulation
#   make        -> kompilieren
#   make run    -> kompilieren und starten
#   make clean  -> aufraeumen

CXX      = c++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

QUELLEN  = $(wildcard *.cpp)
HEADER   = $(wildcard *.h)

smartsensor: $(QUELLEN) $(HEADER)
	$(CXX) $(CXXFLAGS) -o $@ $(QUELLEN)

run: smartsensor
	./smartsensor

clean:
	rm -f smartsensor messdaten.csv

.PHONY: run clean
