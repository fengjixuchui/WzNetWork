
#include "Server.h"

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")
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
struct TcpServerHandle
{
    
};
#else
struct TcpServerHandle
{

};
#endif

TcpServer::TcpServer()
{

}

TcpServer::~TcpServer()
{

}

/* udp server */
#ifdef _WIN32
struct UdpServerHandle
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

}
} // namespace wz