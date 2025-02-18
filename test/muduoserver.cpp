#include "TcpServer.h"
#include <iostream>
#include <functional>
#include <string>
using namespace std;

class ChatServer{
public:
    ChatServer(string ip, uint16_t port, int threadnum = 3) : tcpserver(ip, port, threadnum) {
        tcpserver.setnewconnectioncb(bind(&ChatServer::newconnection, this, placeholders::_1));
        tcpserver.setonmessagecb(bind(&ChatServer::onmessage, this, placeholders::_1, placeholders::_2));
        tcpserver.setcloseconnectioncb(bind(&ChatServer::closeconnection,this,placeholders::_1));
    }

    void start() {
        tcpserver.start();
    }

private:
    void newconnection(spConnection conn)    // 处理新客户端连接请求，在Acceptor类中回调此函数。
    {
        cout << "port:" << conn->port() << "ip:" << conn->ip() << endl;
    }

    void onmessage(spConnection conn,std::string &message) {
        cout << "recv:" << message << endl;
        conn->send(message.c_str(), message.size());
    }

    void closeconnection(spConnection) {
        cout << "closeconnection" << endl;
    }

    TcpServer tcpserver;
};

int main() {
    ChatServer server("192.168.190.131", 5131);
    server.start();
    return 0;
}