CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
CXXFLAGS += $(shell pkg-config --cflags libgit2)
LDFLAGS  = $(shell pkg-config --libs libgit2)

TARGET = git_app
SOURCES = git_interface.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean
