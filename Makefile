# Protocoale de comunicatii:
# Laborator 8: Multiplexare
# Makefile
CC=g++ -std=c++11 -Wall

CFLAGS = -Wall -g

all: build

build: client

client: requests.o buffer.o helpers.o client.o
	$(CC) $^ -o $@

client.o : client.cpp
	$(CC) $^ -c 

requests.o: requests.cpp
	$(CC) $^ -c 

buffer.o: buffer.cpp
	$(CC) $^ -c 

helpers.o: helpers.cpp
	$(CC) $^ -c 

clean:
	rm *.o client


#.PHONY: clean run_server run_subscriber