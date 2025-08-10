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
class Register
{
public:
    using Ptr = std::shared_ptr<Register>;
    /**
     *
     * @param host 主机地址信息
     */
    explicit Register(std::string host);

    ~Register();

    bool Registry(const std::string& key, const std::string& value);
private:
    std::shared_ptr<etcd::Client> _client;
    std::shared_ptr<etcd::KeepAlive> _keep_alive;
    uint64_t _lease_id;
};

class Discovery
{
public:
    using Ptr = std::shared_ptr<Discovery>;
    using NotifyCallback = std::function<void(std::string, std::string)>;

    explicit Discovery(const std::string& host, const std::string& basedir, NotifyCallback put_callback, NotifyCallback del_callback);

    ~Discovery();

private:
    void Callback(const etcd::Response& response);

private:
    NotifyCallback _put_callback;
    NotifyCallback _delete_callback;
    std::shared_ptr<etcd::Client> _client;
    std::shared_ptr<etcd::Watcher> _watcher;
};

}
