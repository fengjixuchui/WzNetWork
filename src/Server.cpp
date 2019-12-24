
#include "Server.h"

#ifdef _WIN32
#include <winsock2.h>
#include <WS2tcpip.h>
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

    TcpClient()
        : clientThread(NULL)
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
      listenStatus(false),
      receiveBuffer(NULL),
      receiveBufferSize(0),
      receiveThread(NULL),
      maxClients(10),
      receiveCallback(NULL)
{
    init();
}

TcpServer::TcpServer(const int &port)
    : handle(NULL),
      listenStatus(false),
      receiveBuffer(NULL),
      receiveBufferSize(0),
      receiveThread(NULL),
      maxClients(10),
      receiveCallback(NULL)
{
    init();
	
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
            printf("[TcpServer::TcpServer(const int &port)(const int &port)]: "
                   "init port failed with incorrect port,please check.\n");
        }
    }
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
        handle->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (INVALID_SOCKET == handle->socket)
        {
            printf("[TcpServer::init()]: init failed with invalid socket. Try again , "
                   "debug source or contact author !\n");
            return false;
        }

        if (SOCKET_ERROR == bind(handle->socket,
                                 (LPSOCKADDR) & (handle->handleAddress),
                                 sizeof(handle->handleAddress)))
        {
            printf("[TcpServer::listen()]: listen failed with bind error , "
                   "may be server is closed.\n");
            return false;
        }

        if (SOCKET_ERROR == ::listen(handle->socket, 5))
        {
            printf("[TcpServer::listen()]: listen failed with ::listen failed.\n");
            return false;
        }
    }

    receiveThread = new std::thread(&TcpServer::receiveThreadRun, this);
    if (NULL == receiveThread)
    {
        printf("[TcpServer::listen()]: listen failed with receiveThread is NULL.\n");
        return false;
    }

    listenStatus = true;
    return true;
}

void TcpServer::close()
{
    if (clients.size() > 0)
    {
        TcpClient *frontClient = NULL;
        while (clients.size() > 0)
        {
            frontClient = clients.front();
            closesocket(frontClient->clientHandle.socket);
            frontClient->clientHandle.socket = INVALID_SOCKET;
            frontClient->clientThread->join();
            delete frontClient->clientThread;
            frontClient->clientThread = NULL;
            delete frontClient;
            frontClient = NULL;
            clients.pop();
        }
    }

    if (INVALID_SOCKET != handle->socket)
    {
        closesocket(handle->socket);
        handle->socket = INVALID_SOCKET;
        receiveThread->join();
        delete receiveThread;
        receiveThread = NULL;
        listenStatus = false;
    }
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

bool TcpServer::getListenStatus()
{
    return listenStatus;
}

int TcpServer::getClientsCount()
{
    return clients.size();
}

int TcpServer::send(const TcpClient &client,
                    const char *data,
                    const int &length)
{
    return ::send(client.clientHandle.socket, data, length, 0);
}

int TcpServer::send(const TcpClient &client,
                    const std::string &data)
{
    return send(client, data.data(), data.size());
}

std::string TcpServer::getClientAddress(const TcpClient &client)
{
    char ip[16] = {'0'};
    inet_ntop(AF_INET, &(client.clientHandle.handleAddress.sin_addr), ip, sizeof(ip));
    return ip;
}

void TcpServer::closeClient(const TcpClient &client)
{
    int count = clients.size();
    TcpClient *frontClient = NULL;
    for (int i = 0; i < count; i++)
    {
        frontClient = clients.front();
        if (frontClient != &client)
        {
            clients.push(frontClient);
        }
        else
        {
            closesocket(frontClient->clientHandle.socket);
            frontClient->clientHandle.socket = INVALID_SOCKET;
            frontClient->clientThread->join();
            delete frontClient->clientThread;
            frontClient->clientThread = NULL;
            delete frontClient;
        }
        frontClient = NULL;
        clients.pop();
    }
}

void TcpServer::receive(const TcpClient &client,
                        const char *data,
                        const int &length)
{
    if (NULL != receiveCallback)
    {
        receiveCallback((void *)this,
                        (void *)&client,
                        data,
                        length);
    }
}

void TcpServer::setReceiveCallback(ReceiveCallback receiveCallback)
{
    this->receiveCallback = receiveCallback;
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
    close();

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

void TcpServer::receiveThreadRun()
{
    TcpClient *client = NULL;
    int clientAddressLength = sizeof(sockaddr_in);

    while (INVALID_SOCKET != handle->socket)
    {
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

        if (maxClients <= clients.size())
        {
            printf("[TcpServer::receiveThreadRun()]: accept failed with "
                    "clients enough , you need resize maxClients.\n");
            closesocket(client->clientHandle.socket);
            client->clientHandle.socket = INVALID_SOCKET;
            delete client;
            client = NULL;
            continue;
        }

        client->clientThread = new std::thread(&TcpServer::clientsReceiveThreadRun,
                                               this,
                                               client);
        if (NULL == client->clientThread)
        {
            printf("[TcpServer::receiveThreadRun()]: accept failed with client->clientThread is NULL.\n");
            delete client;
            client = NULL;
            continue;
        }

        clients.push(client);
    }
}

void TcpServer::clientsReceiveThreadRun(TcpClient *client)
{
    int receiveLength = 0;

    memset(receiveBuffer, 0, receiveBufferSize);
    receiveLength = recv(client->clientHandle.socket,
                         receiveBuffer,
                         receiveBufferSize,
                         0);
    while (receiveLength > 0)
    {
        receive(*client, receiveBuffer, receiveLength);
        memset(receiveBuffer, 0, receiveBufferSize);
        receiveLength = recv(client->clientHandle.socket,
                             receiveBuffer,
                             receiveBufferSize,
                             0);
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