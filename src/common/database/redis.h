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

/**
 * @brief Redis客户端工厂类，用于创建Redis客户端实例
 */
class RedisClientFactory
{
public:
    /**
     * @brief 创建Redis客户端实例
     * @param host Redis服务器地址
     * @param port Redis服务器端口
     * @param db Redis数据库编号
     * @param keepAlive 是否保持连接
     * @return 返回Redis客户端的共享指针
     */
    static std::shared_ptr<sw::redis::Redis> Create(
        const std::string& host,
        const int port,
        int db,
        bool keepAlive);
};

/**
 * @brief 会话管理类，用于管理用户会话信息
 */
class Session
{
public:
    using Ptr = std::shared_ptr<Session>;

    /**
     * @brief 构造函数
     * @param redis_client Redis客户端实例
     */
    explicit Session(const std::shared_ptr<sw::redis::Redis>& redis_client);

    /**
     * @brief 添加会话信息
     * @param ssid 会话ID
     * @param uid 用户ID
     */
    void Append(const std::string& ssid, const std::string& uid) const;

    /**
     * @brief 移除会话信息
     * @param ssid 会话ID
     */
    void Remove(const std::string& ssid) const;

    /**
     * @brief 获取用户ID
     * @param ssid 会话ID
     * @return 返回对应的用户ID，如果不存在则返回空
     */
    [[nodiscard]] sw::redis::OptionalString Uid(const std::string& ssid) const;

private:
    std::shared_ptr<sw::redis::Redis> _client;
};

/**
 * @brief 用户状态管理类，用于管理用户在线状态
 */
class Status
{
public:
    using Ptr = std::shared_ptr<Status>;

    /**
     * @brief 构造函数
     * @param redis_client Redis客户端实例
     */
    explicit Status(const std::shared_ptr<sw::redis::Redis>& redis_client);

    /**
     * @brief 添加用户状态
     * @param uid 用户ID
     */
    void Append(const std::string& uid);

    /**
     * @brief 移除用户状态
     * @param uid 用户ID
     */
    void Remove(const std::string& uid);

    /**
     * @brief 检查用户状态是否存在
     * @param uid 用户ID
     * @return 返回用户状态是否存在
     */
    [[nodiscard]] bool Exists(const std::string& uid) const;

private:
    std::shared_ptr<sw::redis::Redis> _redis_client;
};

/**
 * @brief 验证码管理类，用于管理用户验证码
 */
class Codes
{
public:
    using Ptr = std::shared_ptr<Codes>;
    /**
     * @brief 构造函数
     * @param redisClient Redis客户端实例
     */
    explicit Codes(const std::shared_ptr<sw::redis::Redis>& redisClient);

    /**
     * @brief 添加验证码
     * @param cid 验证码ID
     * @param code 验证码内容
     * @param t 有效时间，默认为300秒
     */
    void Append(const std::string& cid, const std::string& code,
        const std::chrono::milliseconds& t = std::chrono::milliseconds(300000));

    /**
     * @brief 移除验证码
     * @param cid 验证码ID
     */
    void Remove(const std::string& cid);

    /**
     * @brief 获取验证码
     * @param cid 验证码ID
     * @return 返回对应的验证码，如果不存在则返回空
     */
    [[nodiscard]] sw::redis::OptionalString Code(const std::string& cid) const;

private:
    std::shared_ptr<sw::redis::Redis> _redis_client;
};


}

#endif //REDIS_H
