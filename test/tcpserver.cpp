
#include <iostream>

#include "Server.h"

void receive(void* server,
	void* client,
	const char *data,
	const int &length)
{
	printf("*******receive success********\n.");
	wz::network::TcpServer* tcpServer = (wz::network::TcpServer*)server;
	wz::network::TcpClient* tcpClient = (wz::network::TcpClient*)client;
	printf("clients count: %d\n", tcpServer->getClientsCount());
	printf("client: %p\n",&client);
	printf("data: %s\n",data);
	printf("length: %d\n",length);
	printf("client ip: %s\n", tcpServer->getClientAddress(*tcpClient).data());
	tcpServer->send(*tcpClient, "received. repsonse!");
}

int main(int argc, char** argv)
{
	printf("==================tcp server==================\n");

	wz::network::TcpServer server(12345);
	server.setReceiveCallback(receive);
	if (server.listen())
	{
		printf("listen success.\n");
	}
	else
	{
		printf("listen failed.\n");
	}

	getchar();

	printf("had set maxclients to 15.\n");
	server.setMaxClients(15);

	getchar();

	printf("close the server.\n");
	server.close();
	printf("now maxclients: %d\n",server.getClientsCount());

	getchar();
	printf("quit!");

	return 0;
}