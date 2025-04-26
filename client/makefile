# Compiler flags
CFLAGS := -c -Wall -Weffc++ -g -std=c++11 -Iinclude
LDFLAGS := -lboost_system -lpthread

# Targets
all: StompEMIClient

# Build the main executable
StompEMIClient: bin/ConnectionHandler.o bin/StompProtocol.o bin/event.o bin/ConcurrentHashMap.o bin/ConcurrentHashMapReversed.o bin/SummaryManager.o bin/StompClient.o
	g++ -o bin/StompEMIClient bin/ConnectionHandler.o bin/StompProtocol.o bin/event.o bin/ConcurrentHashMap.o bin/ConcurrentHashMapReversed.o bin/SummaryManager.o bin/StompClient.o $(LDFLAGS)

# Object file for ConnectionHandler
bin/ConnectionHandler.o: src/ConnectionHandler.cpp include/ConnectionHandler.h
	g++ $(CFLAGS) -o bin/ConnectionHandler.o src/ConnectionHandler.cpp

# Object file for StompProtocol
bin/StompProtocol.o: src/StompProtocol.cpp include/StompProtocol.h include/SummaryManager.h include/ConcurrentHashMapReversed.h
	g++ $(CFLAGS) -o bin/StompProtocol.o src/StompProtocol.cpp

# Object file for event
bin/event.o: src/event.cpp include/event.h
	g++ $(CFLAGS) -o bin/event.o src/event.cpp

# Object file for ConcurrentHashMap
bin/ConcurrentHashMap.o: src/ConcurrentHashMap.cpp include/ConcurrentHashMap.h
	g++ $(CFLAGS) -o bin/ConcurrentHashMap.o src/ConcurrentHashMap.cpp

# Object file for ConcurrentHashMapReversed
bin/ConcurrentHashMapReversed.o: src/ConcurrentHashMapReversed.cpp include/ConcurrentHashMapReversed.h
	g++ $(CFLAGS) -o bin/ConcurrentHashMapReversed.o src/ConcurrentHashMapReversed.cpp

# Object file for SummaryManager
bin/SummaryManager.o: src/SummaryManager.cpp include/SummaryManager.h include/event.h
	g++ $(CFLAGS) -o bin/SummaryManager.o src/SummaryManager.cpp

# Object file for StompClient (contains main)
bin/StompClient.o: src/StompClient.cpp include/ConnectionHandler.h include/StompProtocol.h include/SummaryManager.h include/ConcurrentHashMapReversed.h
	g++ $(CFLAGS) -o bin/StompClient.o src/StompClient.cpp

# Clean build artifacts
.PHONY: clean
clean:
	rm -f bin/*
