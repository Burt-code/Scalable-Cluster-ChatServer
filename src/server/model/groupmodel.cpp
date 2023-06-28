#include "groupmodel.hpp"
#include "db.h"


// 创建群组
bool GroupModel::createGroup(Group &group)
{
    char sql[1024] = {0};
    sprintf(sql, "INSERT INTO allgroup(groupname, groupdesc) VALUES('%s', '%s')",
            group.getName().c_str(), group.getDesc().c_str());
    
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false; 
}


// 加入群组
void GroupModel::addGroup(int userid, int groupid, string role)
{
    char sql[1024] = {0};
    sprintf(sql, "INSERT INTO groupuser(groupid, userid, grouprole) VALUES(%d, %d, '%s')",
             groupid, userid, role.c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}



// 查询用户所在群组信息
vector<Group> GroupModel::queryGroups(int userid)
{
    char sql[1024] = {0};
    // 先根据userid在groupuser表中查询出该用户所属的所有群组信息
    sprintf(sql, "SELECT a.id, a.groupname, a.groupdesc FROM allgroup a INNER JOIN \
            groupuser b ON a.id=b.groupid WHERE b.userid=%d", userid);

    vector<Group> groupVec;

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES* res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                Group group(atoi(row[0]), row[1], row[2]);
                groupVec.emplace_back(group);
            }
            mysql_free_result(res);
        }
    }

    // 根据群组信息，查询属于该群组的所有用户的userid，并且和user表进行多表联合查询，查出用户的详细信息
    for (Group& group : groupVec)
    {
        sprintf(sql, "SELECT a.id, a.name, a.state FROM user a INNER JOIN \
                groupuser b ON a.id=b.userid WHERE b.groupid=%d", group.getId());
        
        MYSQL_RES* res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                GroupUser user(atoi(row[0]), row[1], row[2], row[3]);
                // 把群组成员封装在Group类定义的字段users的vector里，方便业务层面去使用
                group.getUsers().emplace_back(user);
            }
            mysql_free_result(res);
        }
    }
    return groupVec;
}


// 根据指定的groupid查询群组用户id列表，
// 除userid自己，主要用户群聊业务给群组其它成员群发消息
// 这样如果在线的话就可以直接群发消息，
// 如果不在的话就可以存下来不在线的对象的离线消息
vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    char sql[1024] = {0};
    sprintf(sql, "SELECT userid FROM groupuser WHERE groupid=%d and userid!=%d", groupid, userid);

    vector<int> idVec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES* res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                idVec.emplace_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return idVec;
}

