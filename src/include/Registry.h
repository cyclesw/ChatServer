//
// Created by lang liu on 2024/9/2.
//

#ifndef REGISTRY_H
#define REGISTRY_H

#include <etcd/Client.hpp>
#include <etcd/KeepAlive.hpp>

class Registry {
public:
    explicit Registry(const std::string& hostname);

    ~Registry();

    bool RegistryServer(const std::string& key, const std::string& value);
private:
    std::shared_ptr<etcd::Client> _client;
    std::shared_ptr<etcd::KeepAlive> _keepAlive;
    std::int64_t _leaseId {};
};

class Discovery
{
public:
    using NotifyCallback = std::function<void(std::string, std::string)>;

    Discovery(const std::string& hostname, const NotifyCallback& put_callback,
        const NotifyCallback& del_callback);

    ~Discovery();

private:
    void Callback(const etcd::Response& resp);

    NotifyCallback _put_callback;
    NotifyCallback _del_callback;
    std::shared_ptr<etcd::Client> _client;
    std::shared_ptr<etcd::Watcher> _watcher;
};


#endif //REGISTRY_H
