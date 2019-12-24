
#include <iostream>

#include "Socket.h"

void receive(const char *data,
	const int &length)
{
	printf("data: %s\n",data);
	printf("length: %d\n", length);
}

int main(int argc, char** argv)
{
	printf("==================tcp client==================\n");

	wz::network::TcpSocket socket("127.0.0.1", 12345);
	socket.setReceiveCallback(receive);
	if (socket.connect())
	{
		printf("connect success.\n");
		socket.send("hellworld");
		socket.send("world");
	}
	else
	{
		printf("connect failed.\n");
	}

	getchar();
	
	printf("disconnect!\n");
	socket.disconnect();

	getchar();

	printf("connect again.\n");
	if (socket.connect())
	{
		printf("connect success.\n");
		socket.send("hellworld again");
		socket.send("world again");
	}
	else
	{
		printf("connect failed.\n");
	}

	getchar();
	printf("quit!\n");
	socket.disconnect();

	return 0;
}