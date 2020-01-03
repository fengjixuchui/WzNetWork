
#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <iostream>
#include <thread>

namespace wz
{
namespace network
{

using ReceiveCallback = void (*)(const char *data,
                                 const int &length);

/* tcp socket */
struct TcpSocketHandle;
class TcpSocket
{
public:
    TcpSocket();
    TcpSocket(const std::string &ip, const int &port);
    virtual ~TcpSocket();

    void setIp(const std::string &ip);
    std::string getIp();
    void setPort(const int &port);
    int getPort();

    bool connect();
    bool isConnected();
    void disconnect();

    int send(const char *data, const int &length);
    int send(const std::string &data);
    void resizeReceiveBuffer(const int &size);

    virtual void receive(const char *data, const int &length);
    void setReceiveCallback(ReceiveCallback receiveCallback);

private:
    void init();
    void release();
    void receiveThreadRun();

private:
    TcpSocketHandle *handle;
    char *receiveBuffer;
    int receiveBufferSize;
    std::thread *receiveThread;
    ReceiveCallback receiveCallback;
};

/* udp socket */
struct UdpSocketHandle;
class UdpSocket
{
public:
    UdpSocket();
    UdpSocket(const std::string &ip, const int &port);
    virtual ~UdpSocket();

    void setIp(const std::string &ip);
    std::string getIp();
    void setPort(const int &port);
    int getPort();

    /**
     * @brief open udp socket with mode
     * @param mode 
     *        0: general
     *        1: multicast
     */
    bool open(int mode=0);
    void close();

    int send(const char *data, const int &length);
    int send(const std::string &data);
    void resizeReceiveBuffer(const int &size);

    virtual void receive(const char *data, const int &length);
    void setReceiveCallback(ReceiveCallback receiveCallback);

private:
    void init();
    void release();
    void receiveThreadRun();

private:
    UdpSocketHandle *handle;
    char *receiveBuffer;
    int receiveBufferSize;
    bool receivable;
    std::thread *receiveThread;
    ReceiveCallback receiveCallback;
};

} // namespace network
} // namespace wz

#endif
