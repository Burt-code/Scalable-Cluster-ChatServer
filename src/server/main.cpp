#include <iostream>
#include <signal.h>

#include "chatserver.hpp"
#include "chatservice.hpp"

using namespace std;

void recallException(int)
{
    // 懒汉式，这里调用，static实例才会被创建
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        cout << "Please check command format, e.g.: ./ChatServer 127.0.0.1 6000" << endl;
        exit(-1); // exception
    }

    char* ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // The SIGINT signal is generated when the user presses the CTRL+C key combination on the keyboard.
    // SIGIO  Linux特有
    // 第一阶段注册调函数 协商通知方式，做数据准备（未就绪->就绪) 是异步过程
    signal(SIGINT, recallException);
    // 数据就绪好，准备读取数据的时候是 阻塞在red/recv来靠应用自己读取的，而不是OS帮忙拷贝，是同步过程


    // 调用Muduo网络库，设计事件循环, ip地址， 初始化服务器
    EventLoop loop;

    // sockad_in结构体 muduo网络库绑定了ip和port
    InetAddress addr(ip, port);  

    ChatServer server(&loop, addr, "SCCS");

    server.start();
    loop.loop();

    return 0;
}