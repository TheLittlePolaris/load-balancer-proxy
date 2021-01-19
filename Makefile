CC=g++
CFLAGS= -g -Wall -Werror
OUTPUT=proxy

all: proxy
# proxy_parse proxy_parse.h
proxy: proxy_o request_parser request_parser.h
	$(CC) $(CFLAGS) -o $(OUTPUT) proxy.o request_parser.o

# proxy_parse.o	
proxy_o: proxy.cpp
	$(CC) $(CFLAGS) -o proxy.o -c proxy.cpp

request_parser: request_parser.cpp request_parser.h
	$(CC) $(CFLAGS) -o request_parser.o -c request_parser.cpp

# proxy_parse: proxy_parse.cpp  proxy_parse.h
# 	$(CC) $(CFLAGS) -o proxy_parse.o -c proxy_parse.cpp

clean:
	rm -f $(OUTPUT) *.o
