//
// Created by 19396 on 25-5-12.
//

#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace brpc
{
    class Channel;
}

namespace im
{
/*!
 * @brief ServiceChannel is a wrapper for brpc::Channel.
*/
class ServiceChannel
{
public:
    using Ptr = std::shared_ptr<ServiceChannel>;
    using ChannelPtr = std::shared_ptr<brpc::Channel>;

    ServiceChannel(const std::string &name);

    void Append(const std::string& host);

    void Remove(const std::string& host);

    ChannelPtr Choose();

private:
    int32_t _index;
    std::mutex _mutex;
    std::string _service_name;
    std::vector<ChannelPtr> _channels;
    std::unordered_map<std::string, ChannelPtr> _hosts;
};

class ServiceManager
{
public:
    using Ptr = std::shared_ptr<ServiceManager>;

    ServiceManager();
    ServiceChannel::ChannelPtr Choose(const std::string &service_name);

    void Declared(const std::string& service_name);
    void OnServiceOnline(const std::string &service_instance, const std::string &host);
    void OnServiceOffline(const std::string& service_instance, 
                const std::string& host);
    
private:
    static std::string GetServiceName(const std::string& service_instance);
    
private:
    std::mutex _mutex;
    std::unordered_set<std::string> _follow_service;
    std::unordered_map<std::string, ServiceChannel::Ptr> _services;
};

} // namespace gateway
