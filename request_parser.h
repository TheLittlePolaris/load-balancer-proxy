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

class RequestParser
{
private:
    string host;
    string port;
    string path;

public:
    RequestParser();
    string getHost();
    string getPort();
    string getPath();
    int parseRequest(const char *request);
    int createServerConnection();
    ~RequestParser();
};

#endif // __REQUEST_PARSER__