#include <muduo/base/Logging.h>
#include "db.h"

// 配置数据库的基本信息
// 理论上讲 可以写在配置文件里  或者设置其他的配置方案等
// 这个项目简化处理，用静态变量声明一下
static string ip = "127.0.0.1";  // 本机回环地址
static string user = "root";
static string password = "123456";
static string dbname = "chat";


MySQL::MySQL()
{
    // MYSQL* _conn  private字段声明了的MYSQL类变量,Mysql提供的API
    _conn = mysql_init(nullptr);
}

MySQL::~MySQL()
{
    // 连接存在则释放
    if (_conn != nullptr)
        mysql_close(_conn);
}

bool MySQL::connect()
// bool MySQL::connect(string ip="127.0.0.1", unsigned short port=3306, 
//                     string user="root", string password="123456", string dbname="chat")
{
    MYSQL* p = mysql_real_connect(_conn, ip.c_str(), user.c_str(), password.c_str(), 
                                dbname.c_str(), 3306, nullptr, 0);

    // 成功连接到MySQL 后端
    if (p != nullptr)
    {
        // C和C++代码默认的编码字符是ASCII，如果不设置，从MySQL上拉下来的中文会显示为 ？
        mysql_query(_conn, "set names utf8");
        LOG_INFO << "connect mysql success!";
    }
    else
    {
        LOG_INFO << "connect mysql fail!";
    }
    return p;
}


// 执行insert/delete/update的sql语句 (写操作)
bool MySQL::update(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << sql << "更新失败!";
        return false;
    }
    return true;
}


// 查询 select 的sql语句在这里执行 （读操作）
// MYSQL_RES是Mysql C API的结果集，包括了很多信息
// 要拿结果还得要MYSQL_ROW  取出一行的信息
MYSQL_RES* MySQL::query(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << sql << "查询失败!";
        return nullptr;
    }
    
    return mysql_use_result(_conn);
}


MYSQL* MySQL::getConnection()
{
    return _conn;
}
