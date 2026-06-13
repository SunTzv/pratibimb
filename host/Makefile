CXX = g++
CXXFLAGS = -O3 -Wall -Wno-ignored-attributes -std=c++14
TARGET = pratibimb_host

all: $(TARGET)

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) main.cpp -o $(TARGET)

clean:
	rm -f $(TARGET)
