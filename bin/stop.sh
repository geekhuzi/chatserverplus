# 此脚本用于停止集群聊天室全部的服务程序

# 停止调度程序。
killall -9 procctl

# 尝试让所有服务程序正常停止
killall ChatServer

# 让服务程序有足够的时间退出。
sleep 5  

# 不管服务程序有没有退出，都强制杀死。
killall -9 ChatServer