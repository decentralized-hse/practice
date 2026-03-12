CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Iinclude
TARGET   = gossipsync

all: $(TARGET)

$(TARGET): src/main.cpp include/*.hpp
	$(CXX) $(CXXFLAGS) -o $@ src/main.cpp

clean:
	rm -f $(TARGET)
