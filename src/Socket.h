
#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <iostream>
#include <thread>

namespace wz
{
namespace network
{

/* tcp socket */
struct TcpSocketHandle;
class TcpSocket
{
public:
    TcpSocket();
    TcpSocket(const std::string& ip,const int& port);
    virtual ~TcpSocket();

    void setIp(const std::string& ip);
    std::string getIp();
    void setPort(const int& port);
    int getPort();

    bool connect();
    bool isConnected();
    void disconnect();

    int send(const char* data,const int& length);
    int send(const std::string& data);
    void resizeReceiveBuffer(const int& size);

    virtual void receive(const char* data,const int& length) = 0;

private:
    void init();
    void release();
    void receiveThreadRun();

private:
    TcpSocketHandle *handle;
    char* receiveBuffer;
    int receiveBufferSize;
    std::thread* receiveThread;
};

/* udp socket */
struct UdpSocketHandle;
class UdpSocket
{
public:
    UdpSocket();
    virtual ~UdpSocket();

private:
    void init();
    void release();

private:
    UdpSocketHandle *handle;
};

} // namespace network
} // namespace wz

#endif
