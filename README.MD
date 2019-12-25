# WzNetWork

**跨平台网络库，包括：TCP、UDP**

### test下有使用案例
* tcpclient
```shell
cd test
g++ tcpclient.cpp ../src/Socket.cpp -I../src -lpthread -o tcpclient
```
* tcpserver
```shell
cd test
g++ tcpserver.cpp ../src/Server.cpp -I../src -lpthread -o tcpserver
```