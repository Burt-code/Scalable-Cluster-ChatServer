#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user_.hpp"

class GroupUser : public User
{
public:
    GroupUser(int id, string name, string state, string role)
            : User(id, name, state), role(role) {}

    void setRole(string role) { this->role = role; }
    string getRole() { return this->role; }

private:
    string role;
};


#endif