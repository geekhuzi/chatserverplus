#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <muduoC11/TcpServer.h>
#include <muduoC11/EventLoop.h>
#include "log.h"
#include "watchdog.h"

class ChatServer
{
private:
    TcpServer tcpserver_;
    ThreadPool threadpool_;
public:
    ChatServer(const std::string &ip,const uint16_t port,int subthreadnum = 3,int workthreadnum = 5);
    ~ChatServer();

    void HandleNewConnection(spConnection conn);

    void HandleMessage(spConnection conn,std::string& message);

    void OnMessage(spConnection conn,std::string& message);      //处理客户端的请求报文，用于添加给线程池

    void HandleClose(spConnection conn);

    // void HandleError(spConnection conn);

    // void HandleTimeout(EventLoop *loop);

    void start();
    void Stop();

};

#endif