CC=g++
CFLAGS= -g -Wall -Werror
OUTPUT=proxy_server

all: proxy

proxy: proxy.cpp
	$(CC) $(CFLAGS) -o proxy_parse.o -c proxy_parse.cpp 
	$(CC) $(CFLAGS) -o proxy.o -c proxy.cpp
	$(CC) $(CFLAGS) -o $(OUTPUT) proxy_parse.o proxy.o 
	

clean:
	rm -f $(OUTPUT) *.o
