
#include "Socket.h"

#ifdef _WIN32
#include <winsock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

namespace wz
{
namespace network
{

/* tcp socket */
#ifdef _WIN32
struct TcpSocketHandle
{
    WORD socketVersion;
    WSADATA data;
    int wsaStartupResult;

    sockaddr_in serverAddress;
    SOCKET socket;
    int connectStatus;

    int sendTimeOut;    /* unit: ms */
    int receiveTimeOut; /* unit: ms */

    enum CONNECT_STATUS
    {
        DISCONNECT = -1,
        CONNECTED = 1
    };

};
#else
struct TcpSocketHandle
{
    sockaddr_in serverAddress;
    int socket;
    int connectStatus;

    timeval sendTimeOut;    /* unit: {s,us} */
    timeval receiveTimeOut; /* unit: {s,us} */

    enum CONNECT_STATUS
    {
        DISCONNECT = -1,
        CONNECTED = 1
    };
};
#endif

TcpSocket::TcpSocket()
    : handle(NULL),
      receiveBuffer(NULL),
      receiveBufferSize(0),
      receiveThread(NULL),
      receiveThreadCondition(false)
{
    init();
}

TcpSocket::TcpSocket(const std::string &ip, const int &port)
    : handle(NULL),
      receiveBuffer(NULL),
      receiveBufferSize(0),
      receiveThread(NULL),
      receiveThreadCondition(false)
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
#ifdef _WIN32
    if (NULL != handle && 0 == handle->wsaStartupResult)
#else
    if (NULL != handle)
#endif
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
#ifdef _WIN32
    if (NULL != handle && 0 == handle->wsaStartupResult)
#else
    if (NULL != handle)
#endif
    {
        inet_ntop(AF_INET, &(handle->serverAddress.sin_addr), ip, sizeof(ip));
        return ip;
    }
    else
    {
        return "0.0.0.0";
    }
}

void TcpSocket::setPort(const int &port)
{
#ifdef _WIN32
    if (NULL != handle && 0 == handle->wsaStartupResult)
#else
    if (NULL != handle)
#endif
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
#ifdef _WIN32
    if (NULL != handle && 0 == handle->wsaStartupResult)
#else
    if (NULL != handle)
#endif
    {
        return ntohs(handle->serverAddress.sin_port);
    }
    else
    {
        return 0;
    }
}

bool TcpSocket::connect()
{
    if (TcpSocketHandle::CONNECT_STATUS::CONNECTED == handle->connectStatus)
    {
        printf("[TcpSocket::connect()]: connect failed with current socket is connected!\n");
        return false;
    }

#ifdef _WIN32
    handle->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (INVALID_SOCKET == handle->socket)
#else
    handle->socket = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == handle->socket)
#endif
    {
        printf("[TcpSocket::connect()]: connect failed with invalid socket!");
        return false;
    }
    int connectResult = ::connect(handle->socket,
                                  (sockaddr *)&(handle->serverAddress),
                                  sizeof(handle->serverAddress));
#ifdef _WIN32
    if (SOCKET_ERROR == connectResult)
#else
    if (connectResult < 0)
#endif
    {
        printf("[TcpSocket::connect()]: connect failed with %s:%d,"
               "please check if the server is available.",
               getIp().data(),
               getPort());
        handle->connectStatus = TcpSocketHandle::CONNECT_STATUS::DISCONNECT;
        return false;
    }

#ifdef _WIN32
    setsockopt(handle->socket, SOL_SOCKET, SO_SNDTIMEO,
               (char *)&(handle->sendTimeOut), sizeof(int));
    setsockopt(handle->socket, SOL_SOCKET, SO_RCVTIMEO,
               (char *)&(handle->receiveTimeOut), sizeof(int));
#else
    setsockopt(handle->socket, SOL_SOCKET, SO_SNDTIMEO,
               (char *)&(handle->sendTimeOut), sizeof(timeval));
    setsockopt(handle->socket, SOL_SOCKET, SO_RCVTIMEO,
               (char *)&(handle->receiveTimeOut), sizeof(timeval));
#endif

    handle->connectStatus = TcpSocketHandle::CONNECT_STATUS::CONNECTED;

    receiveThreadCondition = true;
    receiveThread = new std::thread(&TcpSocket::receiveThreadRun, this);

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
#ifdef _WIN32
    if (INVALID_SOCKET != handle->socket &&
        TcpSocketHandle::CONNECT_STATUS::DISCONNECT != handle->connectStatus)
#else
    if (-1 != handle->socket &&
        TcpSocketHandle::CONNECT_STATUS::DISCONNECT != handle->connectStatus)
#endif
    {
        if (NULL != receiveThread)
        {
            receiveThreadCondition = false;
            receiveThread->join();
            delete receiveThread;
            receiveThread = NULL;
        }

#ifdef _WIN32
        closesocket(handle->socket);
        handle->socket = INVALID_SOCKET;
#else
        close(handle->socket);
        handle->socket = -1;
#endif
        handle->connectStatus = TcpSocketHandle::CONNECT_STATUS::DISCONNECT;
    }
}

int TcpSocket::send(const char *data, const int &length)
{
    return ::send(handle->socket, data, length, 0);
}

int TcpSocket::send(const std::string &data)
{
    return send(data.data(), data.size());
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

void TcpSocket::setSendTimeout(const unsigned int &timeout)
{
    if (NULL != handle)
    {
#ifdef _WIN32
        handle->sendTimeOut = timeout;
        if (TcpSocketHandle::CONNECT_STATUS::CONNECTED == handle->connectStatus)
        {
            setsockopt(handle->socket, SOL_SOCKET, SO_SNDTIMEO,
                       (char *)&(handle->sendTimeOut), sizeof(int));
        }
#else
        handle->sendTimeOut = {timeout / 1000, (timeout % 1000) * 1000};
        if (TcpSocketHandle::CONNECT_STATUS::CONNECTED == handle->connectStatus)
        {
            setsockopt(handle->socket, SOL_SOCKET, SO_SNDTIMEO,
                       (char *)&(handle->sendTimeOut), sizeof(timeval));
        }
#endif
    }
}

int TcpSocket::getSendTimeout()
{
    if (NULL != handle)
    {
#ifdef _WIN32
        return handle->sendTimeOut;
#else
        return (handle->sendTimeOut.tv_sec) * 1000 +
               (handle->sendTimeOut.tv_usec) / 1000;
#endif
    }
    else
    {
        return 0;
    }
}

void TcpSocket::setReceiveTimeout(const unsigned int &timeout)
{
    if (NULL != handle)
    {
#ifdef _WIN32
        handle->receiveTimeOut = timeout;
        if (TcpSocketHandle::CONNECT_STATUS::CONNECTED == handle->connectStatus)
        {
            setsockopt(handle->socket, SOL_SOCKET, SO_RCVTIMEO,
                       (char *)&(handle->receiveTimeOut), sizeof(int));
        }
#else
        handle->receiveTimeOut = {timeout / 1000, (timeout % 1000) * 1000};
        if (TcpSocketHandle::CONNECT_STATUS::CONNECTED == handle->connectStatus)
        {
            setsockopt(handle->socket, SOL_SOCKET, SO_RCVTIMEO,
                       (char *)&(handle->receiveTimeOut), sizeof(timeval));
        }
#endif
    }
}

int TcpSocket::getReceiveTimeout()
{
    if (NULL != handle)
    {
#ifdef _WIN32
        return handle->receiveTimeOut;
#else
        return (handle->receiveTimeOut.tv_sec) * 1000 +
               (handle->receiveTimeOut.tv_usec) / 1000;
#endif
    }
    else
    {
        return 0;
    }
}

void TcpSocket::init()
{
    handle = new TcpSocketHandle;
    if (NULL != handle)
    {
#ifdef _WIN32
        handle->socketVersion = MAKEWORD(2, 2);
        handle->wsaStartupResult = WSAStartup(handle->socketVersion, &(handle->data));
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
            handle->sendTimeOut = 5 * 1000;
            handle->receiveTimeOut = 5 * 1000;
        }
#else
        memset(&(handle->serverAddress), 0, sizeof(handle->serverAddress));
        handle->serverAddress.sin_family = AF_INET;
        handle->serverAddress.sin_port = htons(0);
        handle->socket = -1;
        handle->connectStatus = TcpSocketHandle::CONNECT_STATUS::DISCONNECT;
        handle->sendTimeOut = {5, 0};
        handle->receiveTimeOut = {5, 0};
#endif
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
    if (NULL != receiveThread)
    {
        receiveThreadCondition = false;
        receiveThread->join();
        delete receiveThread;
        receiveThread = NULL;
    }
#ifdef _WIN32
    if (INVALID_SOCKET != handle->socket)
    {
        closesocket(handle->socket);
        handle->socket = INVALID_SOCKET;
#else
    if (-1 != handle->socket)
    {
        close(handle->socket);
        handle->socket = -1;
#endif
        handle->connectStatus = TcpSocketHandle::CONNECT_STATUS::DISCONNECT;
    }
#ifdef _WIN32
    if (0 != handle->wsaStartupResult)
    {
        WSACleanup();
        handle->wsaStartupResult = 0;
    }
#endif
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

void TcpSocket::receiveThreadRun()
{
    int receiveLength = 0;
    while (receiveThreadCondition)
    {
        receiveLength = 0;
        memset(receiveBuffer, 0, receiveBufferSize);

#ifdef _WIN32
        if (NULL != handle &&
            0 == handle->wsaStartupResult &&
            INVALID_SOCKET != handle->socket &&
            NULL != receiveBuffer &&
            0 != receiveBufferSize)
        {
#else
        if (NULL != handle &&
            -1 != handle->socket &&
            NULL != receiveBuffer &&
            0 != receiveBufferSize)
        {
#endif
            receiveLength = recv(handle->socket, receiveBuffer, receiveBufferSize, 0);
        }
        if (receiveLength > 0)
        {
            receive(receiveBuffer, receiveLength);
        }
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