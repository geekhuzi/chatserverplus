# 启动集群聊天室的所有服务程序

# 启动守护模块
./procctl 10 ./checkproc ../log/watchdoglog.txt

# 启动服务程序
./procctl 3 ./ChatServer ../log/log.txt 127.0.0.1 6000 
