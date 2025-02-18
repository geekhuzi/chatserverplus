#include "chatserver.h"
#include <iostream>
#include <cstdio>
#include <signal.h>
#include <log.h>
#include <watchdog.h>

using namespace std;
ChatServer* server;
clogfile logfile;
cpactive pactive;
// pactive.addinfo(5, "chatser");

// 心跳线程类
class HeartbeatThread {
private:
    bool running_;
    std::thread heartbeatThread_;

public:
    HeartbeatThread() : running_(true) {}

    // 启动心跳更新线程
    void start() {
        heartbeatThread_ = std::thread(&HeartbeatThread::run, this);
    }

    // 停止心跳线程
    void stop() {
        running_ = false;
        if (heartbeatThread_.joinable()) {
            heartbeatThread_.join();
        }
    }

    // 心跳更新线程运行的方法
    void run() {
        while (running_) {
            std::this_thread::sleep_for(std::chrono::seconds(5));  // 每隔5秒更新一次
            updateHeartbeat();  // 调用心跳更新的方法
        }
    }

    // 更新心跳信息
    void updateHeartbeat() {
        pactive.uptatime();  // 这里是更新心跳的逻辑
        std::cout << "Heartbeat updated." << std::endl;
    }
};

HeartbeatThread heartbeat;  // 创建心跳线程对象

void Stop(int sig)
{
    logfile.write("receive signal %d\n",sig);
    server->Stop();
    heartbeat.stop();
    printf("chatserver stoped\n");
    delete server;
    logfile.write("program ends\n");
    exit(0);
}

int main(int argc,char* argv[])
{
    if(argc!=4)
    {
        printf("usage:%s log ip port\n",argv[0]);
        printf("example:%s /home/huzi/project2/log/log.txt 127.0.0.1 6000 \n",argv[0]);
        // printf("example:%s 192.168.190.131 5131\n",argv[0]);
        return -1;
    }
    signal(SIGINT,Stop);signal(SIGTERM,Stop);

    if (logfile.open(argv[1]) == false) {
        printf("logfile.open(%s) failed.\n", argv[1]);
    }

    logfile.write("program begins\n");
    pactive.addpinfo(30,"ChatServer", &logfile);   // 先不考虑日志
    // ChatServer server(argv[1],atoi(argv[2]));
    // server.start();
    // 目前看跟这两行代码跟可能没关系
    server = new ChatServer(argv[2],atoi(argv[3]),3);
    // server = new ChatServer("192.168.190.131",5131);
    // printf("更改时间:%s\n",Timestamp::now().tostring().c_str());
    heartbeat.start();
    server->start();
    return 0;
}