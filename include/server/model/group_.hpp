#ifndef GROUP_H
#define GROUP_H

#include "groupuser.hpp"
#include <string>
#include <vector>
using namespace std;


class Group
{
public:
    Group(int id=-1, string name="", string desc="")
        : id(id), name(name), desc(desc) {}
    
    void setId(int id) { this->id = id; }
    void setName(string name) { this->name = name; }
    void setDesc(string desc) { this->desc = desc; }

    int getId() { return this->id; }
    string getName() { return this->name; }
    string getDesc() { return this->desc; }

    vector<GroupUser>& getUsers() { return this->users; } 

private:
    int id;
    string name;

    // 群组的描述
    string desc;

    // 一个群组中的成员们
    // 查询之后可以提供给业务层使用，封装在一个vector里
    // 这样不用在业务层面发起多次查询（多表联合查询）
    vector<GroupUser> users;
};

#endif