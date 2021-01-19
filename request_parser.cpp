#include "request_parser.h"

RequestParser::RequestParser()
{
    this->host = "";
    this->port = "";
}

string RequestParser::getHost()
{
    return this->host;
}

string RequestParser::getPort()
{
    return this->port;
}

string RequestParser::getPath()
{
    return this->path;
}

int RequestParser::parseRequest(const char *request)
{
    string reqStr = string(request);
    istringstream ss(reqStr);
    string method, url, protocol, currentToken;
    ss >> method >> url >> protocol;
    // cout << "Methid is: |" << method << "|, url is |" << url << "|, protocol is: |" << protocol << "|." << endl;
    if (protocol.compare(0, 5, "HTTP/") != 0)
    {
        cout << "[E] Protocol is invalid" << endl;
        return -1;
    }
    if (url.compare(0, 7, "http://") == 0)
    {
        url.erase(0, 7);
    }
    else
    {
        cout << "[E] Url protocol invalid" << endl;
        return -1;
    }
    ssize_t port_pos = url.find(":");
    ssize_t path_pos = url.find("/");
    string host, port, path;
    if (port_pos != (ssize_t)string::npos)
    {
        host = url.substr(0, port_pos);
        // url.erase(port_pos + 1);
        port = url.substr(port_pos + 1, path_pos - port_pos);
        // url.erase()
        path = url.substr(path_pos);
    }
    else
    {
        port = "80";
        if (path_pos != (ssize_t)string::npos)
        {
            host = url.substr(0, path_pos);
            path = url.substr(path_pos);
        }
        else
        {
            host = url.substr(0);
            path = "/";
        }
    }
    this->host = host;
    this->port = port;
    this->path = path;

    cout << "http://" << this->host << (this->port.length() > 0 ? ":" + this->port : "") << this->path << endl;
    return 0;
}

int RequestParser::createServerConnection()
{

    struct addrinfo ahints;
    struct addrinfo *paRes;

    int iSockfd;
    /* Get address information for stream socket on input port */
    memset(&ahints, 0, sizeof(ahints));
    ahints.ai_family = AF_UNSPEC;
    ahints.ai_socktype = SOCK_STREAM;
    ahints.ai_flags = AI_PASSIVE;
    if (getaddrinfo(this->host.c_str(), this->port.c_str(), &ahints, &paRes) != 0)
    {
        cout << "[E] getaddrinfo failed!" << endl;
        return -1;
    }

    /* Create and connect */
    if ((iSockfd = socket(paRes->ai_family, paRes->ai_socktype, paRes->ai_protocol)) < 0)
    {
        cout << "[E] Create socket error!" << endl;
        return -1;
    }
    if (connect(iSockfd, paRes->ai_addr, paRes->ai_addrlen) < 0)
    {
        cout << "[E] Cannot connect to server:" << endl;
        cout << "http://" << this->host << ":" << this->port << this->path << endl;
        return -1;
    }

    /* Free paRes, which was dynamically allocated by getaddrinfo */
    freeaddrinfo(paRes);
    return iSockfd;
}

RequestParser::~RequestParser() {}