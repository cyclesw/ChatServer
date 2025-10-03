#include "redis.h"
#include "log.hpp"
#include <sw/redis++/redis.h>


std::shared_ptr<sw::redis::Redis> im::RedisClientFactory::Create(const std::string &host, const int port, int db,
                                                                 bool keepAlive)
{
    sw::redis::ConnectionOptions options;
    options.host = host;
    options.port = port;
    options.db = db;
    options.keep_alive = keepAlive;
    auto res = std::make_shared<sw::redis::Redis>(options);

    return res;
}

im::Session::Session(const std::shared_ptr<sw::redis::Redis> &redis_client) : _client(redis_client)
{
}
void im::Session::Append(const std::string &ssid, const std::string &uid) const
{
    LOG_TRACE("{} 登录成功，关联用户 {}", ssid, uid);
    _client->set(ssid, uid);
}
void im::Session::Remove(const std::string &ssid) const
{
    LOG_TRACE("{} 登出成功，解除用户关联 {}", ssid, _client->get(ssid).value());
    _client->del(ssid);
}
sw::redis::OptionalString im::Session::Uid(const std::string &ssid) const
{
    return _client->get(ssid);
}
im::Status::Status(const std::shared_ptr<sw::redis::Redis> &redis_client) : _redis_client(redis_client)
{
}
void im::Status::Append(const std::string &uid)
{
    LOG_TRACE("{} 上线成功", uid);
    _redis_client->set(uid, "");
}
void im::Status::Remove(const std::string &uid)
{
    LOG_TRACE("{} 下线成功", uid);
    _redis_client->del(uid);
}
bool im::Status::Exists(const std::string &uid) const
{
    auto res = _redis_client->get(uid);
    if (res)
        return true;

    return false;
}
im::Codes::Codes(const std::shared_ptr<sw::redis::Redis> &redisClient) : _redis_client(redisClient)
{
}
void im::Codes::Append(const std::string &cid, const std::string &code, const std::chrono::milliseconds &t)
{
    LOG_TRACE("{} 验证码 {} 有效期 {} 秒", cid, code, t.count());
    _redis_client->set(cid, code, t);
}
void im::Codes::Remove(const std::string &cid)
{
    LOG_TRACE("{} 验证码已失效", cid);
    _redis_client->del(cid);
}
sw::redis::OptionalString im::Codes::Code(const std::string &cid) const
{
    return _redis_client->get(cid);
}
