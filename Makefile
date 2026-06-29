CXX      = g++
CXXFLAGS = -std=c++17 -Wall

SRCS = main.cpp Sensor.cpp Sensoren.cpp SensorStation.cpp

TARGET = smartsensor

all:
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)
