# Scalable Cluster ChatServer (SCCS)  可扩展集群聊天平台

## 项目说明
本项目搭建的是能够在nginx tcp负载均衡条件下正常工作的集群聊天平台

## 搭建环境
- Ubuntu 18.04 
- GCC/G++ 7.5
- CMake 3.10.2
- nginx 1.12.2
- redis 1.1.0
- muduo 2.0.2
- boost 1_69_0 (muduo编译依赖)
- MySQL 5.7

## 执行过程/编译过程
```bash
cd build/
rm -rf *
cmake ..
make
cd ../bin

# for Server
./ChatServer [ip] [port]

# for client
./ChatClient [ip] [port]
```

## 关键技术点

- 代码低耦合度设计，技术栈可迁移性强
    - 使用muduo网络库(2.0.2版本)，解耦网络I/O与具体业务的代码
    - 使用MySQL提供的API，设计了解耦SQL执行逻辑与具体业务的代码

- 采用轻量级Redis的MQ的 publish-subscrbe 功能解决跨服务器通信硬连接的弊病 

- 使用基于轮询的nginx负载均衡策略实现了集群，扩展了并发的支持规模



## 细节说明
- tcp 负载均衡
    Ubuntu 18.04环境下，配置nginx的配置文件，加入了如下配置：

    ``` bash
    # nginx tcp loadbalance config
    stream {
        upstream MyServer {
            server 127.0.0.1:6000 weight=1 max_fails=3 fail_timeout=30s;
            server 127.0.0.1:6002 weight=1 max_fails=3 fail_timeout=30s;
        }

        server {
            proxy_connect_timeout 1s;
            listen 8000;   # 监听端口
            proxy_pass MyServer;
            tcp_nodelay on;
        }
    }
    ```

