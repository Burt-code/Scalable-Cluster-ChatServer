#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <functional>
#include <cstdlib>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>

#include "json.hpp"
#include "group_.hpp"
#include "user_.hpp"
#include "share.hpp"

using namespace std;
using json = nlohmann::json;


// 记录当前系统中间用户的登录信息，好友列表，群组列表
User g_currentUser;
vector<User> g_currentUserFriendList;
vector<Group> g_currentUserGroupList;

// 控制主菜单页面程序
bool isMainMenuRunning = false;

// 创建读写线程间通信的信号量
sem_t rwsem;

// 记录登录状态
atomic_bool g_isLoginSuccess{false};


// 主线程是发消息
// 设置接收线程处理回调，然后通知信号量和共享内存 通知主线程
void readTaskHandler(int clientfd);

// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime();

// 聊天主界页面
void mainMenu(int);

// 显示登录成功用户的信息
void showCurrentUserData();


// 聊天客户端，main线程是发送线程，子线程是接收线程
int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "请重新键入命令! 例： ./ChatServer 127.0.0.1 6000" << endl;
        exit(-1);
    }

    // 解析通过命令行参数传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建客户端socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        cerr << "创建socket失败" << endl;
        exit(-1); 
    }

    // 填写客户端需要连接的服务器ip和端口
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // 与服务器进行连接
    if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        cerr << "连接失败" << endl;
        close(clientfd);
        exit(-1);
    }

    // 初始化读写线程通信用的信号量
    // 初始化读写线程通信用的信号量(为0)
    // 信号量为1是应用于A、B两个进程构建互斥信号
    // A首先执行   P（-1) --> 信号量为0  A可执行  B阻塞
    // A执行完了   V(+1) --> 信号量为0   唤醒阻塞状态的B

    // 这里通信线程>2，所以初始化信号量为0
    // 这样当有先后执行依赖关系时
    // 后执行的会先被安排执行P(-1) --> 信号量为-1  后执行的没法执行 阻塞住
    // 先执行的会先执行动作，然后执行V(+1) 
    // --> 如果有后执行者先把信号量卡为-1了，那么此时信号量恢复为0   唤醒后执行的拿到先执行的内容再执行下去

    // 典型案例：连接池  生产者-消费者模型 
    sem_init(&rwsem, 0, 0);  // 信号量初始化为0


    // 连接服务器成功，启动接收子线程
    std::thread readTask(readTaskHandler, clientfd); // pthread_create
    readTask.detach();                               // pthread_detach

    // main线程用于接收用户输入，负责发送数据
    for (;;)
    {
        // 显示首页面菜单 登录、注册、退出
        cout << "========================" << endl;
        cout << "1. 登录" << endl;
        cout << "2. 用户注册" << endl;
        cout << "3. 离开" << endl;
        cout << "========================" << endl;
        cout << "choice:";
        int choice = 0;
        cin >> choice;
        cin.get(); // 读掉缓冲区残留的回车

        switch (choice)
        {
        // 登录
        case 1: 
        {
            int id = 0;
            char pwd[50] = {0};
            cout << "请输入系统给您分配的id: ";
            cin >> id;
            cin.get();      // 读掉缓冲区残留的回车 
            cout << "请输入您的密码:";     
            
            // 密码暗文处理
            disableEcho();   
            cin.getline(pwd, 50);
            enableEcho();

            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            string request = js.dump();

            g_isLoginSuccess = false;

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << " 向服务器发送登录消息:" << request << "失败" << endl;
            }

            // 等待信号量，由子线程处理完登录的响应消息后，通知这里
            // sem_wait执行P操作 （信号量-1）
            sem_wait(&rwsem); 
                
            if (g_isLoginSuccess) 
            {
                // 进入聊天主菜单
                isMainMenuRunning = true;
                mainMenu(clientfd);
            }
        }
        break;

        // 注册
        case 2: 
        {
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "请输入您想要注册的用户名:";
            cin.getline(name, 50);
            cout << "请输入您想要注册的账户的密码:";
            disableEcho();
            cin.getline(pwd, 50);
            enableEcho();

            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << " 发送信息错误 " << request << endl;
            }
            
            sem_wait(&rwsem); // 等待信号量，子线程处理完注册消息会通知
        }
        break;

        case 3:  // quit
            close(clientfd);
            sem_destroy(&rwsem);
            exit(0);
        default:
            cerr << " 输入有误，请您检查并重新输入 " << endl;
            break;
        }
    }

    return 0;
}

// 处理注册的响应逻辑
void doRegResponse(json &responsejs)
{
    if (0 != responsejs["errno"].get<int>()) // 注册失败
    {
        cerr << "用户名已被注册!" << endl;
    }
    else // 注册成功
    {
        cout << "注册成功, 您的用户id是: " << responsejs["id"] << "可用于之后的登录" << endl;
    }
}

// 处理登录的响应逻辑
void doLoginResponse(json &responsejs)
{
    if (0 != responsejs["errno"].get<int>()) // 登录失败
    {
        cerr << responsejs["errmsg"] << endl;
        g_isLoginSuccess = false;
    }
    else // 登录成功
    {
        // 记录当前用户的id和name
        g_currentUser.setId(responsejs["id"].get<int>());
        g_currentUser.setName(responsejs["name"]);

        // 记录当前用户的好友列表信息
        if (responsejs.contains("friends"))
        {
            g_currentUserFriendList.clear();

            vector<string> vec = responsejs["friends"];
            for (string &str : vec)
            {
                json js = json::parse(str);
                User user(js["id"].get<int>(), js["name"], "", js["state"]);
                // (int id=-1, string name="", string pwd="", string state="offline"
                // user.setId(js["id"].get<int>());
                // user.setName(js["name"]);
                // user.setState(js["state"]);
                g_currentUserFriendList.emplace_back(user);
            }
        }

        // 记录当前用户的群组列表信息
        if (responsejs.contains("groups"))
        {
            // 初始化
            g_currentUserGroupList.clear();

            vector<string> vec1 = responsejs["groups"];
            for (string &groupstr : vec1)
            {
                json grpjs = json::parse(groupstr);
                Group group;
                group.setId(grpjs["id"].get<int>());
                group.setName(grpjs["groupname"]);
                group.setDesc(grpjs["groupdesc"]);

                vector<string> vec2 = grpjs["users"];
                for (string &userstr : vec2)
                {
                    json js = json::parse(userstr);
                    GroupUser user(js["id"].get<int>(), js["name"], js["state"], js["role"]);
                    // user.setId(js["id"].get<int>());
                    // user.setName(js["name"]);
                    // user.setState(js["state"]);
                    // user.setRole(js["role"]);
                    group.getUsers().emplace_back(user);
                }

                g_currentUserGroupList.emplace_back(group);
            }
        }

        // 显示登录用户的信息
        showCurrentUserData();

        // 显示当前用户的离线消息  个人聊天信息 或者 群消息
        if (responsejs.contains("offlinemsg"))
        {
            vector<string> vec = responsejs["offlinemsg"];
            for (string &str : vec)
            {
                json js = json::parse(str);
                // time + [id] + name + " said: " + xxx
                if (ONE_CHAT_MSG == js["msgid"].get<int>())
                {
                    cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                            << "说: " << js["msg"].get<string>() << endl;
                }
                else
                {
                    cout << "群消息[" << js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                            << "说: " << js["msg"].get<string>() << endl;
                }
            }
        }

        // 记录登陆成功的状态
        g_isLoginSuccess = true;
    }
}


// 子线程 - 接收线程
void readTaskHandler(int clientfd)
{
    for (;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);  // 阻塞
        if (-1 == len || 0 == len)
        {
            close(clientfd);
            exit(-1);
        }

        // 接收ChatServer转发的数据，反序列化生成json数据对象
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if (ONE_CHAT_MSG == msgtype)
        {
            cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " 说: " << js["msg"].get<string>() << endl;
            continue;
        }

        if (GROUP_CHAT_MSG == msgtype)
        {
            cout << "群消息[" << js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " 说: " << js["msg"].get<string>() << endl;
            continue;
        }

        if (LOGIN_MSG_ACK == msgtype)
        {
            // 处理登录响应的业务逻辑
            doLoginResponse(js); 
            // sem_post执行V操作  信号量 + 1
            sem_post(&rwsem);    // 通知因为信号量<0而阻塞的主线程  登录结果已经处理完成
            continue;
        }

        if (REG_MSG_ACK == msgtype)
        {
            // 处理注册响应的业务逻辑
            doRegResponse(js);
            sem_post(&rwsem);    // 通知主线程，注册结果处理完成
            continue;
        }
    }
}

// 显示当前登录成功用户的基本信息
void showCurrentUserData()
{
    cout << "====================== 登录成功 ======================" << endl;
    cout << "当前您登录的id为 :" << g_currentUser.getId() << " 用户名为:" << g_currentUser.getName() << endl;

    cout << "---------------------- 您的好友信息如下： ---------------------\n" << endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    else
    {
        cout << "您暂时没有添加好友，欢迎您踊跃地在社群里与其他朋友互动，并建立好友关系\n" << endl;
    }

    cout << "---------------------- 您加入的群聊 ----------------------" << endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            cout << " 群id: " << group.getId() << "  群名称: " << group.getName() << " 群名片: " << group.getDesc() << endl;
            cout << endl;
            for (GroupUser &user : group.getUsers())
            {
                cout << " 群成员id: " << user.getId() << " 群成员名称: " << user.getName() << " 状态" << user.getState()
                     << " 群成员身份: " << user.getRole() << endl;
            }
        }
    }
    cout << "\n======================================================\n" << endl;
}

// "help" command handler
void help(int fd=0, string str="");
// "chat" command handler
void chat(int, string);
// "addfriend" command handler
void addfriend(int, string);
// "creategroup" command handler
void creategroup(int, string);
// "addgroup" command handler
void addgroup(int, string);
// "groupchat" command handler
void groupchat(int, string);
// "loginout" command handler
void loginout(int, string);

// 系统支持的客户端命令列表
unordered_map<string, string> commandMap = {
    {"help", "显示所有支持的命令, 格式help"},
    {"chat", "一对一聊天, 格式chat:friendid:message"},
    {"addfriend", "添加好友, 格式addfriend:friendid"},
    {"creategroup", "创建群组, 格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组, 格式addgroup:groupid"},
    {"groupchat", "群聊, 格式groupchat:groupid:message"},
    {"loginout", "注销, 格式loginout"}};

// 注册系统支持的客户端命令处理
unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}};

// 主聊天页面程序
void mainMenu(int clientfd)
{
    help();

    char buffer[1024] = {0};

    while (isMainMenuRunning)
    {
        cin.getline(buffer, 1024);
        string commandbuf(buffer);
        string command; 
        int idx = commandbuf.find(":");
        if (-1 == idx)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }
        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())
        {
            cerr << "invalid input command!" << endl;
            continue;
        }

        // 调用相应命令的事件处理回调
        // mainMenu对修改封闭，添加新功能不需要修改该函数
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx)); // 调用命令处理方法
    }
}

// "help" command handler
void help(int, string)
{
    cout << " 你可以执行如下命令 >>> " << endl;
    for (auto &p : commandMap)
    {
        cout << p.first << " : " << p.second << endl;
    }
    cout << endl;
}

// "addfriend" command handler
void addfriend(int clientfd, string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "您发送的添加好友请求 -> " << buffer << "失败了" << endl;
    }
}

// "chat" command handler
void chat(int clientfd, string str)
{
    int idx = str.find(":"); // friendid:message
    if (-1 == idx)
    {
        cerr << "消息发送格式有误，发送失败!" << endl;
        return;
    }

    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << " 您向好友发送的请求 -> " << buffer << "失败了" << endl;
    }
}
// "creategroup" command handler  groupname:groupdesc
void creategroup(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "您创建群组的申请失败了" << endl;
        return;
    }

    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr <<  " 您发送的群消息 -> " << buffer  << "失败" << endl;
    }
}
// "addgroup" command handler
void addgroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << " 您申请进入群聊 " << buffer << "的请求失败了" << endl;
    }
}
// "groupchat" command handler   groupid:message
void groupchat(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "您输入的群聊消息指令有错误，请核对" << endl;
        return;
    }

    int groupid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << " 发送群聊消息-> " << buffer << "失败, 请重试" << endl;
    }
}
// "loginout" command handler
void loginout(int clientfd, string)
{
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << " 注销用户的消息 " << buffer << " 发送失败" << endl;
    }
    else
    {
        isMainMenuRunning = false;
    }   
}

// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}