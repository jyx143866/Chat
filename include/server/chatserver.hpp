/***
 * @Author: jyx
 * @Date: 2024-11-13 15:02:51
 * @LastEditors: jyx
 * @Description:
 */
#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
using namespace muduo::net;
using namespace muduo;

// 聊天服务器的主类
class ChatServer
{
public:
    // 初始化聊天服务器对象
    ChatServer(EventLoop *loop, const InetAddress &listenAddr, const string &nameArg);
    // 启动服务
    void start();

private:
    // 上报链接相关信息的回调函数
    void onConnection(const TcpConnectionPtr &conn);

    // 上报读写事件相关信息的回调函数
    void onMessage(const TcpConnectionPtr &conn, Buffer *buffer, Timestamp time);

    TcpServer _server; // 组合的muduo库，实现服务器功能的类对象
    EventLoop *_loop;  // 指向事件循环的对象指针
};

#endif