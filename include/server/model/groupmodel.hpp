#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "group.hpp"
#include <string>
#include <vector>
using namespace std;
class GroupModel
{
public:
    // 创建群聊
    bool createGroup(Group &group);
    // 加入群聊
    void addGroup(int userid, int groupid, string role);
    // 查询用户所在群组信息
    vector<Group> queryGroup(int userid);
    // 根据指定的groupid查询用户群聊id列表，除userid自己，主要用户群聊业务给其他成员群发消息
    vector<int> queryGroupUsers(int userid, int groupid);
};

#endif