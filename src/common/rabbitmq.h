#pragma once

#include <amqpcpp/exchangetype.h>
#include <ev.h>
#include <functional>
#include <memory>
#include <string>
#include <thread>


namespace AMQP
{
    class LibEvHandler;
    class TcpConnection;
    class TcpChannel;
} // namespace AMQP

namespace im
{
    /**
     * @brief RabbitMQ客户端类，提供消息队列的发布和订阅功能
     */
    class MQClient
    {
    public:
        /**
         * @brief 消息回调函数类型
         * @param data 消息内容
         * @param size 消息大小
         */
        using MessageCallback = std::function<void(const char *, size_t)>;

        /**
         * @brief MQClient智能指针类型别名
         */
        using Ptr = std::shared_ptr<MQClient>;

        /**
         * @param user RabbitMQ用户名
         * @param password RabbitMQ密码
         * @param host RabbitMQ服务器地址
         */
        MQClient(const std::string &user, std::string password, std::string host);

        ~MQClient();

        /**
         * @brief 声明交换器和队列
         * @param exchange 交换器名称
         * @param queue 队列名称
         * @param routing_key 路由键，默认为"routing_key"
         * @param exchange_type 交换器类型，默认为direct
         */
        void DeclareComponents(const std::string &exchange, const std::string &queue,
                               const std::string &routing_key = "routing_key",
                               AMQP::ExchangeType exchange_type = AMQP::ExchangeType::direct);

        /**
         * @brief 发布消息
         * @param exchange 交换器名称
         * @param msg 消息内容
         * @param routing_key 路由键，默认为"routing_key"
         * @return 发布成功返回true，失败返回false
         */
        bool Publish(const std::string &exchange, const std::string &msg,
                     const std::string &routing_key = "routing_key");

        /**
         * @brief 消费消息
         * @param queue 队列名称
         * @param callback 消息处理回调函数
         */
        void Consume(const std::string &queue, const MessageCallback &callback);

    private:
        /**
         * @brief 异步观察器回调函数
         * @param loop 事件循环
         * @param watcher 异步观察器
         * @param revents 事件类型
         */
        static void WatcherCallback(struct ev_loop *loop, ev_async *watcher, int32_t revents);

    private:
        /**
         * @brief 异步观察器，用于处理事件循环中的异步事件
         */
        struct ev_async _async_watcher;

        /**
         * @brief 事件循环指针
         */
        struct ev_loop *_loop;

        /**
         * @brief LibEv事件处理器
         */
        std::unique_ptr<AMQP::LibEvHandler> _handler;

        /**
         * @brief TCP连接对象
         */
        std::unique_ptr<AMQP::TcpConnection> _connection;

        /**
         * @brief TCP通道对象
         */
        std::unique_ptr<AMQP::TcpChannel> _channel;

        /**
         * @brief 事件循环线程
         */
        std::thread _loop_thread;
    };
} // namespace im
