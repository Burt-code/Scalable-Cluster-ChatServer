#include "friendmodel.hpp"
#include "db.h"

// 添加好友关系
void FriendModel::insert(int userid, int friendid)
{
    char sql[1024] = {0};
    sprintf(sql, "INSERT INTO friend(userid, friendid) values(%d, %d)", userid, friendid);

    MySQL mysql;
    if (mysql.connect())
    // if (mysql.connect("127.0.0.1", 3306, "root", "123456", "chat"))
    {
        mysql.update(sql);
    }
}

// 返回用户好友列表
vector<User> FriendModel::query(int userid)
{
    char sql[1024] = {0};

    // 拿userid找它的friendid
    // user表和friend表的联合查询
    sprintf(sql, "SELECT a.id,a.name,a.state from user a inner join friend b on b.friendid = a.id where b.userid=%d", userid);

    vector<User> vec;
    MySQL mysql;
    // if (mysql.connect("127.0.0.1", 3306, "root", "123456", "chat"))
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            // 下线后到下次上线前，好友列表不变化
            // 上线的时候把userid用户的所有离线消息放入vec中返回即可
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.emplace_back(user);
            }
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;
}