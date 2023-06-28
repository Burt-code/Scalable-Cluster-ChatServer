#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "group_.hpp"
#include <string>
#include <vector>
using namespace std;


// 维护群组信息的操作接口方法
class GroupModel
{
public:
    bool createGroup(Group& group);

    // 用户加入群组
    // 多对多  用到的中间表 GroupUser
    void addGroup(int userid, int groupid, string role);

    // 用户能够用下面的API查到加入了哪些组，组里有谁
    // 组里成员的用户号，是否在线，身份是管理员还是普通成员
    // 解耦数据库代码
    vector<Group> queryGroups(int userid);

    // 根据指定的groupid查询群组用户id列表，
    // 除userid自己，主要用户群聊业务给群组其它成员群发消息
    // 在线就直接群发消息，不在的话就存那个不在线的对象的离线消息
    vector<int> queryGroupUsers(int userid, int groupid);
};

#endif