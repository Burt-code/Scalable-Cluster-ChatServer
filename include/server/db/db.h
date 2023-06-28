#ifndef DB_H
#define DB_H

#include <mysql/mysql.h>
#include <string>
#include <ctime>
using namespace std;


class MySQL
{
public:
    // 还没有输入用户名、密码登录进来，无法连接，不能理解错误
    // 这里并不是初始化连接，而是开辟一块用于连接资源的存储空间
    // 析构时用~MySQL() 把资源close掉
    MySQL();

    // 释放MySQL资源
    ~MySQL();

    // 连接到数据库
    // bool connect(string ip, unsigned short port, string user,
    //              string password, string dbname);
    bool connect();

    // 更新操作(insert/update/delete)
    bool update(string sql);

    // 查询 select 操作
    MYSQL_RES* query(string sql);
    
    // 获取私有字段定义的连接
    MYSQL* getConnection();

    void refreshAliveTime() { _alivetime = clock(); }

    // 当前时间与之前记录的时间之间的差值
    clock_t getAliveTime() const { return clock() - _alivetime; }

private:
    MYSQL* _conn;

    clock_t _alivetime;
};

#endif