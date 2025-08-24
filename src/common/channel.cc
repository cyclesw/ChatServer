//
// Created by 19396 on 25-5-12.
//

#include "channel.h"
#include "log.hpp"
#include <brpc/channel.h>
#include <brpc/options.pb.h>
#include <cstdint>
#include <memory>
#include <mutex>

using namespace im;

ServiceChannel::ServiceChannel(const std::string &name) : _index(0), _service_name(name) {};

void ServiceChannel::Append(const std::string &host)
{
    auto channel = std::make_shared<brpc::Channel>();
    brpc::ChannelOptions options;
    options.protocol = brpc::PROTOCOL_BAIDU_STD;
    options.timeout_ms = -1;
    options.max_retry = 3;
    options.connect_timeout_ms = -1;
    int ret = channel->Init(host.c_str(), &options);
    if (ret != 0)
    {
        LOG_ERROR("初始化{}-{}信道失败", _service_name, host);
        return;
    }

    std::lock_guard lock(_mutex);
    _hosts.emplace(host, channel);
    _channels.emplace_back(channel);
}

void ServiceChannel::Remove(const std::string &host)
{
    std::lock_guard lock(_mutex);
    auto it = _hosts.find(host);
    if (it == _hosts.end())
    {
        LOG_WARN("{}-{}节点删除信道时，没有找到信道信息！", _service_name, host);
        return;
    }

    for (auto vit = _channels.begin(); vit != _channels.end(); ++vit)
    {
        if (it->second == *vit)
        {
            _channels.erase(vit);
            break;
        }
    }
    _hosts.erase(it);
}

ServiceChannel::ChannelPtr ServiceChannel::Choose()
{
    std::lock_guard lock(_mutex);
    if (_channels.empty())
    {
        LOG_ERROR("{}没有可用的信道", _service_name);
        return nullptr;
    }

    int32_t idx = _index++ % _channels.size();
    return _channels[idx];
}

ServiceManager::ServiceManager() = default;

ServiceChannel::ChannelPtr ServiceManager::Choose(const std::string &service_name)
{
    std::lock_guard lock(_mutex);
    auto it = _services.find(service_name);
    if (it == _services.end())
    {
        LOG_ERROR("当前没有能够提供 {} 服务的节点！", service_name);
        return nullptr;
    }

    return it->second->Choose();
}

void ServiceManager::Declared(const std::string &service_name)
{
    std::lock_guard lock(_mutex);
    if (_follow_service.find(service_name) != _follow_service.end())
        return;

    auto channel = std::make_shared<ServiceChannel>(service_name);
    _services.emplace(service_name, channel);
    _follow_service.emplace(service_name);
}

void ServiceManager::OnServiceOnline(const std::string &service_instance, const std::string &host)
{
    std::string service_name = GetServiceName(service_instance);
    ServiceChannel::Ptr service;
    {
        std::lock_guard lock(_mutex);
        auto fit = _follow_service.find(service_name);
        if (fit == _follow_service.end())
        {
            LOG_DEBUG("{}-{} 服务上线了，但是当前并不关心！", service_name, host);
            return;
        }
        auto sit = _services.find(service_name);
        if (sit == _services.end())
        {
            service = std::make_shared<ServiceChannel>(service_name);
            _services.insert(std::make_pair(service_name, service));
        }
        else
        {
            service = sit->second;
        }
    }

    if (!service)
    {
        LOG_ERROR("新增 {} 服务管理节点失败！", service_name);
        return ;
    }
    service->Append(host);
    LOG_DEBUG("{}-{} 服务上线新节点，进行添加管理！", service_name, host);
}

void ServiceManager::OnServiceOffline(const std::string &service_instance, const std::string &host)
{
    std::string service_name = GetServiceName(service_instance);
    ServiceChannel::Ptr service;
    {
        std::lock_guard lock(_mutex);
        auto fit = _follow_service.find(service_name);
        if (fit == _follow_service.end())
        {
            LOG_DEBUG("{}-{} 服务下线了，但是当前并不关心！", service_name, host);
            return;
        }
        auto sit = _services.find(service_name);
        if (sit == _services.end())
        {
            LOG_WARN("删除{}服务节点时，没有找到管理对象", service_name);
            return;
        }
        service = sit->second;
    }

    service->Append(host);
    LOG_DEBUG("{}-{} 服务下线节点，进行删除管理！", service_name, host);
}

std::string ServiceManager::GetServiceName(const std::string &service_instance)
{
    auto pos = service_instance.find_last_of('/');
    if (pos == std::string::npos)
        return service_instance;
    return service_instance.substr(0, pos);
}