
#include "Socket.h"

#ifdef _WIN32
#include <winsock2.h>  
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

namespace wz
{
namespace network
{

/* tcp socket */
#ifdef _WIN32
struct TcpSocketHandle
{
    
};
#else
struct TcpSocketHandle
{

};
#endif

TcpSocket::TcpSocket()
{
    init();
}

TcpSocket::~TcpSocket()
{
    release();
}

/* udp socket */
#ifdef _WIN32
struct UdpSocketHandle
{

};
#else
struct UdpSocketHandle
{

};
#endif

UdpSocket::UdpSocket()
{

}

UdpSocket::~UdpSocket()
{

}
    
}
} // namespace wz