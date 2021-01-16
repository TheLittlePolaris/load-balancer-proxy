#include <bits/stdc++.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/wait.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <poll.h>

#include <sys/epoll.h>
#include <iostream>
#include <string>
#include <cstring>
#include <array>

#include "proxy_parse.h"

using namespace std;

#define MAX_BUFFER_SIZE 5000

char *convert_Request_to_string(struct ParsedRequest *req)
{

	/* Set headers */
	ParsedHeader_set(req, "Host", req->host);
	ParsedHeader_set(req, "Connection", "close");

	int iHeadersLen = ParsedHeader_headersLen(req);

	char *headersBuf;

	headersBuf = (char *)malloc(iHeadersLen + 1);

	if (headersBuf == NULL)
	{
		cout << " Error in memory allocation  of headersBuffer !" << endl;
		exit(1);
	}

	ParsedRequest_unparse_headers(req, headersBuf, iHeadersLen);
	headersBuf[iHeadersLen] = '\0';

	int request_size = strlen(req->method) + strlen(req->path) + strlen(req->version) + iHeadersLen + 4;

	char *serverReq;

	serverReq = (char *)malloc(request_size + 1);

	if (serverReq == NULL)
	{
		cout << " Error in memory allocation for serverrequest !" << endl;
		exit(1);
	}

	serverReq[0] = '\0';
	strcpy(serverReq, req->method);
	strcat(serverReq, " ");
	strcat(serverReq, req->path);
	strcat(serverReq, " ");
	strcat(serverReq, req->version);
	strcat(serverReq, "\r\n");
	strcat(serverReq, headersBuf);

	free(headersBuf);

	return serverReq;
}

int createserverSocket(char *pcAddress, char *pcPort)
{
	struct addrinfo ahints;
	struct addrinfo *paRes;

	int iSockfd;

	/* Get address information for stream socket on input port */
	memset(&ahints, 0, sizeof(ahints));
	ahints.ai_family = AF_UNSPEC;
	ahints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(pcAddress, pcPort, &ahints, &paRes) != 0)
	{
		cout << " Error in server address format !" << endl;
		exit(1);
	}

	/* Create and connect */
	if ((iSockfd = socket(paRes->ai_family, paRes->ai_socktype, paRes->ai_protocol)) < 0)
	{
		cout << " Error in creating socket to server !" << endl;
		exit(1);
	}
	if (connect(iSockfd, paRes->ai_addr, paRes->ai_addrlen) < 0)
	{
		cout << " Error in connecting to server !" << endl;
		exit(1);
	}

	/* Free paRes, which was dynamically allocated by getaddrinfo */
	freeaddrinfo(paRes);
	return iSockfd;
}

void writeToserverSocket(const char *buff_to_server, int sockfd, int buff_length)
{
	int totalsent = 0;
	int senteach;
	while (totalsent < buff_length)
	{
		if ((senteach = send(sockfd, (void *)(buff_to_server + totalsent), buff_length - totalsent, 0)) < 0)
		{
			cout << " Error in sending to server !" << endl;
			exit(1);
		}
		totalsent += senteach;
	}
}

void writeToclientSocket(const char *buff_to_server, int sockfd, int buff_length)
{
	int totalsent = 0;
	int senteach;
	while (totalsent < buff_length)
	{
		if ((senteach = send(sockfd, (void *)(buff_to_server + totalsent), buff_length - totalsent, 0)) < 0)
		{
			cout << " Error in sending to server !" << endl;
			exit(1);
		}
		totalsent += senteach;
	}
}

void writeToClient(int Clientfd, int Serverfd)
{
	int iRecv;
	char buf[MAX_BUFFER_SIZE];

	while ((iRecv = recv(Serverfd, buf, MAX_BUFFER_SIZE, 0)) > 0)
	{
		writeToclientSocket(buf, Clientfd, iRecv); // writing to client
		memset(buf, 0, sizeof buf);
	}
}

int processData(int fd, char *request)
{

	int req_len = strlen(request);
	struct ParsedRequest *req; // contains parsed request

	req = ParsedRequest_create();
	int code = ParsedRequest_parse(req, request, req_len);
	switch (code)
	{
	case OTHER_PROBLEM:
		return -1;
	case FILTERED_DOMAIN:
		ParsedRequest_destroy(req);
		return -1;
	case PROTOCOL_NOT_ACCEPTED:
		ParsedRequest_destroy(req);
		return -1;
	case METHOD_NOT_ACCEPTED:
		ParsedRequest_destroy(req);
		return -1;
	}
	if (req->port == NULL)		  // if port is not mentioned in URL, we take default as 80
		req->port = (char *)"80"; // default web server port

	/*final request to be sent*/

	char *browser_req = convert_Request_to_string(req);
	int iServerfd;

	iServerfd = createserverSocket(req->host, req->port);

	writeToserverSocket(browser_req, iServerfd, req_len);
	cout << "Write to server socket" << endl;
	writeToClient(fd, iServerfd);
	cout << "Write to client" << endl;
	ParsedRequest_destroy(req);
	cout << "Destroy req" << endl;
	close(iServerfd);
	return 0;
}

constexpr int max_events = 32;

auto create_and_bind(string const &port)
{
	struct addrinfo hints;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;	 /* Return IPv4 and IPv6 choices */
	hints.ai_socktype = SOCK_STREAM; /* TCP */
	hints.ai_flags = AI_PASSIVE;	 /* All interfaces */

	struct addrinfo *result;
	int sockt = getaddrinfo(nullptr, port.c_str(), &hints, &result);
	if (sockt != 0)
	{
		cerr << "[E] getaddrinfo failed\n";
		return -1;
	}

	struct addrinfo *rp;
	int socketfd;
	for (rp = result; rp != nullptr; rp = rp->ai_next)
	{
		socketfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (socketfd == -1)
		{
			continue;
		}

		sockt = bind(socketfd, rp->ai_addr, rp->ai_addrlen);
		if (sockt == 0)
		{
			break;
		}

		close(socketfd);
	}

	if (rp == nullptr)
	{
		cerr << "[E] bind failed\n";
		return -1;
	}

	freeaddrinfo(result);

	return socketfd;
}

auto make_socket_nonblocking(int socketfd)
{
	int flags = fcntl(socketfd, F_GETFL, 0);
	if (flags == -1)
	{
		cerr << "[E] fcntl failed (F_GETFL)\n";
		return false;
	}

	flags |= O_NONBLOCK;
	int s = fcntl(socketfd, F_SETFL, flags);
	if (s == -1)
	{
		cerr << "[E] fcntl failed (F_SETFL)\n";
		return false;
	}

	return true;
}

auto accept_connection(int socketfd, struct epoll_event &event, int epollfd)
{
	struct sockaddr in_addr;
	socklen_t in_len = sizeof(in_addr);
	int infd = accept(socketfd, &in_addr, &in_len);
	if (infd == -1)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK) // Done processing incoming connections
		{
			return false;
		}
		else
		{
			cerr << "[E] accept failed\n";
			return false;
		}
	}

	string hbuf(NI_MAXHOST, '\0');
	string sbuf(NI_MAXSERV, '\0');

	if (!make_socket_nonblocking(infd))
	{
		cerr << "[E] make_socket_nonblocking failed\n";
		return false;
	}

	event.data.fd = infd;
	event.events = EPOLLIN | EPOLLET;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, infd, &event) == -1)
	{
		cerr << "[E] epoll_ctl failed\n";
		return false;
	}

	return true;
}

auto read_data(int fd)
{
	char buf[MAX_BUFFER_SIZE];
	auto count = read(fd, buf, MAX_BUFFER_SIZE);
	if (count == -1)
	{
		if (errno == EAGAIN) // read all data
		{
			return false;
		}
	}
	else if (count == 0) // EOF - remote closed connection
	{
		cout << "[I] Close " << fd << "\n";
		close(fd);
		return false;
	}

	cout << fd << " says: " << buf;
	processData(fd, buf);
	return true;
}

int main()
{
	auto socketfd = create_and_bind("8888");
	if (socketfd == -1)
	{
		return 1;
	}

	if (!make_socket_nonblocking(socketfd))
	{
		return 1;
	}

	if (listen(socketfd, SOMAXCONN) == -1)
	{
		cerr << "[E] listen failed\n";
		return 1;
	}

	int epollfd = epoll_create1(0);
	if (epollfd == -1)
	{
		cerr << "[E] epoll_create1 failed\n";
		return 1;
	}

	struct epoll_event event;
	event.data.fd = socketfd;
	event.events = EPOLLIN | EPOLLET;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, socketfd, &event) == -1)
		if (epollfd == -1)
		{
			cerr << "[E] epoll_ctl failed\n";
			return 1;
		}

	array<struct epoll_event, max_events> events;

	while (1)
	{
		auto n = epoll_wait(epollfd, events.data(), max_events, -1);
		for (int i = 0; i < n; ++i)
		{
			if (events[i].events & EPOLLERR ||
				events[i].events & EPOLLHUP ||
				!(events[i].events & EPOLLIN)) // error
			{
				cerr << "[E] epoll event error\n";
				close(events[i].data.fd);
			}
			else if (socketfd == events[i].data.fd) // new connection
			{
				while (accept_connection(socketfd, event, epollfd))
				{
				}
			}
			else // data to read
			{
				auto fd = events[i].data.fd;
				while (read_data(fd))
				{
				}
			}
		}
	}

	cout << "BREAK LOOP" << endl;

	close(socketfd);
	return 0;
}