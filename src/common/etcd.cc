//
// Created by 19396 on 25-5-14.
//

#include "etcd.h"
#include "log.hpp"

#include <etcd/Response.hpp>
#include <etcd/Watcher.hpp>
#include <etcd/Client.hpp>
#include <etcd/KeepAlive.hpp>

using namespace im;

 Register::Register(std::string host)
    : _client(std::make_shared<etcd::Client>(host)), _keep_alive(_client->leasekeepalive(3).get()),
      _lease_id(_keep_alive->Lease())
{
    LOG_DEBUG("Register:lease_id: {}", _lease_id);
}

Register::~Register()
{
    _keep_alive->Cancel();
}

bool Register::Registry(const std::string &key, const std::string &value)
{
    auto resp = _client->put(key, value, _lease_id).get();
    if (resp.is_ok() == false)
    {
        LOG_ERROR("注册服务失败: {}-{}", key, value);
        return false;
    }
    return true;
}

Discovery::Discovery(const std::string& host, const std::string& basedir, NotifyCallback put_callback, NotifyCallback del_callback)
    : _put_callback(std::move(put_callback))
    , _delete_callback(std::move(del_callback))
    , _client(std::make_shared<etcd::Client>(host))
{
    auto resp = _client->ls(basedir).get();
    if (resp.is_ok() == false)
    {
        LOG_ERROR("无法查询服务信息: {}", resp.error_message());
    }

    size_t size = resp.keys().size();
    for (size_t i = 0; i < size; ++i)
    {
        if (_put_callback)
            _put_callback(resp.key(i), resp.value(i).as_string());
    }

    auto func = [this](const etcd::Response& resp) {
        this->Callback(resp);
    };

    _watcher = std::make_shared<etcd::Watcher>(*_client.get(), basedir,
        func, true);
}

 Discovery::~Discovery()
{
    _watcher->Cancel();
}

void Discovery::Callback(const etcd::Response &response)
{
    if (response.is_ok() == false)
    {
        LOG_ERROR("收到一个错误的事件通知: {}", response.error_message());
        return;
    }

    for (auto const& ev: response.events())
    {
        if (ev.event_type() == etcd::Event::EventType::PUT)
        {
            if (_put_callback)
                _put_callback(ev.kv().key(), ev.kv().as_string());
            LOG_DEBUG("新增服务: {}-{}", ev.kv().key(), ev.kv().as_string());
        }
        else if (ev.event_type() == etcd::Event::EventType::DELETE_)
        {
            if (_delete_callback)
                _delete_callback(ev.prev_kv().key(), ev.prev_kv().as_string());
            LOG_DEBUG("下线服务: {}-{}", ev.prev_kv().key(), ev.prev_kv().as_string());
        }
    }
}
