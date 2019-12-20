
#ifndef __SERVER_H__
#define __SERVER_H__

#include <iostream>

namespace wz
{
namespace network
{

/* tcp server */
struct TcpServerHandle;
class TcpServer
{
public:
    TcpServer();
    virtual ~TcpServer();

private:
    void init();
    void release();

private:
    TcpServerHandle *handle;
};

/* udp server */
struct UdpServerHandle;
class UdpServer
{
public:
    UdpServer();
    virtual ~UdpServer();

private:
    void init();
    void release();

private:
    UdpServerHandle *handle;
};

} // namespace network
} // namespace wz

#endif
