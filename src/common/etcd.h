#pragma once

#include <functional>
#include <memory>
#include <cstdint>


namespace etcd
{
class Client;
class KeepAlive;
class Watcher;
class Response;
}

namespace im
{
class Register  // 服务注册类，用于将服务注册到etcd并保持心跳
{
public:
    using Ptr = std::shared_ptr<Register>;  // Register类的智能指针类型别名
    /**
     * @brief 构造函数
     * @param host etcd服务器的主机地址信息
     */
    explicit Register(std::string host);

    ~Register();  // 析构函数，用于清理资源

    /**
     * @brief 注册服务到etcd
     * @param key 注册的键名
33  * @param value 注册的键值
34  * @return 注册是否成功
     */
    bool Registry(const std::string& key, const std::string& value);
private:
    std::shared_ptr<etcd::Client> _client;  // etcd客户端指针，用于与etcd服务器通信
    std::shared_ptr<etcd::KeepAlive> _keep_alive;  // 保持连接的指针，用于维持租约
    uint64_t _lease_id;  // 租约ID，用于标识服务的租约
};

class Discovery  // 服务发现类，用于发现和监听etcd中的服务变化
{
public:
    using Ptr = std::shared_ptr<Discovery>;  // Discovery类的智能指针类型别名
    using NotifyCallback = std::function<void(std::string, std::string)>;  // 通知回调函数类型，用于处理服务变化

    /**
     * @brief 构造函数
     * @param host etcd服务器的主机地址
     * @param basedir 服务发现的基础目录
     * @param put_callback 服务添加时的回调函数
     * @param del_callback 服务删除时的回调函数
     */
    explicit Discovery(const std::string& host, const std::string& basedir, NotifyCallback put_callback, NotifyCallback del_callback);

    ~Discovery();  // 析构函数，用于清理资源

private:
    /**
     * @brief etcd响应回调函数
     * @param response etcd服务器的响应
     */
    void Callback(const etcd::Response& response);

private:
    NotifyCallback _put_callback;  // 服务添加时的回调函数
    NotifyCallback _delete_callback;  // 服务删除时的回调函数
    std::shared_ptr<etcd::Client> _client;  // etcd客户端指针，用于与etcd服务器通信
    std::shared_ptr<etcd::Watcher> _watcher;  // 观察者指针，用于监听服务变化
};

}
