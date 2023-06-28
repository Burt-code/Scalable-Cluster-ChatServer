// # ifndef COMMAND_H
// # define COMMAND_H

// #include <unordered_map>
// #include <functional>
// #include <semaphore.h>
// #include <atomic>
// #include <iostream>

// #include <sys/socket.h>  // recv()
// #include <unistd.h>   // closefd()
// #include <chrono>

// #include "group_.hpp"
// #include "user_.hpp"
// #include "json.hpp"
// #include "share.hpp"

// using namespace std;
// using json = nlohmann::json;

// // ---------------------------------------
// //  登录成功后，服务器会把好友离线消息列表返回
// //  在客户端记录下来，想看就直接打印
// //  不需要去服务端再把这些信息拉取下来

// // 记录当前系统中间用户的登录信息，好友列表，群组列表
// User g_currentUser;
// vector<User> g_currentUserFriendList;
// vector<Group> g_currentUserGroupList;


// // 控制主菜单页面的控制程序
// bool isMainMenuRunning = false;

// // 创建读写线程间通信的信号量
// sem_t rwsem;

// // 记录登录的状态
// atomic_bool g_isLoginSuccess {false};


// // 显示当前登录成功用户的基本信息
// void showCurrentUserData()
// {
//     cout << "======================用户登录======================" << endl;
//     cout << "恭喜你登录成功! => id:" << g_currentUser.getId() << " 昵称:" << g_currentUser.getName() << endl;
//     cout << "----------------------friend list---------------------" << endl;
//     if (!g_currentUserFriendList.empty())
//     {
//         for (User& user : g_currentUserFriendList)
//         {
//             cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
//         }
//     }
//     cout << "---------------------- 你所加入的群聊 ----------------------" << endl;
//     if (!g_currentUserGroupList.empty())
//     {
//         for (Group& group : g_currentUserGroupList)
//         {
//             cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
//             for (GroupUser& user : group.getUsers())
//             {
//                 cout << user.getId() << " " << user.getName() << " " << user.getState()
//                      << " " << user.getRole() << endl;
//             }
//         }
//     }
//     cout << "======================================================" << endl;
// }

// // 处理登录的响应逻辑
// void doLoginResponse(json &responsejs)
// {
//     // 登录失败
//     if (0 != responsejs["erron"].get<int>())
//     {
//         cerr << responsejs["errmsg"] << endl;
//         g_isLoginSuccess = false;
//     }
//     // 登录成功
//     else
//     {
//         // 记录当前用户的id和name
//         g_currentUser.setId(responsejs["id"].get<int>());
//         g_currentUser.setName(responsejs["name"]);

//         // 记录当前用户的好友列表信息
//         if (responsejs.contains("friends"))
//         // {
//             g_currentUserFriendList.clear();
            
//             vector<string> vec = responsejs["friends"];
//             for (string& str : vec)
//             {
//                 json js = json::parse(str);
//                 User user(js["id"].get<int>(), js["name"], "", js["state"]);
//                 // (int id=-1, string name="", string pwd="", string state="offline"
//                 // user.setId(js["id"].get<int>());
//                 // user.setName(js["name"]);
//                 // user.setState(js["state"]);
//                 g_currentUserFriendList.emplace_back(user);
//             }
//         }

//         // 记录当前用户的群组列表信息
//         if (responsejs.contains("groups"))
//         {
//             g_currentUserGroupList.clear();

//             vector<string> vec1 = responsejs["groups"];
//             for (string& groupstr : vec1)
//             {
//                 json grpjs = json::parse(groupstr);
//                 Group group(grpjs["id"].get<int>(), grpjs["groupname"], grpjs["groupnamegroupdesc"]);
//                 // group.setId(grpjs["id"].get<int>());
//                 // group.setName(grpjs["groupname"]);
//                 // group.setDesc(grpjs["groupdesc"]);

//                 vector<string> vec2 = grpjs["users"];
//                 for (string& userstr : vec2)
//                 {
//                     json js = json::parse(userstr);
//                     GroupUser user(js["id"].get<int>(), js["name"], js["state"], js["role"]);
//                     // user.setId(js["id"].get<int>());
//                     // user.setName(js["name"]);
//                     // user.setState(js["state"]);
//                     // user.setRole(js["role"]);
//                     group.getUsers().emplace_back(user);
//                 }

//                 g_currentUserGroupList.emplace_back(group);
//             }
//         }

//         // 显示登录用户的基本信息
//         showCurrentUserData();

//         // 显示当前用户的离线消息  个人聊天信息或者群组消息
//         if (responsejs.contains("offlinemsg"))
//         {
//             vector<string> vec = responsejs["offlinemsg"];
//             for (string &str : vec)
//             {
//                 json js = json::parse(str);
//                 // time + [id] + name + " said: " + xxx
//                 if (ONE_CHAT_MSG == js["msgid"].get<int>())
//                 {
//                     cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
//                             << " said: " << js["msg"].get<string>() << endl;
//                 }
//                 else
//                 {
//                     cout << "群消息[" << js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
//                             << " said: " << js["msg"].get<string>() << endl;
//                 }
//             }
//         }

//         // 登录成功 记录状态
//         g_isLoginSuccess = true;
//     }
// }


// // 处理注册的逻辑
// void doRegResponse(json &responsejs)
// {
//     if (0 != responsejs["errno"].get<int>()) // 注册失败
//     {
//         cerr << "用户名已被注册!" << endl;
//     }
//     else // 注册成功
//     {
//         cout << "注册成功, 用户id是: " << responsejs["id"] << endl;
//     }
// }

// // 主线程发消息
// // 设置接收线程处理回调，然后通知信号量和共享内存 通知主线程
// void readTaskHandler(int clientfd)
// {
//     while (true)
//     {
//         char buffer[1024] = {0};
//         int len = recv(clientfd, buffer, 1024, 0);
//         if (len == -1 || len == 0)
//         {
//             close(clientfd);
//             exit(-1);
//         }

//         // 接收到Server经过Reis转发的数据包，反序列化成json对象
//         json js = json::parse(buffer);
//         int msgtype = js["msgid"].get<int>();

//          if (ONE_CHAT_MSG == msgtype)
//         {
//             cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
//                  << " said: " << js["msg"].get<string>() << endl;
//             continue;
//         }

//         if (GROUP_CHAT_MSG == msgtype)
//         {
//             cout << "群消息[" << js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
//                  << " said: " << js["msg"].get<string>() << endl;
//             continue;
//         }

//         if (LOGIN_MSG_ACK == msgtype)
//         {
//             doLoginResponse(js); // 处理登录响应的业务逻辑
//             sem_post(&rwsem);    // 通知主线程，登录结果处理完成
//             continue;
//         }

//         if (REG_MSG_ACK == msgtype)
//         {
//             doRegResponse(js);
//             sem_post(&rwsem);    // 通知主线程，注册结果处理完成
//             continue;
//         }
//     }
// }

// // 获取系统时间（聊天信息需要添加时间信息）
// string getCurrentTime()
// {
//     auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
//     struct tm *ptm = localtime(&tt);
//     char date[60] = {0};
//     sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
//             (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
//             (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
//     return std::string(date);
// }



// // -------------------------------- 解耦业务代码的数据结构 -------------------------------
// // 用回调来处理

// // "help" command handler
// void help();
// // "chat" command handler
// void chat(int, string);
// // "addfriend" command handler
// void addfriend(int, string);
// // "creategroup" command handler
// void creategroup(int, string);
// // "addgroup" command handler
// void addgroup(int, string);
// // "groupchat" command handler
// void groupchat(int, string);
// // "loginout" command handler
// void loginout(int, string);


// // 系统支持的客户端命令列表
// unordered_map<string, string> commandMap = 
// {
//     {"help", "显示所有支持的命令,格式help"},
//     {"chat", "一对一聊天,格式chat:friendid:message"},
//     {"addfriend", "添加好友,格式addfriend:friendid"},
//     {"creategroup", "创建群组,格式creategroup:groupname:groupdesc"},
//     {"addgroup", "加入群组,格式addgroup:groupid"},
//     {"groupchat", "群聊,格式groupchat:groupid:message"},
//     {"loginout", "注销,格式loginout"}
// };


// // 注册系统支持的客户端命令处理
// unordered_map<string, function<void(int, string)>> commandHandlerMap = 
// {
//     {"help", help},
//     {"chat", chat},
//     {"addfriend", addfriend},
//     {"creategroup", creategroup},
//     {"addgroup", addgroup},
//     {"groupchat", groupchat},
//     {"loginout", loginout}
// };



// // -------------------------------- 具体的业务代码 -------------------------------- 
// // {"chat", "一对一聊天,格式chat:friendid:message"}
// void chat(int clientfd, string str)
// {
//     int idx = str.find(":");
//     if (-1 == idx)
//     {
//         cerr << " 输入的聊天命令有误 " << endl;
//         return;
//     }

//     int friendid = atoi(str.substr(0, idx).c_str());
//     string msg = str.substr(idx + 1, str.size() - idx);

//     json js;
//     js["msgid"] = ONE_CHAT_MSG;
//     js["id"] = g_currentUser.getId();
//     js["name"] = g_currentUser.getName();
//     js["toid"] = friendid;
//     js["msg"] = msg;
//     js["time"] = getCurrentTime();
//     string buffer = js.dump();

//     int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
//     if (-1 == len)
//     {
//         cerr << "发送聊天消息 " << buffer << " 失败" << endl; 
//     }
// }

// // {"addfriend", "添加好友,格式addfriend:friendid"},
// void addfriend(int clientfd, string str)
// {
//     int friendid = atoi(str.c_str());

//     json js;
//     js["msgid"] = ADD_FRIEND_MSG;
//     js["id"] = g_currentUser.getId();
//     js["friendid"] = friendid;
//     string buffer = js.dump();

//     int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
//     if (-1 == len)
//     {
//         cerr << "添加好友 " << buffer << " 失败" << endl; 
//     }
// }

// // {"creategroup", "创建群组,格式creategroup:groupname:groupdesc"},
// void creategroup(int clientfd, string str)
// {
//     int idx = str.find(":");
//     if (-1 == idx)
//     {
//         cerr << "creategroup command invalid!" << endl;
//         return;
//     }

//     string groupname = str.substr(0, idx);
//     string groupdesc = str.substr(idx + 1, str.size() - idx);

//     json js;
//     js["msgid"] = CREATE_GROUP_MSG;
//     js["id"] = g_currentUser.getId();
//     js["groupname"] = groupname;
//     js["groupdesc"] = groupdesc;
//     string buffer = js.dump();

//     int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
//     if (-1 == len)
//     {
//         cerr << "创建群组 " << buffer << " 失败" << endl; 
//     }
// }

// // {"addgroup", "加入群组,格式addgroup:groupid"},
// void addgroup(int clientfd, string str)
// {
//     int groupid = atoi(str.c_str());
//     json js;
//     js["msgid"] = ADD_GROUP_MSG;
//     js["id"] = g_currentUser.getId();
//     js["groupid"] = groupid;
//     string buffer = js.dump();

//     int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
//     if (-1 == len)
//     {
//         cerr << "加入群组 " << buffer << " 失败" << endl; 
//     }
// }

// // {"groupchat", "群聊,格式groupchat:groupid:message"},
// void groupchat(int clientfd, string str)
// {
//     int idx = str.find(":");
//     if (-1 == idx)
//     {
//         cerr << "发送群聊消息格式有误，请检查!" << endl;
//         return;
//     }

//     int groupid = atoi(str.substr(0, idx).c_str());
//     string message = str.substr(idx + 1, str.size() - idx);

//     json js;
//     js["msgid"] = GROUP_CHAT_MSG;
//     js["id"] = g_currentUser.getId();
//     js["name"] = g_currentUser.getName();
//     js["groupid"] = groupid;
//     js["msg"] = message;
//     js["time"] = getCurrentTime();
//     string buffer = js.dump();

//     int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
//     if (-1 == len)
//     {
//         cerr << "发送群聊消息 " << buffer << " 失败" << endl; 
//     }
// }

// //  {"loginout", "注销,格式loginout"}
// void loginout(int clientfd, string)
// {
//     json js;
//     js["msgid"] = LOGINOUT_MSG;
//     js["id"] = g_currentUser.getId();
//     string buffer = js.dump();

//     int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
//     if (-1 == len)
//     {
//         cerr << "发送注销账号消息 " << buffer << " 失败" << endl; 
//     }
//     else
//     {
//         isMainMenuRunning = false;
//     }   
// }

// // 帮助
// void help()
// {
//     cout << "你可以执行如下命令： >>> " << endl;
//     for (auto &p : commandMap)
//     {
//         cout << p.first << " : " << p.second << endl;
//     }
//     cout << endl;

// }


// // main函数会接入的主菜单API  聊天主界面 程序
// void mainMenu(int clientfd)
// {
//     help();    
    
//     char buffer[1024] = {0};

//     while (isMainMenuRunning)
//     {
//         cin.getline(buffer, 1024);
//         string commandbuf(buffer);
//         string command;
        
//         int idx = commandbuf.find(":");
//         if (-1 == idx)
//         {
//             command = commandbuf;
//         }
//         else
//         {
//             command = commandbuf.substr(0, idx);
//         }

//         auto it = commandHandlerMap.find(command);
//         if (it == commandHandlerMap.end())
//         {
//             cerr << "输入的指令有误，请检查" << endl;
//             continue;
//         }

//         // 调用响应的命令处理事件回调函数
//         // mainMenu对修改封闭，添加新功能不需要修改该函数
//         it->second(clientfd, commandbuf.substr(idx+1, commandbuf.size()-idx)); // 调用命令处理方法
//     }
// }

// # endif