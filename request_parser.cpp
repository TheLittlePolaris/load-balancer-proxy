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

string RequestParser::getCookieHeader(const char *request)
{
    string reqstr = string(request);
    istringstream is = istringstream(reqstr);
    string token;
    while (is >> token)
    {
        if (token == "Cookie:")
        {
            is >> token;
            return token;
        }
    }
    return "";
}

string RequestParser::getServerID(string serverIp)
{

    // int rndServer = rand() % 3;
    // this->host = this->serverids[rndServer]; // this->serverids[0];
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
        return -1;
    }
    this->path = path;

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

    cout << "Selected web server: " << this->host << endl;

    if (getaddrinfo(this->host.c_str(), "8080", &ahints, &paRes) != 0)
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
        return -1;
    }
    freeaddrinfo(paRes);
    return iSockfd;
}

string RequestParser::modifyBuffer(const char *buffer)
{
    string buf = string(buffer);
    string newHost = this->host + ":" + this->port;

    int index = buf.find("http://");
    int nextSlash = 0;
    for (int i = 0; i < 3; i++)
    {
        nextSlash = buf.find("/", nextSlash + 1);
    }
    buf.replace(index + 7, nextSlash - (index + 7), newHost);
    return buf;
}

void RequestParser::processRequest(const char *buffer, int clientfd, int buffer_len)
{
    int parseRes = this->parseRequest(buffer);
    if (parseRes >= 0)
    {
        string serverIp;
        string cookie = this->getCookieHeader(buffer);
        if (cookie.length())
        {
            int last = cookie.length() - 1;
            this->host = getServerIP(string(1, cookie.at(last)));
        }
        else
        {
            unsigned seed = time(0);
            srand(seed);
            int rndServer = rand() % 3;
            this->host = this->serverids[rndServer]; // this->serverids[0];
        }
        // this->host = "172.31.141.212";
        // this->port = "8080";
        int serverFd = this->createServerConnection();
        if (serverFd >= 0)
        {
            string serverId = this->getServerID(this->host);
            // cout << "ServerId: " << serverId << endl;
            // this->writeToServerSocket(modifiedBuffer.c_str(), serverFd, buffer_len);
            this->writeToServerSocket(buffer, serverFd, buffer_len);
            this->writeToClient(clientfd, serverFd, serverId);
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

string RequestParser::modifyResponse(const char *response)
{
    string strbuf = string(response);
    if (strbuf.compare(0, 5, "HTTP/") != 0)
        return "";
    istringstream is = istringstream(strbuf);
    int count = 0;
    while (count <= 5)
    {
        string token;
        is >> token;
        if (token == "Content-Type:")
        {
            string type;
            is >> type;
            if (type.compare(0, 6, "image/") == 0)
            {
                return "";
            }
        }
        count += 1;
    }
    ssize_t first_header = strbuf.find("\r\n");
    string serverId = this->getServerID(this->host);
    if (first_header != (ssize_t)string::npos)
    {
        string newheader = strbuf.substr(0, first_header + 2);
        strbuf.erase(0, first_header + 2);
        strbuf = newheader + "Set-Cookie: SERVERID=" + serverId + "\r\n" + strbuf;
    }

    return strbuf;
}

void RequestParser::writeToClientSocket(const char *buff_to_client, int sockfd, int buff_length, string serverId)
{
    int totalsent = 0;
    int senteach;
    string newbuf = this->modifyResponse(buff_to_client);
    buff_to_client = newbuf.length() ? newbuf.c_str() : buff_to_client;
    buff_length = newbuf.length() ? newbuf.length() : buff_length;
    while (totalsent < buff_length)
    {
        if ((senteach = send(sockfd, (buff_to_client + totalsent), buff_length - totalsent, 0)) < 0)
        {
            cout << "[E] Cannot send to server" << endl;
            return;
        }
        totalsent += senteach;
    }
}

void RequestParser::writeToClient(int clientFd, int serverFd, string serverId)
{
    int iRecv;
    char buf[MAX_BUFFER_SIZE];
    while ((iRecv = recv(serverFd, buf, MAX_BUFFER_SIZE, 0)) > 0)
    {
        this->writeToClientSocket(buf, clientFd, iRecv, serverId); // writing to client
        memset(buf, 0, iRecv);
    }
}

RequestParser::~RequestParser() {}