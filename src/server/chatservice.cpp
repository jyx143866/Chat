/***
 * @Author: jyx
 * @Date: 2024-11-14 05:09:07
 * @LastEditors: jyx
 * @Description:
 */
#include "chatservice.hpp"
#include "public.hpp"

#include <string>
#include <vector>
#include <muduo/base/Logging.h>
using namespace std;
using namespace muduo;

// 获取单例对象的接口函数
ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}
// 构造方法，注册消息和对应的回调操作
ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
}

// 服务器异常，业务重置方法
void ChatService::reset()
{
    // 把online状态的用户设置成offline
    _usermodel.resetState();
}

// 获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志，msgid没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        LOG_ERROR << "msgid:" << msgid << "can not find handler !";
        // 返回一个默认的处理器，空操作
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time)
        {
            LOG_ERROR << "msgid:" << msgid << "can not find handler!";
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}

// 处理登录业务
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    string pwd = js["password"];

    User user = _usermodel.query(id);
    if (user.getId() == id && user.getPwd() == pwd)
    {
        if (user.getState() == "online")
        {
            // 该用户已经登录，不允许重复登录
            json responce;
            responce["msgid"] = LOGIN_MSG_ACK;
            responce["errno"] = 2;
            responce["errmsg"] = "该账号已经登录，不能再次登录！";
            conn->send(responce.dump());
        }
        else
        {
            // 登录成功，记录连接信息
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnectionMap.insert({id, conn});
            }
            // 登录成功
            user.setState("online");
            _usermodel.updateState(user);

            json responce;
            responce["msgid"] = LOGIN_MSG_ACK;
            responce["errno"] = 0;
            responce["id"] = user.getId();
            responce["name"] = user.getName();
            // 查询该用户是否有离线消息
            vector<string> vec = _offlineMsgModel.query(user.getId());
            if (!vec.empty())
            {
                responce["offlinemsg"] = vec;
                // 读取完该用户的离线消息后，把该用户的所有离线消息删除掉
                _offlineMsgModel.remove(id);
            }
            // 查询该用户的好友信息并返回
            vector<User> uservec = _friendModel.query(id);
            if (!uservec.empty())
            {
                vector<string> vec2;
                for (User &user : uservec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                responce["friends"] = vec2;
            }
            conn->send(responce.dump());
        }
    }
    else
    {
        // 登录失败
        json responce;
        responce["msgid"] = LOGIN_MSG_ACK;
        responce["errno"] = 1;
        responce["errmsg"] = "用户名或者密码错误";
        conn->send(responce.dump());
    }
}

// 处理注册业务  name password
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = _usermodel.insert(user);
    if (state)
    {
        // 注册成功
        json responce;
        responce["msgid"] = REG_MSG_ACK;
        responce["errno"] = 0;
        responce["id"] = user.getId();
        conn->send(responce.dump());
    }
    else
    {
        // 注册失败
        json responce;
        responce["msgid"] = REG_MSG_ACK;
        responce["errno"] = 1;
        conn->send(responce.dump());
    }
}

// 处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnectionMap.begin(); it != _userConnectionMap.end(); ++it)
        {
            if (it->second == conn)
            {
                // 从map表删除用户的连接信息
                user.setId(it->first);
                _userConnectionMap.erase(it);
                break;
            }
        }
    }
    // 更新用户的状态信息
    if (user.getId() != -1)
    {
        user.setState("offline");
        _usermodel.updateState(user);
    }
}

// 一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["toid"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnectionMap.find(toid);
        if (it != _userConnectionMap.end())
        {
            // toid在线转发消息     服务器主动推送消息给toid用户
            it->second->send(js.dump());
            return;
        }
    }
    // toid不在线，存储离线消息
    _offlineMsgModel.insert(toid, js.dump());
}

// 添加好友业务
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息
    _friendModel.insert(userid, friendid);
}

// 创建群聊业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 存储新创建的群组信息
    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        // 存储群组创建人的信息
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}
// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"];
    _groupModel.addGroup(userid, groupid, "normal");
}
// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);
    lock_guard<mutex> lock(_connMutex);
    for (int id : useridVec)
    {
        auto it = _userConnectionMap.find(id);
        if (it != _userConnectionMap.end())
        {
            it->second->send(js.dump());
        }
        else
        {
            _offlineMsgModel.insert(id, js.dump());
        }
    }
}