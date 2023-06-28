#include <muduo/base/Logging.h>
#include <vector>

#include "chatservice.hpp"
#include "share.hpp"

using namespace std;
using namespace muduo;


ChatService* ChatService::instance()
{
    static ChatService service;
    return &service;
}

MsgHandler ChatService::getHandler(int msgid)
{
    auto it = _msgHandlerMap.find(msgid);
    if (it != _msgHandlerMap.end())
    {
        return it->second;
    }

    // 此前没有注册过msgid对应的handler，或者输错了msgid
    // 返回一个默认的空处理器->确保系统不会崩溃
    return [=](const TcpConnectionPtr& conn, json& js, Timestamp)
    {
        LOG_ERROR << "msgid: " << msgid << " cannot find handler!";
    };
}


// 构造函数
// 注册好消息和对应的一系列的回调API --> reactor
ChatService::ChatService()
{
    // -------------- 用户自身业务回调 -------------------
    _msgHandlerMap.emplace(LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3));
    _msgHandlerMap.emplace(LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3));
    _msgHandlerMap.emplace(REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3));
    _msgHandlerMap.emplace(ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3));
    _msgHandlerMap.emplace(ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3));

    // -------------- 群组业务回调 ------------------- 
    _msgHandlerMap.emplace(CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3));
    _msgHandlerMap.emplace(ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3));
    _msgHandlerMap.emplace(GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3));
    
    // 连redis服务器，设置好上报消息的回调
    if (_redis.connect())
    {
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}


// ----------------  用户登入、登出、注册、聊天、添加朋友 -----------------
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    string pwd = js["password"];

    User user = _userModel.query(id);
    // 账号密码匹配正确
    if (user.getId() == id && user.getPwd() == pwd)
    {
        // 若已经在线则不能重复登录
        if (user.getState() == "online")
        {
            json respon;
            respon["msgid"] = LOGIN_MSG_ACK;  // 登录响应
            respon["errno"] = 2;
            respon["errmsg"] = "账号已在其他地方登录，请注意！";
            conn->send(respon.dump());
        }
        else
        // 用户成功登录
        // 回调onMessage本身是多线程环境下工作的(用户上线/下线)
        // 注意STL是线程不安全的，加互斥锁处理
        // 用{} 控制锁的粒度不太大
        {
            lock_guard<mutex> lock(_connMutex);
            _userConnMap.emplace(id, conn);
        }

        // 数据库的并发依靠mysql自己的API保证，不处理

        // -------------------------- 登录成功 --------------------------------
        // 向redis订阅Channel(id)
        _redis.subscribe(id);  // channel编号(reply->element[1])就是id号码，直接赋予

        // 用户状态更新
        user.setState("online");
        _userModel.updateState(user);

        // 形成json 准备返回消息给客户端
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        response["name"] = user.getName();

        // 看有没有离线消息要传递
        // 如果有则传递给新上线的用户，然后移除离线消息
        vector<string> vec = _offlineMsgModel.query(id);
        if (!vec.empty())
        {
            response["offlinemsg"] = vec;
            _offlineMsgModel.remove(id);
        }

        // 查询好友信息并且返回给用户  显示
        vector<User> userVec = _friendModel.query(id);
        if (!vec.empty())
        {
            vector<string> tmp;
            for (auto user : userVec)
            {
                json js;
                js["id"] = user.getId();
                js["name"] = user.getName();
                js["state"] = user.getState();
                tmp.emplace_back(js.dump());
            }
            response["friends"] = tmp;
        }

        // 查询用户的群组信息
        vector<Group> groupuserVec = _groupModel.queryGroups(id);
        if (!groupuserVec.empty())
        {
            // group:[{groupid:[xxx, xxx, xxx, xxx]}]
            vector<string> groupV;
            for (Group &group : groupuserVec)
            {
                json grpjson;
                grpjson["id"] = group.getId();
                grpjson["groupname"] = group.getName();
                grpjson["groupdesc"] = group.getDesc();
                vector<string> userV;
                for (GroupUser &user : group.getUsers())
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    js["role"] = user.getRole();
                    userV.push_back(js.dump());
                }
                grpjson["users"] = userV;
                groupV.push_back(grpjson.dump());
            }

            response["groups"] = groupV;
        }

        // 以上把消息准备好了，转为json 打出去，发给client
        conn->send(response.dump());
    }
    // 账号密码不匹配 或 用户不存在
    else
    {
        json respon;
        respon["msgid"] = LOGIN_MSG_ACK;  // 登录响应
        respon["errno"] = 1;
        respon["errmsg"] = "用户名或密码错误！";
        conn->send(respon.dump());
    }
}


void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // 收到用户名密码
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = _userModel.insert(user);
    if (state)
    {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());  
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}


// 2人单独聊天
// 消息内容
 // msgid 
 // from: "zhangsan"
 // to: "lisi"
 // msg: "xxxxx"
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["toid"].get<int>();

    {
        // 要访问连接信息表
        // 锁控制map表的查询操作 粒度不宜过大
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            // toid在线，转发消息   服务器主动推送消息给toid用户
            it->second->send(js.dump());
            return;
        }
    }

    // 查询 toid 在线与否
    User user = _userModel.query(toid);
    // 在线直接打走消息
    if (user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
        return;
    }
    // 离线则存储离线消息
    _offlineMsgModel.insert(toid, js.dump());
}


// 添加好友业务 msgid id friendid
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息
    _friendModel.insert(userid, friendid);
}


// 作为管理员 creator 创建群组
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{

    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 存储新创建的群组信息
    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        // 存储群组创建人信息
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}


// 作为普通用户加入群组
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}

// 群聊
// 如果有对象是在线状态，那么就经过redis消息队列转发消息过去
// 否则存为离线消息，等对象上线后经过查询离线消息再转发给他（写在login逻辑里）
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);

    lock_guard<mutex> lock(_connMutex);
    for (int id : useridVec)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            // 转发群消息
            it->second->send(js.dump());
        }
        else
        {
            // 查询toid是否在线 
            User user = _userModel.query(id);
            if (user.getState() == "online")
            {
                _redis.publish(id, js.dump());
            }
            else
            {
                // 存储离线群消息
                _offlineMsgModel.insert(id, js.dump());
            }
        }
    }

}


// 注销用户，取消redis订阅，置为offline
// 注意线程安全
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

     // 用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(userid); 

    // 更新用户的状态信息
    User user(userid, "", "", "offline");
    _userModel.updateState(user);

}

// 客户端也可能异常崩溃
// 根据有异常的TCPconn 遍历连接map表，这个代价省不了，得搜出这个用户信息
// 删掉用户信息，state改为offline
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                // 从map表删除用户的连接信息
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    // 用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(user.getId()); 

    // 更新用户的状态信息
    if (user.getId() != -1) // 没找到就不用向数据库请求了，稍微省一些开销
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}


// 服务器宕机，把online的用户置为offline
void ChatService::reset()
{
    _userModel.resetState();
}


// 从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        it->second->send(msg);
        return;
    }

    // 存储该用户的离线消息
    _offlineMsgModel.insert(userid, msg);
}
