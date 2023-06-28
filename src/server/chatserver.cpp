#include "chatserver.hpp"
#include "chatservice.hpp"
#include "json.hpp"


using namespace std;
using namespace placeholders;
using json = nlohmann::json;

ChatServer::ChatServer(EventLoop* loop, 
                       const InetAddress& listenAddr, 
                       const string& serverName)
                    : _server(loop, listenAddr, serverName)
                    , _loop(loop)

{
   // muduo的TCPServer类，封装了一个通知机制  回调函数（EventHandler）  
   // Reactor模型：
   // 有新的连接注册时 demultiplex/epoll 会把事件通知给reactor 
   // reactor之前注册好了event和eventHandler，这时候就能根据返回的event找对应的handler处理
   // 所以这里要先绑定好回调操作
   _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
   _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

   // 设置了事件回调，还要设置事件的reactor模型
   // one loop per thread
   // network I/O --> 1 thread
   // 工作线程 --> 3 thread     总和对应CPU核数  吃干榨净
   _server.setThreadNum(4);
}


void ChatServer::start()
{
   _server.start();
}


void ChatServer::onConnection(const TcpConnectionPtr& conn)
{
   // client异常断开
   if (!conn->connected())
   {
      // 懒汉式单例模式，这里需要用到这个实例了，调用它，才会构造
      ChatService::instance()->clientCloseException(conn);
      // 释放客户端的连接资源后，关闭与客户端的连接
      conn->shutdown();
   }
}


// 处理业务读写逻辑的回调
void ChatServer::onMessage(const TcpConnectionPtr& conn, muduo::net::Buffer* buffer, Timestamp time)
{
   // json string在网络中传输，现在收到的json string放在buff里
   string buff = buffer->retrieveAllAsString();
   
   // buff里读取到的是整体的json string，要反序列化回输入方输进来的格式 
   json js = json::parse(buff);

   // 解耦网络代码和业务代码
   // 而不是直接写if - else  / switch 在这里调用业务逻辑
   // 通过json字符串中的msgid字段  绑定上callback --> 也就是eventHandler
   // 反序列化，查出来是哪个msgid内容后，调用哪个handler去处理，把conn、js、time传给对应的回调就行
   auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
   msgHandler(conn, js, time);

}