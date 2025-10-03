#include "rabbitmq.h"

#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <amqpcpp/linux_tcp/tcpchannel.h>
#include <ev.h>
#include <memory>

#include "log.hpp"

using namespace im;

MQClient::MQClient(const std::string &user, std::string password, std::string host)
{
    _loop = EV_DEFAULT;
    _handler = std::make_unique<AMQP::LibEvHandler>(_loop); // ev_init 

    std::string url = "amqp://" + user + ":" + password + "@" + host + "/";    
    AMQP::Address address(url);

    _connection = std::make_unique<AMQP::TcpConnection>(_handler.get(), address);
    _channel = std::make_unique<AMQP::TcpChannel>(_connection.get());

    ev_async_init(&_async_watcher, WatcherCallback);
    ev_async_start(_loop, &_async_watcher);

    _loop_thread = std::thread([this]() {
        ev_run(_loop, 0);
    });
}


MQClient::~MQClient()
{
    ev_async_send(_loop, &_async_watcher);
    _loop_thread.join();
    _loop = nullptr;
}
void MQClient::DeclareComponents(const std::string &exchange, const std::string &queue, const std::string &routing_key,
                                 AMQP::ExchangeType echange_type)
{
    _channel->declareExchange(exchange, echange_type)
        .onError([](const char *message) {
            LOG_ERROR("声明交换机失败：{}", message);
            exit(0);
        })
        .onSuccess([exchange](){
            LOG_INFO("{} 交换机创建成功！", exchange);
        });

    _channel->declareQueue(queue)
        .onError([](const char *message) {
            LOG_ERROR("声明队列失败：{}", message);
            exit(0);
        })
        .onSuccess([queue](){
            LOG_INFO("{} 队列创建成功！", queue);
        });

    //6. 针对交换机和队列进行绑定
    _channel->bindQueue(exchange, queue, routing_key)
        .onError([exchange, queue](const char *message) {
            LOG_ERROR("{} - {} 绑定失败：", exchange, queue);
            exit(0);
        })
        .onSuccess([exchange, queue, routing_key](){
            LOG_INFO("{} - {} - {} 绑定成功！", exchange, queue, routing_key);
        });
}

bool MQClient::Publish(const std::string& exchange,
            const std::string& msg,
            const std::string& routing_key)
{
    LOG_DEBUG("向交换机 {}-{} 发布消息！", exchange, routing_key);
    bool ret = _channel->publish(exchange, routing_key, msg);
    if (ret == false) {
        LOG_ERROR("{} 发布消息失败：", exchange);
        return false;
    }
    return true;
}

void MQClient::Consume(const std::string& queue, const MessageCallback& callback)
{
    _channel->consume(queue, "consume-tag")  //返回值 DeferredConsumer
        .onReceived([this, callback](const AMQP::Message &message, 
            uint64_t deliveryTag, 
            bool redelivered) {
            callback(message.body(), message.bodySize());
            _channel->ack(deliveryTag);
        })
        .onError([queue](const char *message){
            LOG_ERROR("订阅 {} 队列消息失败: {}", queue, message);
            exit(0);
        });

}

void MQClient::WatcherCallback(struct ev_loop *loop, ev_async *watcher, int32_t revents)
{
    ev_break(loop, EVBREAK_ALL);
}

