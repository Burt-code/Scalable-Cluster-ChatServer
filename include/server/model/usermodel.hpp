#ifndef USERMODEL_H
#define USERMODEL_H

#include "user_.hpp"


class UserModel
{
public:
    bool insert(User& user);

    // 根据用户id查用户的User类信息
    User query(int id);

    bool updateState(User& user);

    // 服务器宕机等异常，得把所有online的client置为offline
    void resetState();
};

#endif