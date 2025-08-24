#include "rabbitmq.h"

#include <amqpcpp.h>
#include <amqpcpp/libev.h>

using namespace im;

MQClient::MQClient(const std::string &user, std::string password, std::string host)
{
    _loop = EV_DEFAULT;
    _handler = std::make_unique<AMQP::LibEvHandler>(_loop);

    std::string url = "amqp://" + user = ":" + password + "@" + host + "/";
    AMQP::Address address(url);

    _connection = std::make_unique<AMQP::TcpConnection>(_handler.get(), address);
}


MQClient::~MQClient()
{
    ev_async_init(&_async_watcher, WatcherCallback);
    ev_async_start(_loop, &_async_watcher);
    ev_async_send(_loop, &_async_watcher);
    _loop_thread.join();
    _loop = nullptr;
}
void MQClient::DeclareComponents(const std::string &exchange, const std::string &queue, const std::string &routing_key,
                                 AMQP::ExchangeType echange_type)
{
}

void MQClient::WatcherCallback(struct ev_loop *loop, ev_async *watcher, int32_t revents)
{
    ev_break(loop, EVBREAK_ALL);
}

