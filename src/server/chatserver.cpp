#include "chatserver.h"
#include "json.hpp"
#include "chatservice.h"

#include <functional>
using namespace std;
using namespace placeholders;
using json = nlohmann::json;

ChatServer::ChatServer(const std::string &ip, const uint16_t port,int subthreadnum,int workthreadnum)
            :tcpserver_(ip, port,subthreadnum),threadpool_(workthreadnum,"WORKS")
{
    tcpserver_.setnewconnectioncb(std::bind(&ChatServer::HandleNewConnection,this,_1));
    tcpserver_.setonmessagecb(std::bind(&ChatServer::HandleMessage,this,_1,_2));
    // tcpserver_.settimeoutcb(std::bind(&ChatServer::HandleTimeout,this,_1));
    tcpserver_.setcloseconnectioncb(std::bind(&ChatServer::HandleClose,this,std::placeholders::_1));
    // tcpserver_.seterrorconnectioncb(std::bind(&ChatServer::HandleError,this,std::placeholders::_1));
}
ChatServer::~ChatServer()
{

}

void ChatServer::start()
{
    tcpserver_.start();
}

void ChatServer::Stop()
{
    ChatService::instance()->reset();
    threadpool_.stop();
    logfile.write("workthread stoped\n");
    tcpserver_.stop();
}

void ChatServer::HandleNewConnection(spConnection conn)
{
    logfile.write("%s new connection(fd=%d,ip=%s,port=%d) ok.\n",Timestamp::now().tostring().c_str(),conn->fd(),conn->ip().c_str(),conn->port());
    //printf("%s new connection(fd=%d,ip=%s,port=%d) ok.\n",Timestamp::now().tostring().c_str(),conn->fd(),conn->ip().c_str(),conn->port());
}

// void ChatServer::HandleTimeout(EventLoop *loop)
// {
//     std::cout << "Chatserver timeout" << Timestamp::now().tostring() << std::endl;
// }

void ChatServer::HandleClose(spConnection conn)
{
    //std::cout << "EchoServer conn closed" << std::endl;
    logfile.write("%s connection closed(fd=%d,ip=%s,port=%d) ok.\n",Timestamp::now().tostring().c_str(),conn->fd(),conn->ip().c_str(),conn->port());
    ChatService::instance()->clientCloseException(conn);
}

// void ChatServer::HandleError(spConnection conn)
// {
//     //std::cout << "EchoServer conn error" << std::endl;
//     printf("%s connection error(fd=%d,ip=%s,port=%d) ok.\n",Timestamp::now().tostring().c_str(),conn->fd(),conn->ip().c_str(),conn->port());
// }

void ChatServer::HandleMessage(spConnection conn, std::string &message)
{
    if (threadpool_.size() == 0)
    {
        OnMessage(conn,message);
    }
    else
    {
        // printf("EchoServer::HandleMessage thread is %d\n",syscall(SYS_gettid));
        threadpool_.addtask(std::bind(&ChatServer::OnMessage, this, conn, message));
    }
}

void ChatServer::OnMessage(spConnection conn, std::string &message)
{
    json js = json::parse(message);    
    auto msghandler = ChatService::instance()->GetHandler(js["msgid"].get<int>());
    msghandler(conn,js);
    pactive.uptatime();
}