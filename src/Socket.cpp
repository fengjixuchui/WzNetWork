
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
      receiveCallback(NULL)
{
    init();
}

TcpSocket::TcpSocket(const std::string &ip, const int &port)
    : handle(NULL),
      receiveBuffer(NULL),
      receiveBufferSize(0),
      receiveThread(NULL),
      receiveCallback(NULL)
{
    init();

    setIp(ip);
    setPort(port);
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
    if (NULL != handle && 0 == handle->wsaStartupResult)
    {
        handle->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (INVALID_SOCKET == handle->socket)
#else
    if (NULL != handle)
    {
        handle->socket = socket(AF_INET, SOCK_STREAM, 0);
        if (-1 == handle->socket)
#endif
        {
            printf("[TcpSocket::connect()]: connect failed with invalid socket!\n");
            return false;
        }
    }
    else
    {
        printf("[TcpSocket::connect()]: connect failed with handle is NULL"
               " or invalid handle!\n");
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
               "please check if the server is available.\n",
               getIp().data(),
               getPort());
        handle->connectStatus = TcpSocketHandle::CONNECT_STATUS::DISCONNECT;
        return false;
    }

    handle->connectStatus = TcpSocketHandle::CONNECT_STATUS::CONNECTED;

    receiveThread = new std::thread(&TcpSocket::receiveThreadRun, this);
    if (NULL == receiveThread)
    {
        printf("[TcpSocket::connect()]: connect failed with receiveThread is NULL.\n");
        return false;
    }

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
#ifdef _WIN32
        closesocket(handle->socket);
        handle->socket = INVALID_SOCKET;
#else
        shutdown(handle->socket, SHUT_RDWR);
        close(handle->socket);
        handle->socket = -1;
#endif
        handle->connectStatus = TcpSocketHandle::CONNECT_STATUS::DISCONNECT;

        if (NULL != receiveThread)
        {
            receiveThread->join();
            delete receiveThread;
            receiveThread = NULL;
        }
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

void TcpSocket::receive(const char *data, const int &length)
{
    if (NULL != receiveCallback)
    {
        receiveCallback(data, length);
    }
}

void TcpSocket::setReceiveCallback(ReceiveCallback receiveCallback)
{
    this->receiveCallback = receiveCallback;
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
        }
#else
        memset(&(handle->serverAddress), 0, sizeof(handle->serverAddress));
        handle->serverAddress.sin_family = AF_INET;
        handle->serverAddress.sin_port = htons(0);
        handle->socket = -1;
        handle->connectStatus = TcpSocketHandle::CONNECT_STATUS::DISCONNECT;
#endif
    }
    else
    {
        printf("[TcpSocket::init()]: init failed with handle is NULL."
               "Try again , debug source or contact author!\n");
    }

    receiveBufferSize = 4096;
    receiveBuffer = new char[receiveBufferSize];
    if (NULL == receiveBuffer)
    {
        printf("[TcpSocket::init()]: init failed with receiveBuffer is NULL."
               "Try again , debug source or contact author!\n");
    }
}

void TcpSocket::release()
{
#ifdef _WIN32
    if (INVALID_SOCKET != handle->socket)
    {
        closesocket(handle->socket);
        handle->socket = INVALID_SOCKET;
#else
    if (-1 != handle->socket)
    {
        shutdown(handle->socket, SHUT_RDWR);
        close(handle->socket);
        handle->socket = -1;
#endif
        handle->connectStatus = TcpSocketHandle::CONNECT_STATUS::DISCONNECT;

        if (NULL != receiveThread)
        {
            receiveThread->join();
            delete receiveThread;
            receiveThread = NULL;
        }
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
        while (receiveLength > 0)
        {
            receive(receiveBuffer, receiveLength);
            memset(receiveBuffer, 0, receiveBufferSize);
            receiveLength = recv(handle->socket, receiveBuffer, receiveBufferSize, 0);
        }
    }
}

/* udp socket */
#ifdef _WIN32
struct UdpSocketHandle
{
    WORD socketVersion;
    WSADATA data;
    int wsaStartupResult;

    sockaddr_in serverAddress;
    SOCKET socket;
};
#else
struct UdpSocketHandle
{
    sockaddr_in serverAddress;
    int socket;
};
#endif

UdpSocket::UdpSocket()
    : handle(NULL),
      receiveBuffer(NULL),
      receiveBufferSize(0),
      receivable(false),
      receiveThread(NULL),
      receiveCallback(NULL)
{
    init();
}

UdpSocket::UdpSocket(const std::string &ip, const int &port)
    : handle(NULL),
      receiveBuffer(NULL),
      receiveBufferSize(0),
      receivable(false),
      receiveThread(NULL),
      receiveCallback(NULL)
{
    init();
    setIp(ip);
    setPort(port);
}

UdpSocket::~UdpSocket()
{
    release();
}

void UdpSocket::setIp(const std::string &ip)
{
#ifdef _WIN32
    if (NULL != handle && 0 == handle->wsaStartupResult)
#else
    if (NULL != handle)
#endif
    {
        switch (inet_pton(AF_INET, ip.data(), &(handle->serverAddress.sin_addr.S_un.S_addr)))
        {
        case 0: /* invalid ip */
            printf("[UdpSocket::setIp(const std::string &ip)]: "
                   "set ip failed with invalid ip,please check.\n");
            break;
        case -1:
            printf("[UdpSocket::setIp(const std::string &ip)]: "
                   "set ip failed with function 'inet_pton' inner error,please debug.\n");
            break;
        case 1: /* set ip success */
        default:
            break;
        }
    }
}

std::string UdpSocket::getIp()
{
    char ip[16] = {'0'};
#ifdef _WIN32
    if (NULL != handle && 0 == handle->wsaStartupResult)
#else
    if (NULL != handle)
#endif
    {
        inet_ntop(AF_INET, &(handle->serverAddress.sin_addr.S_un.S_addr), ip, sizeof(ip));
        return ip;
    }
    else
    {
        return "0.0.0.0";
    }
}

void UdpSocket::setPort(const int &port)
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
            printf("[UdpSocket::setPort(const int &port)]: "
                   "set port failed by incorrect port,please check.\n");
        }
    }
}

int UdpSocket::getPort()
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

bool UdpSocket::open(int mode)
{
#ifdef _WIN32
    if (NULL == handle || 0 != handle->wsaStartupResult)
#else
    if (NULL == handle)
#endif
    {
        printf("[UdpSocket::open(int mode)]: open failed with init() failed , "
               "please check init().\n");
        return false;
    }

#ifdef _WIN32
    if (INVALID_SOCKET != handle->socket)
#else
    if (-1 != handle->socket)
#endif
    {
        printf("[UdpSocket::open(int mode)]: open failed with an available socket , "
               "maybe you need to invoke close() before.\n");
        return false;
    }

    if (NULL != receiveThread)
    {
        printf("[UdpSocket::open(int mode)]: open failed with receiveThread is not NULL , "
               "maybe you need to invoke close() before.\n");
        return false;
    }

    switch (mode)
    {
    case 0: /* general */
        handle->socket = socket(PF_INET, SOCK_DGRAM, 0);
        if (INVALID_SOCKET == handle->socket)
        {
            printf("[UdpSocket::open(int mode)]: open failed with invalid socket."
                   "Try again , debug source or contact author!\n");
            return false;
        }
        break;
    case 1: /* multicast */
        break;
    default:
        break;
    }

    receivable = true;
    receiveThread = new std::thread(&UdpSocket::receiveThreadRun, this);
    if (NULL == receiveThread)
    {
        printf("[UdpSocket::open()]: open failed with receiveThread is NULL.\n");
        return false;
    }

    return true;
}

void UdpSocket::close()
{
    if (NULL != receiveThread)
    {
        if(INVALID_SOCKET != handle->socket)
        {
            closesocket(handle->socket);
            handle->socket = INVALID_SOCKET;
        }
        receivable = false;

        receiveThread->join();
        delete receiveThread;
        receiveThread = NULL;
    }
}

int UdpSocket::send(const char *data, const int &length)
{
    return sendto(handle->socket,
                  data,
                  length,
                  0,
                  (sockaddr *)&handle->serverAddress,
                  sizeof(handle->serverAddress));
}

int UdpSocket::send(const std::string &data)
{
    return send(data.data(), data.size());
}

void UdpSocket::resizeReceiveBuffer(const int &size)
{
    if (receiveBufferSize > 0)
    {
        delete[] receiveBuffer;
        receiveBuffer = NULL;

        receiveBufferSize = size;
        receiveBuffer = new char[receiveBufferSize];
    }
}

void UdpSocket::receive(const char *data, const int &length)
{
    if (NULL != receiveCallback)
    {
        receiveCallback(data, length);
    }
}

void UdpSocket::setReceiveCallback(ReceiveCallback receiveCallback)
{
    this->receiveCallback = receiveCallback;
}

void UdpSocket::init()
{
    handle = new UdpSocketHandle;
    if (NULL != handle)
    {
        handle->socketVersion = MAKEWORD(2, 2);
        handle->wsaStartupResult = WSAStartup(handle->socketVersion, &(handle->data));
        if (0 != handle->wsaStartupResult)
        {
            printf("[UdpSocket::init()]: init failed with socket version."
                   "Try again , debug source or contact author!\n");
        }
        else
        {
            handle->serverAddress.sin_family = AF_INET;
            switch (inet_pton(AF_INET, "0.0.0.0", &(handle->serverAddress.sin_addr.S_un.S_addr)))
            {
            case -1:
                printf("[UdpSocket::init()]: "
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
        }
    }
    else
    {
        printf("[UdpSocket::init()]: init failed with handle is NULL."
               "Try again , debug source or contact author!\n");
    }

    receiveBufferSize = 4096;
    receiveBuffer = new char[receiveBufferSize];
    if (NULL == receiveBuffer)
    {
        printf("[UdpSocket::init()]: init failed with receiveBuffer is NULL."
               "Try again , debug source or contact author!\n");
    }
}

void UdpSocket::release()
{
    close();

    if(NULL != receiveBuffer)
    {
        delete[] receiveBuffer;
        receiveBuffer = NULL;
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
}

void UdpSocket::receiveThreadRun()
{
    int receiveLength = 0;
    int serverAddressLength = sizeof(handle->serverAddress);
    memset(receiveBuffer, 0, receiveBufferSize);

    while (receivable)
    {
        receiveLength = recvfrom(handle->socket,
                                 receiveBuffer,
                                 receiveBufferSize,
                                 0,
                                 (sockaddr *)&handle->serverAddress,
                                 &serverAddressLength);
        if(receiveLength > 0)
        {
            receive(receiveBuffer, receiveLength);
            memset(receiveBuffer, 0, receiveBufferSize);
        }
    }
}

} // namespace network
} // namespace wz