#include <iostream>
#include "redis.hpp"

using namespace std;

Redis::Redis() : _publish_context(nullptr), _subscribe_context(nullptr) {}

Redis::~Redis()
{
    if (_publish_context != nullptr)
    {
        redisFree(_publish_context);
    }

    if (_subscribe_context != nullptr)
    {
        redisFree(_subscribe_context);
    }
}


void Redis::init_notify_handler(function<void(int, string)> fn)
{
    this->_notify_message_handler = fn;
}

// 本机redis知名端口 6379
bool Redis::connect()
{
    // 连接一个负责publish消息的上下文环境
    _publish_context = redisConnect("127.0.0.1", 6379);
    if (_publish_context == nullptr)
    {
        cerr << "Cannot connect to redis backend!" << endl;
        return false;
    }

    // 连接一个负责subscribe消息的上下文环境
    _subscribe_context = redisConnect("127.0.0.1", 6379); 
    if (_subscribe_context == nullptr)
    {
        cerr << " cannot connect to redis backend to subscribe msg!" << endl;
    }

    // subscribe会阻塞式进行等待消息到来（监听）
    // 这个过程应该开一个单独线程，否则主线程就阻塞在这里了
    thread t([&]() { observer_channel_message();} );
    t.detach();

    cout << "connect to redis backend successfully." << endl;
    return true;
}


// 独立开一个线程处理订阅通道的消息监听过程
void Redis::observer_channel_message()
{
    redisReply* reply = nullptr;
    // 在_subscribe_context 上以循环阻塞方式等待消息发生,再上报
    while (REDIS_OK == redisGetReply(this->_subscribe_context, (void**)& reply))
    {
        // 消息格式是三元素的数组
        // 1) "message"  --> reply->element[0]
        // 2) "13"    // channel对应的int整数 是哪个通道 -> reply->element[1]
        // 3) "hello"  // 具体消息  -> reply->element[2]

        // 如果通道上发生了消息,那么给业务层上报
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            // 回调，上报channel是谁，以及消息内容
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }
        freeReplyObject(reply);
    }

    // 应该一直监听，如果跳出了while
    cerr << " Observer_channel_message force quit abruptly" << endl;
}


// 向redis指定的channel发送消息
bool Redis::publish(int channel, string msg)
{
    // 例如：publish 13 "hello"
    // redisCommand首先 调用redisAppendCommand  把输入的命令PUBLISH xxx 缓存到本地
    // 然后调用redisBufferWrite 把命令发送到redis server上
    // 然后调用 redisGetReply  阻塞式等待
    // 因此publish方法可以直接调用redisAppendCommand
    // 因为Publish后会立即返回结果  无需阻塞式等在这里
    redisReply* reply = (redisReply*)redisCommand(_publish_context, "PUBLISH %d %s", channel, msg.c_str());

    if (reply == nullptr)
    {
        cerr << "publish command failed!" << endl;
        return false;
    }
    // 返回值是动态生成的结构体，需要释放，否则泄漏内存
    freeReplyObject(reply);
    return true;
}


// 向redis指定的通道subscribe订阅消息
bool Redis::subscribe(int channel)
{
    // subscribe会阻塞式等待，所以不能够直接用redisCommand
    // 具体讲：
    // SUBSCRIBE命令本身会造成线程阻塞等待通道里面发生消息，这里只做订阅通道，不接收通道消息
    // 通道消息的接收专门在observer_channel_message函数中的独立线程中进行
    // 只负责发送命令，不阻塞接收redis server响应消息，否则和notifyMsg线程抢占响应资源
    if (REDIS_ERR == redisAppendCommand(this->_subscribe_context, "SUBSCRIBE %d", channel))
    {
        cerr << "Subscribe channel failed! " << endl;
        return false;
    }

    // redisBufferWrite()循环着把缓冲区的命令发给redis server
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            cerr << "Subscribe channel failed!" << endl;
            return false;
        }
    }

    return true;
}


// 向redis指定的通道unsubscribe取消订阅消息
bool Redis::unsubscribe(int channel)
{
    if (REDIS_ERR == redisAppendCommand(this->_subscribe_context, "UNSUBSCRIBE %d", channel))
    {
        cerr << "unsubscribe command failed!" << endl;
        return false;
    }
    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            cerr << "unsubscribe command failed!" << endl;
            return false;
        }
    }
    return true;
}



