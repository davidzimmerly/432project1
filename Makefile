CC=g++
CFLAGS = -g 

all: server client

server: server.o 
	$(CC) -o server server.o $(LIBS)

server.o: server.cpp

client: client.o 
	$(CC) -o client client.o $(LIBS)

client.o: client.cpp

clean:
	rm -f server server.o client client.o