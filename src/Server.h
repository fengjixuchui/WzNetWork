
#ifndef __SERVER_H__
#define __SERVER_H__

#include <iostream>
#include <thread>

namespace wz
{
namespace network
{

/* tcp server */
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

    void resizeReceiveBuffer(const int& size);

    void setMaxClients(const int& maxClients);
    int getMaxClients();

    virtual void receive(const TcpClient& client,
                        const char* data,
                        const int& length) = 0;

private:
    void init();
    void release();
    void receiveThreadRun();
    void clientsReceiveThreadRun(TcpClient* client);

private:
    TcpHandle *handle;
    char* receiveBuffer;
    int receiveBufferSize;

    std::thread* receiveThread;
    bool receiveThreadCondition;

    std::vector<TcpClient*> clients;
    int maxClients; /* default: 10 */
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
