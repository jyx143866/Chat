/***
 * @Author: jyx
 * @Date: 2024-11-14 05:05:50
 * @LastEditors: jyx
 * @Description:
 */
#ifndef PUBLIC_H
#define PUBLIC_H

/*
server和client的公共文件
*/
enum EnMsgType
{
    LOGIN_MSG = 1,        // 登录消息
    LOGIN_MSG_ACK = 2,    // 登录响应消息
    REG_MSG = 3,          // 注册消息
    REG_MSG_ACK = 4,      // 注册相应消息
    ONE_CHAT_MSG = 5,     // 聊天消息
    ADD_FRIEND_MSG = 6,   // 添加好友消息
    CREATE_GROUP_MSG = 7, // 创建群聊
    ADD_GROUP_MSG = 8,    // 加入群聊
    GROUP_CHAT_MSG = 9,    // 群聊天
    LOGINOUT_MSG
};

#endif