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
 *
 * ServiceChannel 类封装了 brpc::Channel，提供了一种简单的方式来管理
 * 服务的多个主机实例。它实现了负载均衡功能，通过轮询方式选择可用的通道。
 */
class ServiceChannel
{
public:
    // 使用智能指针定义类型别名
    using Ptr = std::shared_ptr<ServiceChannel>;  // ServiceChannel的智能指针类型
    using ChannelPtr = std::shared_ptr<brpc::Channel>;  // brpc::Channel的智能指针类型

    // 构造函数，初始化服务通道
    ServiceChannel(const std::string &name);

    // 添加一个新的主机通道
    void Append(const std::string& host);

    // 移除指定主机的通道
    void Remove(const std::string& host);

    // 选择一个可用的通道（使用轮询算法）
    ChannelPtr Choose();

private:
    int32_t _index;  // 轮询计数器，用于选择通道
    std::mutex _mutex;  // 互斥锁，保证线程安全
    std::string _service_name;  // 服务名称
    std::vector<ChannelPtr> _channels;  // 存储所有可用通道的列表
    std::unordered_map<std::string, ChannelPtr> _hosts;  // 主机地址到通道的映射
};

/*!
 * @brief ServiceManager 管理所有服务通道
56  *
57  * ServiceManager 负责创建、管理和维护多个服务的通道实例。
58  * 它提供了服务发现的功能，能够处理服务的上线和下线事件。
*/
class ServiceManager
{
public:
    /**
     * @brief ServiceManager的智能指针类型别名
     */
    using Ptr = std::shared_ptr<ServiceManager>;

    /**
     * @brief 构造函数，初始化服务管理器
     */
    ServiceManager();

    /**
     * @brief 选择指定服务的可用通道
     * @param service_name 服务名称
     * @return 可用的服务通道，如果不存在则返回nullptr
     */
    ServiceChannel::ChannelPtr Choose(const std::string &service_name);

    /**
     * @brief 声明需要关注的服务
     * @param service_name 服务名称
     */
    void Declared(const std::string& service_name);

    /**
     * @brief 处理服务上线事件
     * @param service_instance 服务实例名称
 89  *      @param host 服务主机地址
     */
    void OnServiceOnline(const std::string &service_instance, const std::string &host);

    /**
     * @brief 处理服务下线事件
     * @param service_instance 服务实例名称
     * @param host 服务主机地址
     */
    void OnServiceOffline(const std::string& service_instance,
                const std::string& host);

private:
    /**
     * @brief 从服务实例名称中提取服务名称
     * @param service_instance 服务实例名称
     * @return 服务名称
     */
    static std::string GetServiceName(const std::string& service_instance);

private:
    std::mutex _mutex;  ///< 互斥锁，保证线程安全
    std::unordered_set<std::string> _follow_service;  ///< 需要关注的服务集合
    std::unordered_map<std::string, ServiceChannel::Ptr> _services;  ///< 服务名称到服务通道的映射
};

} // namespace gateway
