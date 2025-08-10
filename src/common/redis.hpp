//
// Created by 19396 on 25-5-14.
//

#ifndef REDIS_H
#define REDIS_H

#include <memory>
#include <sw/redis++/redis.h>



namespace im
{

class RedisClientFactory
{
public:
    static std::shared_ptr<sw::redis::Redis> Create(
        const std::string& host,
        const int port,
        int db,
        bool keepAlive)
    {
        sw::redis::ConnectionOptions options;
        options.host = host;
        options.port = port;
        options.db   = db;
        options.keep_alive = keepAlive;
        auto res = std::make_shared<sw::redis::Redis>(options);

        return res;
    }
};

class Session
{
public:
    using Ptr = std::shared_ptr<Session>;

    explicit Session(const std::shared_ptr<sw::redis::Redis>& redis_client)
        :_client(redis_client) {}
    void Append(const std::string& ssid, const std::string& uid)
    {
        _client->set(ssid, uid);
    }

    void Remove(const std::string& ssid)
    {
        _client->del(ssid);
    }

    [[nodiscard]] sw::redis::OptionalString Uid(const std::string& ssid) const
    {
        return _client->get(ssid);
    }

private:
    std::shared_ptr<sw::redis::Redis> _client;
};

class   Status
{
public:
    using Ptr = std::shared_ptr<Status>;

    Status(const std::shared_ptr<sw::redis::Redis>& redis_client)
        :_redis_client(redis_client) {}

    void Append(const std::string& uid)
    {
        _redis_client->set(uid, "");
    }

    void Remove(const std::string& uid)
    {
        _redis_client->del(uid);
    }

    [[nodiscard]] bool Exists(const std::string& uid) const
    {
        auto res = _redis_client->get(uid);
        if (res)
            return true;

        return false;
    }

private:
    std::shared_ptr<sw::redis::Redis> _redis_client;
};

class Codes
{
public:
    using Ptr = std::shared_ptr<Codes>;
    explicit Codes(const std::shared_ptr<sw::redis::Redis>& redisClient)
        :_redis_client(redisClient){}

    void Append(const std::string& cid, const std::string& code,
        const std::chrono::milliseconds& t = std::chrono::milliseconds(300000))
    {
        _redis_client->set(cid, code, t);
    }

    void Remove(const std::string& cid)
    {
        _redis_client->del(cid);
    }

    [[nodiscard]] sw::redis::OptionalString Code(const std::string& cid) const
    {
        return _redis_client->get(cid);
    }
private:
    std::shared_ptr<sw::redis::Redis> _redis_client;
};


}

#endif //REDIS_H
