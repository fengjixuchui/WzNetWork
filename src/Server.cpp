
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

    sockaddr_in handleAddress;
    SOCKET socket;
    int connectStatus;

    enum CONNECT_STATUS
    {
        DISCONNECT = -1,
        CONNECTED = 1
    };
};
struct TcpClient
{
    TcpHandle clientHandle;
    std::thread *clientThread;
    bool clientThreadCondition;

    TcpClient()
        : clientThread(NULL),
          clientThreadCondition(false)
    {
    }
};
#else
struct TcpServerHandle
{
};
#endif

TcpServer::TcpServer()
    : handle(NULL),
      receiveBuffer(NULL),
      receiveThread(NULL),
      receiveThreadCondition(false),
      receiveBufferSize(0),
      maxClients(10)
{
    init();
}

TcpServer::TcpServer(const int &port)
    : handle(NULL),
      receiveBuffer(NULL),
      receiveThread(NULL),
      receiveThreadCondition(false),
      receiveBufferSize(0),
      maxClients(10)
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
            handle->handleAddress.sin_port = htons(port);
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
        return ntohs(handle->handleAddress.sin_port);
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
                                 (LPSOCKADDR) & (handle->handleAddress),
                                 sizeof(handle->handleAddress)))
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
    if(NULL == receiveThread)
    {
        printf("[TcpServer::listen()]: listen failed with receiveThread is NULL.\n");
        receiveThreadCondition = false;
        return false;
    }
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

void TcpServer::setMaxClients(const int &maxClients)
{
    this->maxClients = maxClients;
}

int TcpServer::getMaxClients()
{
    return maxClients;
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
                       "debug source or contact author !\n");
            }

            handle->handleAddress.sin_family = AF_INET;
            handle->handleAddress.sin_port = htons(0);
            handle->handleAddress.sin_addr.S_un.S_addr = INADDR_ANY;
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
    TcpClient *client = NULL;
    int clientAddressLength = sizeof(sockaddr_in);
    while (receiveThreadCondition)
    {
        if (maxClients <= clients.size())
        {
            printf("[TcpServer::receiveThreadRun()]: accept failed with "
                   "clients enough , you need resize maxClients.\n");
            continue;
        }

        client = new TcpClient;
        if (NULL == client)
        {
            printf("[TcpServer::receiveThreadRun()]: accept failed with client is NULL.\n");
            continue;
        }
        client->clientHandle.socket = accept(handle->socket,
                                             (SOCKADDR *)&(client->clientHandle.handleAddress),
                                             &clientAddressLength);
        if (INVALID_SOCKET == client->clientHandle.socket)
        {
            printf("[TcpServer::receiveThreadRun()]: accept failed with invalid socket.\n");
            delete client;
            client = NULL;
            continue;
        }

        client->clientThreadCondition = true;
        client->clientThread = new std::thread(&TcpServer::clientsReceiveThreadRun,
                                               this,
                                               client);
        if (NULL == client->clientThread)
        {
            printf("[TcpServer::receiveThreadRun()]: accept failed with client->clientThread is NULL.\n");
            delete client;
            client = NULL;
            client->clientThreadCondition = false;
            continue;
        }

        clients.push_back(client);
    }
}

void TcpServer::clientsReceiveThreadRun(TcpClient *client)
{
    int receiveLength = 0;
    while (client->clientThreadCondition)
    {
        receiveLength = 0;
        memset(receiveBuffer,0,receiveBufferSize);

        receiveLength = recv(client->clientHandle.socket,
                             receiveBuffer,
                             receiveBufferSize,
                             0);
        if (receiveLength > 0)
        {
            receive(*client, receiveBuffer, receiveBufferSize);
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