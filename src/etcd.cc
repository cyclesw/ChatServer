//
// Created by lang liu on 2024/9/2.
//

#include "etcd.h"
#include "etcd.h"

#include <etcd/Client.hpp>
#include <etcd/KeepAlive.hpp>
#include <etcd/Watcher.hpp>
#include "log.hpp"

using namespace ns_chat;

Registry::Registry(const std::string &hostname)
    :_client(std::make_shared<etcd::Client>(hostname))
    ,_keepAlive(_client->leasekeepalive(3).get())
    ,_leaseId(_keepAlive->Lease())
{}

Registry::~Registry()
{
    _keepAlive->Cancel();
}

bool Registry::RegistryServer(const std::string &key, const std::string &value)
{
    auto resp = _client->put(key, value, _leaseId).get();
    if (!resp.is_ok())
    {
        LOG_ERROR("注册数据失败[{}]: {}", resp.error_code() ,resp.error_message());
        return false;
    }
    return true;
}

Discovery::Discovery(const std::string &hostname, const std::string& basedir,
    const NotifyCallback &put_callback,
    const NotifyCallback &del_callback)
        :_client(std::make_shared<etcd::Client>(hostname)),
         _put_callback(put_callback),
         _del_callback(del_callback)
{
    auto resp = _client->ls(basedir).get();
    if (resp.is_ok() == false)
    {
        LOG_ERROR("获取服务器信息数据失败[{}]: {}", resp.error_code(), resp.error_message());
    }

    size_t sz = resp.keys().size();
    for (int i = 0; i < sz; ++i)
    {
        if (_put_callback)
            _put_callback(resp.keys()[i], resp.value(i).as_string());
    }

    _watcher = std::make_shared<etcd::Watcher>(*_client, basedir,
        std::bind(&Discovery::Callback, this, std::placeholders::_1), true);
}

Discovery::~Discovery()
{
    _watcher->Cancel();
}

void Discovery::Callback(const etcd::Response &resp)
{
    if (resp.is_ok() == false)
    {
        LOG_ERROR("收到错误信息通知[{}]:{}", resp.error_code(), resp.error_message());
        return;
    }

    for (auto const& ev : resp.events())
    {
        if (ev.event_type() == etcd::Event::EventType::PUT)
        {
            if (_put_callback)
                _put_callback(ev.kv().key(), ev.kv().as_string());
            LOG_DEBUG("新增服务: {}-{}", ev.kv().key(), ev.kv().as_string());
        }
        else if (ev.event_type() == etcd::Event::EventType::DELETE_)
        {
            if (_del_callback)
                _del_callback(ev.kv().key(), ev.kv().as_string());
            LOG_DEBUG("下线服务: {}-{}", ev.prev_kv().key(), ev.prev_kv().as_string());
        }
    }
}

