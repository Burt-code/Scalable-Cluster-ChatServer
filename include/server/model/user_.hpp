#ifndef USER_H
#define USER_H

#include <string>
using namespace std;


// User ORM
class User
{
public:
    User(int id=-1, string name="", string pwd="", string state="offline")
        : id(id), name(name), password(pwd), state(state) {}
    
    // ----- set 系列是在创建好的user对象基础上 要做修改用的 --- 
    // 如果是初始化的时候，直接用上面的初始化列表（userModel里的User对象修改为这样）
    void setId(int id) { this->id = id; }
    void setName(string name) { this->name = name; }
    void setPwd(string pwd) { this->password = pwd; }
    void setState(string state) { this->state = state; }

    int getId() { return this->id; }
    string getName() { return this->name; }
    string getPwd() { return this->password; }
    string getState() { return this->state; }


// UserModel要继承的部分
protected:
    int id;
    string name;
    string password;
    string state;
};

#endif