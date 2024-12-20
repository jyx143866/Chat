/***
 * @Author: jyx
 * @Date: 2024-11-24 05:39:37
 * @LastEditors: jyx
 * @Description:
 */
#include "redis.hpp"

Redis::Redis()
    : _publish_context(nullptr), _subscribe_context(nullptr)
{
}

Redis::~Redis()
{
    if (_publish_context != nullptr)
    {
        redisFree(_publish_context);
    }
    if (_subscribe_context != nullptr)
    {
        redisFree(_subscribe_context);
    }
}

// 连接redis服务器
bool Redis::connect()
{
    // 负责publish发布消息的上下文连接
    _publish_context = redisConnect("127.0.0.1", 6379);
    if (_publish_context == nullptr)
    {
        cerr << "connect redis failed !" << endl;
        return false;
    }

    // 负责subscribe订阅消息的上下文链接
    _subscribe_context = redisConnect("127.0.0.1", 6379);
    if (_subscribe_context == nullptr)
    {
        cerr << "connect redis failed !" << endl;
        return false;
    }

    // 在单独的线程中，监听通道上的事件，有消息给业务层上报
    thread t([&]() -> void
             { observer_channel_message(); });
    t.detach();
    cout << "connect redis-server success!" << endl;
    return true;
}

// 向redis指定的通道publish发布消息
bool Redis::publish(int channel, string message)
{
    redisReply *reply = (redisReply *)redisCommand(_publish_context, "PUBLISH %d %s", channel, message);
    if (reply == nullptr)
    {
        cerr << "publish command failed!" << endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

// 向redis指定的通道subscribe订阅消息
bool Redis::subscribe(int channel)
{
    if(REDIS_ERR == redisAppendCommand(this->_subscribe_context, "SUBSCREIBE %d", channel))
    {
        cerr << "subscribe command invalid!" << endl;
        return false;
    }
    int done = 0;
    while(!done)
    {
        if(REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            cerr << "subcribe command failed!" << endl;
        }
    }
    return true;
}
// 向redis指定的通道ubsubscribe取消订阅消息
bool Redis::unsubscribe(int channel)
{
    if (REDIS_ERR == redisAppendCommand(this->_subscribe_context, "UNSUBSCREIBE %d", channel))
    {
        cerr << "unsubscribe command invalid!" << endl;
        return false;
    }
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            cerr << "unsubcribe command failed!" << endl;
        }
    }
    return true;
}
// 在独立线程中接收订阅通道中的消息
void Redis::observer_channel_message()
{
    redisReply * reply = nullptr;
    while(REDIS_OK == redisGetReply(this->_subscribe_context, (void **)&reply))
    {
        // 订阅收到的是一个带三元素的数组
        if(reply != nullptr && reply->element[2] != nullptr &&reply->element[2]->str != nullptr)
        {
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    cerr << ">>>>>>>>>>>>>>>>>> observer_channel_message quit<<<<<<<<<<<<<<" << endl;
}
// 初始化业务层上报通道消息的回调对象
void Redis::init_notify_handler(function<void(int, string)> fn)
{
    this->_notify_message_handler = fn;
}