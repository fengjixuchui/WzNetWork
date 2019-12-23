
#include "Server.h"

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

namespace wz
{
namespace network
{

/* tcp server */
#ifdef _WIN32
struct TcpHandle
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
struct TcpServerHandle
{
};
#endif

TcpServer::TcpServer()
    : handle(NULL),
      receiveBuffer(NULL),
      receiveBufferSize(0),
      receiveThread(NULL),
      receiveThreadCondition(false)
{
    init();
}

TcpServer::TcpServer(const int &port)
    : handle(NULL),
      receiveBuffer(NULL),
      receiveBufferSize(0),
      receiveThread(NULL),
      receiveThreadCondition(false)
{
    init();
}

TcpServer::~TcpServer()
{
    release();
}

void TcpServer::setPort(const int &port)
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
            printf("[TcpServer::setPort(const int &port)]: "
                   "set port failed by incorrect port,please check.\n");
        }
    }
}

int TcpServer::getPort()
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

bool TcpServer::listen()
{
    if (NULL != handle && 0 == handle->wsaStartupResult)
    {
        if (SOCKET_ERROR == bind(handle->socket,
                                 (LPSOCKADDR) & (handle->serverAddress),
                                 sizeof(handle->serverAddress)))
        {
            printf("[TcpServer::listen()]: listen failed with bind error.\n");
            return false;
        }

        if (SOCKET_ERROR == ::listen(handle->socket, 5))
        {
            printf("[TcpServer::listen()]: listen failed with ::listen failed.\n");
            return false;
        }
    }

    receiveThreadCondition = true;
    receiveThread = new std::thread(&TcpServer::receiveThreadRun, this);

    return true;
}

void TcpServer::close()
{
}

void TcpServer::resizeReceiveBuffer(const int &size)
{
    if (receiveBufferSize > 0)
    {
        delete[] receiveBuffer;
        receiveBuffer = NULL;

        receiveBufferSize = size;
        receiveBuffer = new char[receiveBufferSize];
    }
}

void TcpServer::init()
{
    handle = new TcpHandle;
    if (NULL != handle)
    {
        handle->socketVersion = MAKEWORD(2, 2);
        handle->wsaStartupResult = WSAStartup(handle->socketVersion, &(handle->data));
        handle->socket = INVALID_SOCKET;
        if (0 != handle->wsaStartupResult)
        {
            printf("[TcpServer::init()]: init failed with socket version. Try again , "
                   "debug source or contact author!\n");
        }
        else
        {
            handle->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (INVALID_SOCKET == handle->socket)
            {
                printf("[TcpServer::init()]: init failed with invalid socket. Try again , "
                       "debug source or contact author !");
            }

            handle->serverAddress.sin_family = AF_INET;
            handle->serverAddress.sin_port = htons(0);
            handle->serverAddress.sin_addr.S_un.S_addr = INADDR_ANY;
        }
    }
    else
    {
        printf("[TcpServer::init()]: handle is NULL,new TcpServerHandle failed.\n");
    }

    receiveBufferSize = 4096;
    receiveBuffer = new char[receiveBufferSize];
}

void TcpServer::release()
{
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

void TcpServer::receiveThreadRun()
{
    TcpHandle client;
    int clientAddressLength = sizeof(client.serverAddress);

    while (receiveThreadCondition)
    {
        client.socket = accept(handle->socket,
                        (SOCKADDR *)&client.serverAddress, 
                        &clientAddressLength);

        if(INVALID_SOCKET == client.socket)
        {
            printf("[TcpServer::receiveThreadRun()]: accept failed with "
            "invalid client.\n");
            continue;
        }

    }
}

/* udp server */
#ifdef _WIN32
struct UdpHandle
{
};
#else
struct UdpServerHandle
{
};
#endif

UdpServer::UdpServer()
{
}

UdpServer::~UdpServer()
{
}

} // namespace network
} // namespace wz