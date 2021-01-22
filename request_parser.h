#ifndef __REQUEST_PARSER__
#define __REQUEST_PARSER__

#include <iostream>
#include <string>
#include <sstream>

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

using namespace std;

#define MAX_BUFFER_SIZE 20000

class RequestParser
{
private:
    string host;
    string port;
    string path;
    string serverids[3];
    string syncTargetIP;

public:
    RequestParser();
    string getHost();
    string getPort();
    string getPath();
    string getServerIP(string serverId);
    string getServerID(string serverIp);

    void processRequest(const char *buffer, int clientfd, int buffer_len);
    int parseRequest(const char *request);
    void writeToServerSocket(const char *buffer, int socketfd, int length);
    void writeToClient(int clientFd, int serverFd);
    void writeToClientSocket(const char *buff_to_server, int sockfd, int buff_length);

    string getSyncTargetIP();

    int createServerConnection();
    ~RequestParser();
};

#endif // __REQUEST_PARSER__