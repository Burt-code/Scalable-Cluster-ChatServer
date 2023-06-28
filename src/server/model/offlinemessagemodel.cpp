#include "offlinemessagemodel.hpp"
#include "db.h"

void OfflineMsgModel::insert(int userid, string msg)
{
    char sql[1024] = {0};
    sprintf(sql, "INSERT INTO offlinemessage(userid, message) VALUES(%d, '%s')",
            userid, msg.c_str());
    
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}


vector<string> OfflineMsgModel::query(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "SELECT message FROM offlinemessage WHERE userid=%d", userid);
    
    vector<string> vec;

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES* res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                vec.emplace_back(row[0]);
            }
            mysql_free_result(res);
        }
        return vec;
    }
    return vec;  // 返回空数组
}


// 发送对象上线，离线消息发过去之后，删掉离线消息
void OfflineMsgModel::remove(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "DELETE FROM offlinemessage WHERE userid=%d", userid);
    
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}



