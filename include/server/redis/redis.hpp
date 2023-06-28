#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>

using namespace std;

// 解耦各个服务器之间的跨服务器硬连接
// 用redis作为消息中间件   
// 来实现跨服务器的通信问题
// 实现功能（订阅通道、发布消息、取消订阅、上报消息给服务器）消息传递的中间媒介是redis

// 使用redis的发布-订阅功能是把redis当成观察者模式使用：
// 关注对象一（redis）对多（server）的关系
// 多个对象都依赖于1个对象，当这个对象的状态发生改变的时候，其他对象都能够收到对应的通知

// 我们想消息队列订阅消息，发布消息，消息队列把需要我们知道的消息上报回来
// 具体过程：
// 两个客户端C1和C2分别登录在两台服务器S1和S2上，服务器通过redis作为消息中间件进行消息通信。
// 当客户端C1和C2通信时，C1先发送消息到S1上，S1向redis上名字为C2的channel通道发布消息，
// S2服务器从C2的channel上接收消息后，再把消息转发给C2客户端，
// 注意hiredis提供的发送redis命令行接口函数redisCommand，
// 由于订阅/subscribe完channel后   是阻塞等待上报信息 --》开单独线程来做
// 是同步的，而不是异步的libevent(aio编程更复杂，本项目暂不需要)
// 在发送发布-订阅命令时，需要在不同的上下文Context环境中进行，不能够在同一Context下进行。

class Redis
{
public:
    Redis();
    ~Redis();

    bool connect();

    bool publish(int channel, string msg);

    bool subscribe(int channel);

    bool unsubscribe(int channel);

    // 初始化回调对象（私有属性）
    void init_notify_handler(function<void(int, string)> fn);

    // 接收通道中的消息
    void observer_channel_message();
    

private:
    // 由于订阅/subscribe完channel后   是阻塞等待上报信息
    // 在发送发布-订阅命令时，需要在不同的上下文Context环境中进行，不能够在同一Context下进行。
    redisContext* _subscribe_context;
    redisContext* _publish_context;

    // 订阅的channel收到消息时，redis要给service层上报(观察者-监听者模式)
    // 要想能够到时候通知，需要先绑定事件回调handler
    //           channel data
    function<void(int, string)> _notify_message_handler;
};

#endif