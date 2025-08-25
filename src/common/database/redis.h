//
// Created by 19396 on 25-5-14.
//

#ifndef REDIS_H
#define REDIS_H

#include <chrono>
#include <memory>
#include <optional>

/*!
 * 因为 brpc 和 hiredis 中 枚举重名，为了避免冲突，将 redis 头文件放到.cc中
 */
namespace sw::redis
{
    using OptionalString = std::optional<std::string>;
    class Redis;
}
namespace im
{

class RedisClientFactory
{
public:
    static std::shared_ptr<sw::redis::Redis> Create(
        const std::string& host,
        const int port,
        int db,
        bool keepAlive);
};

class Session
{
public:
    using Ptr = std::shared_ptr<Session>;

    explicit Session(const std::shared_ptr<sw::redis::Redis>& redis_client);
    void Append(const std::string& ssid, const std::string& uid) const;

    void Remove(const std::string& ssid) const;

    [[nodiscard]] sw::redis::OptionalString Uid(const std::string& ssid) const;

private:
    std::shared_ptr<sw::redis::Redis> _client;
};

class   Status
{
public:
    using Ptr = std::shared_ptr<Status>;

    explicit Status(const std::shared_ptr<sw::redis::Redis>& redis_client);

    void Append(const std::string& uid);

    void Remove(const std::string& uid);

    [[nodiscard]] bool Exists(const std::string& uid) const;

private:
    std::shared_ptr<sw::redis::Redis> _redis_client;
};

class Codes
{
public:
    using Ptr = std::shared_ptr<Codes>;
    explicit Codes(const std::shared_ptr<sw::redis::Redis>& redisClient);

    void Append(const std::string& cid, const std::string& code,
        const std::chrono::milliseconds& t = std::chrono::milliseconds(300000));

    void Remove(const std::string& cid);

    [[nodiscard]] sw::redis::OptionalString Code(const std::string& cid) const;

private:
    std::shared_ptr<sw::redis::Redis> _redis_client;
};


}

#endif //REDIS_H
