集群聊天室升级版——重写网络模块，增加日志模块和守护模块

前置条件
配置ngix负载均衡，启动redis服务，启动mysql 

编译
cd build
rm -rf *
cmake ..
make

运行与终止
cd bin
运行
sh start.sh
终止
sh stop.sh
