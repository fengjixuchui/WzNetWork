
#include "Socket.h"

#ifdef _WIN32
#include <winsock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

namespace wz
{
namespace network
{

/* tcp socket */
#ifdef _WIN32
struct TcpSocketHandle
{
    WORD sockVersion;
    WSADATA data;
    int wsaStartupResult;

    sockaddr_in serverAddress;
    SOCKET socket;
    int connectStatus;

    enum CONNECT_STATUS
    {
        DISCONNECT = -1,
        CONNECTED = 1
    };
};
#else
struct TcpSocketHandle
{
};
#endif

TcpSocket::TcpSocket()
    : handle(NULL),
      receiveBuffer(NULL),
      receiveBufferSize(0)
{
    init();
}

TcpSocket::TcpSocket(const std::string &ip, const int &port)
    : handle(NULL),
      receiveBuffer(NULL),
      receiveBufferSize(0)
{
    init();

    if (NULL != handle)
    {
        switch (inet_pton(AF_INET, ip.data(), &(handle->serverAddress.sin_addr)))
        {
        case 0: /* invalid ip */
            printf("[TcpSocket::TcpSocket(const std::string &ip, const int &port)]: "
                   "init ip failed with invalid value,please check or reset ip by "
                   "setIP(const std::string& ip).\n");
            break;
        case -1:
            printf("[TcpSocket::TcpSocket(const std::string &ip, const int &port)]: "
                   "init ip failed with function 'inet_pton' inner error,please try "
                   "after with setIP(const std::string& ip).\n");
            break;
        case 1: /* set ip success */
        default:
            break;
        }

        if (0 <= port && port <= 65535)
        {
            handle->serverAddress.sin_port = htons(port);
        }
        else
        {
            printf("[TcpSocket::TcpSocket(const std::string &ip, const int &port)]: "
                   "init port failed with invalid port,please check or reset port by "
                   "setPort(const int& port).\n");
        }
    }
}

TcpSocket::~TcpSocket()
{
    release();
}

void TcpSocket::setIp(const std::string &ip)
{
    if (NULL != handle && 0 == handle->wsaStartupResult)
    {
        switch (inet_pton(AF_INET, ip.data(), &(handle->serverAddress.sin_addr)))
        {
        case 0: /* invalid ip */
            printf("[TcpSocket::setIp(const std::string &ip)]: "
                   "set ip failed with invalid ip,please check.\n");
            break;
        case -1:
            printf("[TcpSocket::setIp(const std::string &ip)]: "
                   "set ip failed with function 'inet_pton' inner error,please debug.\n");
            break;
        case 1: /* set ip success */
        default:
            break;
        }
    }
}

std::string TcpSocket::getIp()
{
    char ip[16] = {'0'};
    inet_ntop(AF_INET, &(handle->serverAddress.sin_addr), ip, sizeof(ip));
    return ip;
}

void TcpSocket::setPort(const int &port)
{
    if (NULL != handle && 0 == handle->wsaStartupResult)
    {
        if (0 <= port && port <= 65535)
        {
            handle->serverAddress.sin_port = htons(port);
        }
        else
        {
            printf("[TcpSocket::setPort(const int &port)]: "
                   "set port failed by incorrect port,please check.\n");
        }
    }
}

int TcpSocket::getPort()
{
    return ntohs(handle->serverAddress.sin_port);
}

bool TcpSocket::connect()
{
    handle->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (INVALID_SOCKET == handle->socket)
    {
        printf("[TcpSocket::connect()]: connect failed with invalid socket!");
        return false;
    }
    int connectResult = ::connect(handle->socket,
                                  (sockaddr *)&(handle->serverAddress),
                                  sizeof(handle->serverAddress));
    if (SOCKET_ERROR == connectResult)
    {
        printf("[TcpSocket::connect()]: connect failed with %s:%d,"
               "please check if the server is available.",
               getIp().data(),
               getPort());
        handle->connectStatus = TcpSocketHandle::CONNECT_STATUS::DISCONNECT;
        return false;
    }

    handle->connectStatus = TcpSocketHandle::CONNECT_STATUS::CONNECTED;
    return true;
}

bool TcpSocket::isConnected()
{
    if (TcpSocketHandle::CONNECT_STATUS::CONNECTED == handle->connectStatus)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void TcpSocket::disconnect()
{
    if (INVALID_SOCKET != handle->socket && 
        TcpSocketHandle::CONNECT_STATUS::DISCONNECT != handle->connectStatus)
    {
        closesocket(handle->socket);
        handle->connectStatus = TcpSocketHandle::CONNECT_STATUS::DISCONNECT;
        handle->socket = INVALID_SOCKET;
    }
}

int TcpSocket::send(const char *data,const int& length)
{
    return ::send(handle->socket,data,length,0);
}

int TcpSocket::send(const std::string& data)
{
    return send(data.data(),data.size());
}

void TcpSocket::resizeReceiveBuffer(const int &size)
{
    if (receiveBufferSize > 0)
    {
        delete[] receiveBuffer;
        receiveBuffer = NULL;

        receiveBufferSize = size;
        receiveBuffer = new char[receiveBufferSize];
    }
}

void TcpSocket::receive(const char *data, const int &length)
{
    
}

void TcpSocket::init()
{
    handle = new TcpSocketHandle;
    if (NULL != handle)
    {
        handle->sockVersion = MAKEWORD(2, 2);
        handle->wsaStartupResult = WSAStartup(handle->sockVersion, &(handle->data));
        if (0 != handle->wsaStartupResult)
        {
            printf("[TcpSocket::init()]: init failed with socket version. Try again , "
                   "debug source or contact author!\n");
        }
        else
        {
            handle->serverAddress.sin_family = AF_INET;
            switch (inet_pton(AF_INET, "0.0.0.0", &(handle->serverAddress.sin_addr)))
            {
            case -1:
                printf("[TcpSocket::init()]: "
                       "init ip failed with function 'inet_pton' inner error,please try "
                       "after with setIP(const std::string& ip).\n");
                break;
            case 0: /* invalid ip */
            case 1: /* set ip success */
            default:
                break;
            }
            handle->serverAddress.sin_port = htons(0);
            handle->socket = INVALID_SOCKET;
            handle->connectStatus = TcpSocketHandle::CONNECT_STATUS::DISCONNECT;
        }
    }
    else
    {
        printf("[TcpSocket::init()]: handle is NULL,new TcpTcpSocketHandle failed.\n");
    }

    receiveBufferSize = 4096;
    receiveBuffer = new char[receiveBufferSize];
}

void TcpSocket::release()
{
    if (INVALID_SOCKET != handle->socket)
    {
        closesocket(handle->socket);
        handle->connectStatus = TcpSocketHandle::CONNECT_STATUS::DISCONNECT;
        handle->socket = INVALID_SOCKET;
    }
    if (0 != handle->wsaStartupResult)
    {
        WSACleanup();
        handle->wsaStartupResult = 0;
    }
    if (NULL != handle)
    {
        delete handle;
        handle = NULL;
    }
    if (NULL != receiveBuffer)
    {
        delete[] receiveBuffer;
        receiveBuffer = NULL;
        receiveBufferSize = 0;
    }
}

/* udp socket */
#ifdef _WIN32
struct UdpSocketHandle
{
};
#else
struct UdpSocketHandle
{
};
#endif

UdpSocket::UdpSocket()
{
}

UdpSocket::~UdpSocket()
{
}

} // namespace network
} // namespace wz