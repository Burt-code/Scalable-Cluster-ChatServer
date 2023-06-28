#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;

// 聊天服务器类，包括对象创建，启动服务，绑定回调函数
class ChatServer
{
public:
    ChatServer(EventLoop* loop, const InetAddress& listenAddr, const string& serverName);

    void start();

private:
    // 有连接注册时的callback
    void onConnection(const TcpConnectionPtr& conn);

    // 有读写事件发生时的callback
    void onMessage(const TcpConnectionPtr&, Buffer*, Timestamp);

    // Muduo封装好的TcpServer，直接用来作为服务器的类
    TcpServer _server;

    // muduo事件循环
    // at most one eventLoop per thread
    EventLoop* _loop;
};

#endif