#pragma once
#include "user.hpp"
//User表的数据操作类
class UserModel{
public:
//User表的增加方法
    bool insert(User &user);
    User query(int id);
    //更新用户的状态信息
    bool updateState(User user);
    //重置所有用户状态信息
    void resetState();
};