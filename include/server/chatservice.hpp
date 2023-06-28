#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <iostream>
#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>

#include "json.hpp"
#include "usermodel.hpp"
#include "groupmodel.hpp"
#include "friendmodel.hpp"
#include "offlinemessagemodel.hpp"
#include "redis.hpp"


using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

// msgid映射的callback
using MsgHandler = std::function<void(const TcpConnectionPtr& conn, json& js, Timestamp)>;


class ChatService
{
public:
    // 实例对象构造
    // 静态方法在程序编译时分配内存
    // 调用到的时候才构造，巧妙实现懒汉式单例模式
    static ChatService* instance();

    // 解耦业务与网络I/O  找具体是哪个handler来处理event
    MsgHandler getHandler(int msgid);

    // 从redis消息队列获得订阅的消息
    void handleRedisSubscribeMessage(int userid, string msg);

    // ----------------------- 业务逻辑 ---------------------------
    // 处理登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理注销业务
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);

    // 服务器异常，业务重置，把在线用户设置为离线
    void reset();  


private:
    // 懒汉式单例模式，构造函数私有化
    ChatService(); 

    // reactor里的核心数据结构，存储msgid与其对应的msgHandler的绑定关系
    // using MsgHandler = std::function<void(const TcpConnectionPtr& conn, json& js, Timestamp)>;
    unordered_map<int, MsgHandler> _msgHandlerMap;

    // 存储用户的通信连接们
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    // 互斥锁确保线程安全
    mutex _connMutex;

    // --------- mysql和业务代码也解耦，这样方便技术栈迁移 ----------
    // 调用的时候用每个表的类的API（我们把每个表的类都命名为xxxModel），
    // model接口里给业务层提供的要传入的参数是表的实例对象，而不是具体的表中的字段
    // 具体的sql语句就放在mysql的Model类里面，用各个model去用xx对象请求各个字段里的内容
    // 这样就不需要业务去关心sql层面部分的代码，解耦了sql和业务逻辑
    UserModel _userModel;
    GroupModel _groupModel;
    OfflineMsgModel _offlineMsgModel;
    FriendModel _friendModel;

    // --------------- redis 用作消息队列 ------------------------
    Redis _redis;
};

#endif