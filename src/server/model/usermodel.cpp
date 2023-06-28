#include <iostream>

#include "usermodel.hpp"
#include "db.h"

using namespace std;

// --------- 只负责增删改查 业务的事业务自己管去 -------
// 这里都是API，Model类们只需要调用这些API
// 插入成功,把插入的用户数据生成主键id --> 给用户分配的qq号，返回给用户user类的实例
// 然后返回true
bool UserModel::insert(User& user)
{
    char sql[1024] = {0};
    sprintf(sql, "INSERT INTO user(name, password, state) VALUES('%s', '%s', '%s')"
            , user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}


User UserModel::query(int id)
{
    char sql[1024] = { 0 };
    sprintf(sql, "SELECT * FROM user WHERE id = %d", id);

    MySQL mysql;
    if (mysql.connect())
    {
        // MYSQL_RES是Mysql C API的结果集，包括了很多信息
        // 下面我们需要把一行的信息提取出来 赋给创建的user对象，
        // 然后把创建的user对象返回回来,告诉我们query的user类别是什么
        MYSQL_RES* res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                User user(atoi(row[0]), row[1], row[2], row[3]);
                mysql_free_result(res);
                return user;
            }
        }
    }
}

bool UserModel::updateState(User& user)
{
    char sql[1024] = {0};
    sprintf(sql, "UPDATE user SET state='%s' WHERE id=%d", user.getState().c_str(), user.getId());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            return true;
        }       
    }
    return false;
}


void UserModel::resetState()
{
    char sql[1024] = {0};
    sprintf(sql, "UPDATE user SET state='offline' WHERE state='online'");

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}
