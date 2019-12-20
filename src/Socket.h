
#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <iostream>

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
    virtual ~TcpSocket();

private:
    void init();
    void release();

private:
    TcpSocketHandle *handle;
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
