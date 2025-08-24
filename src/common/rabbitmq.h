#pragma once

#include <amqpcpp/exchangetype.h>
#include <ev.h>
#include <thread>
#include <functional>
#include <memory>
#include <string>



namespace AMQP
{
    class LibEvHandler;
    class TcpConnection;
    class TcpChannel;
}

namespace im
{
    class MQClient
    {
    public:
        using MessageCallback = std::function<void(const char*, size_t)>;
        using Ptr = std::shared_ptr<MQClient>;

        MQClient(const std::string& user,
            std::string password,
            std::string host);

        ~MQClient();

        void DeclareComponents(const std::string &exchange,
            const std::string &queue,
            const std::string& routing_key = "routing_key",
            AMQP::ExchangeType echange_type = AMQP::ExchangeType::direct);

        bool Publish(const std::string& exchange,
            const std::string& msg,
            const std::string& routing_key = "routing_key");

        void Consume(const std::string& queue, const MessageCallback& callback);

    private:
        static void WatcherCallback(struct ev_loop* loop, ev_async* watcher, int32_t revents);

    private:
        struct ev_async _async_watcher;
        struct ev_loop* _loop;

        std::unique_ptr<AMQP::LibEvHandler> _handler;
        std::unique_ptr<AMQP::TcpConnection> _connection;
        std::unique_ptr<AMQP::TcpChannel> _channel;
        std::thread _loop_thread;
    };
}