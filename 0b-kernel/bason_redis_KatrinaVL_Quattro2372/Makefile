
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pthread -I./src
LDFLAGS = -pthread

SOURCES = src/bason_codec.cpp \
          src/bason_redis_store.cpp \
          src/persistence_manager.cpp \
          src/bason_redis_server.cpp \
          src/bason_redis_client.cpp

OBJECTS = $(SOURCES:.cpp=.o)

all: basonredis-server basonredis-client test_store

basonredis-server: $(OBJECTS) examples/server_example.o
	$(CXX) $(LDFLAGS) -o $@ $^

basonredis-client: $(OBJECTS) examples/client_example.o
	$(CXX) $(LDFLAGS) -o $@ $^

test_store: $(OBJECTS) tests/test_store.o
	$(CXX) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) examples/*.o tests/*.o
	rm -f basonredis-server basonredis-client test_store

test: test_store
	./test_store

.PHONY: all clean test
