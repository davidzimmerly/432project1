CC=g++
CPPFLAGS = -g -Wall
CXXFLAGS = -std=gnu++11

all: server client

server: server.o 
	$(CC) -o server server.o $(LIBS)

server.o: server.cpp shared.h

client: client.o 
	$(CC) -o client client.o $(LIBS)

client.o: client.cpp shared.h	

clean:
	rm -f server server.o client client.o