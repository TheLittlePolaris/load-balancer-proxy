#include "request_parser.h"

RequestParser::RequestParser()
{
    this->host = "";
    this->port = "";

    this->serverids[0] = "192.168.102.150";
    this->serverids[1] = "192.168.102.151";
    this->serverids[2] = "192.168.102.152";

    this->syncTargetIP = "192.168.102.135";
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

string RequestParser::getServerIP(string serverId)
{
    if (serverId == "A")
    {
        return this->serverids[0];
    }
    if (serverId == "B")
    {
        return this->serverids[1];
    }
    if (serverId == "C")
    {
        return this->serverids[2];
    }

    return this->serverids[0];
}
string RequestParser::getServerID(string serverIp)
{
    if (serverIp == this->serverids[0])
    {
        return "A";
    }

    if (serverIp == this->serverids[1])
    {
        return "B";
    }

    if (serverIp == this->serverids[2])
    {
        return "C";
    }

    return "A";
}

string RequestParser::getSyncTargetIP()
{
    return this->syncTargetIP;
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
        port = url.substr(port_pos + 1, path_pos - port_pos);
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

    // cout << "http://" << this->host << (this->port.length() > 0 ? ":" + this->port : "") << this->path << endl;
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
        // cout << "http://" << this->host << ":" << this->port << this->path << endl;
        return -1;
    }

    /* Free paRes, which was dynamically allocated by getaddrinfo */
    freeaddrinfo(paRes);
    return iSockfd;
}

void RequestParser::processRequest(const char *buffer, int clientfd, int buffer_len)
{
    int parseRes = this->parseRequest(buffer);
    if (parseRes >= 0)
    {
        this->host = this->serverids[0];
        this->port = "8080";
        int serverFd = this->createServerConnection();
        if (serverFd >= 0)
        {
            this->writeToServerSocket(buffer, serverFd, buffer_len);
            this->writeToClient(clientfd, serverFd);
            close(serverFd);
        }
    }
}

void RequestParser::writeToServerSocket(const char *buff_to_server, int sockfd, int buff_length)
{
    int totalsent = 0;
    int senteach;
    while (totalsent < buff_length)
    {
        if ((senteach = send(sockfd, (void *)(buff_to_server + totalsent), buff_length - totalsent, 0)) < 0)
        {
            cout << " Error in sending to server !" << endl;
            return;
        }
        totalsent += senteach;
    }
}

void RequestParser::writeToClientSocket(const char *buff_to_server, int sockfd, int buff_length)
{
    int totalsent = 0;
    int senteach;
    while (totalsent < buff_length)
    {
        if ((senteach = write(sockfd, (void *)(buff_to_server + totalsent), buff_length - totalsent)) < 0)
        {
            cout << "[E] Cannot send to server" << endl;
            return;
        }
        totalsent += senteach;
    }
}

void RequestParser::writeToClient(int clientFd, int serverFd)
{
    int iRecv;
    char buf[MAX_BUFFER_SIZE];
    while ((iRecv = recv(serverFd, buf, MAX_BUFFER_SIZE, 0)) > 0)
    {
        cout << "[I] Buffer from server: " << buf << endl;
        this->writeToClientSocket(buf, clientFd, iRecv + 1); // writing to client
        memset(buf, 0, iRecv + 1);
    }
}

RequestParser::~RequestParser() {}