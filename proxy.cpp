

#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <sys/epoll.h>
#include <iostream>
#include <string>
#include <cstring>
#include <array>

#include "request_parser.h"

using namespace std;
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

	int pid = fork();
	if (pid < 0)
	{
		perror("[E] Fork failed");
		return false;
	}
	if (pid == 0)
	{
		RequestParser *rp = new RequestParser();
		rp->processRequest(buf, fd, count);
		delete rp;
		exit(0);
	}
	close(fd);
	return false;
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
		auto n = epoll_wait(epollfd, events.data(), max_events, 1);
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
	close(socketfd);
	return 0;
}