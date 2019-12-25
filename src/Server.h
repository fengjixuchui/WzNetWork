
#ifndef __SERVER_H__
#define __SERVER_H__

#include <iostream>
#include <thread>
#include <queue>

namespace wz
{
namespace network
{

/* tcp server */
using ReceiveCallback = void (*)(void* server,
                                 void* client,
                                 const char *data,
                                 const int &length);
struct TcpHandle;
struct TcpClient;
class TcpServer
{
public:
    TcpServer();
    TcpServer(const int &port);
    virtual ~TcpServer();

    void setPort(const int &port);
    int getPort();

    bool listen();
    void close();

    void resizeReceiveBuffer(const int &size);

    void setMaxClients(const int &maxClients);
    int getMaxClients();
    int getClientsCount();

    bool getListenStatus();

    int send(const TcpClient &client,
             const char *data,
             const int &length);
    int send(const TcpClient &client,
             const std::string &data);

    std::string getClientAddress(const TcpClient &client);

    void closeClient(const TcpClient &client);

    virtual void receive(const TcpClient &client,
                         const char *data,
                         const int &length);
    void setReceiveCallback(ReceiveCallback ReceiveCallback);

private:
    void init();
    void release();
    void receiveThreadRun();
    void clientsReceiveThreadRun(TcpClient *client);

private:
    TcpHandle *handle;
    bool listenStatus;
    char *receiveBuffer;
    int receiveBufferSize;
    std::thread *receiveThread;
    std::queue<TcpClient *> clients;
    int maxClients; /* default: 10 */
    ReceiveCallback receiveCallback;
};

/* udp server */
struct UdpHandle;
class UdpServer
{
public:
    UdpServer();
    virtual ~UdpServer();

private:
    void init();
    void release();

private:
    UdpHandle *handle;
};

} // namespace network
} // namespace wz

#endif
