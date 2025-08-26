#pragma once

#include "base.pb.h"
#include "etcd.h"
#include "rabbitmq.h"
#include "channel.h"
#include "database/mysql.hpp"
#include "database/mysql_chat_session_member.h"

#include "transmit.pb.h"
#include "user.pb.h"

#include "log.hpp"
#include "utils.h"

#include <brpc/closure_guard.h>
#include <brpc/channel.h>
#include <brpc/controller.h>
#include <brpc/server.h>
#include <memory>

namespace im
{

    class TransmitServiceImpl: public im::MsgTransmitService
    {
    public:
        TransmitServiceImpl(const std::string &user_service_name,
                const std::shared_ptr<ServiceManager> &channels,
                const std::shared_ptr<odb::core::database> &mysql_client,
                const std::string &exchange_name,
                const std::string &routing_key,
                const std::shared_ptr<MQClient> &mq_client)
            :_user_service_name(user_service_name),
             _channels(channels),
             _mysql_session_member_table(std::make_shared<ChatSessionMemberTable>(mysql_client)),
             _exchange_name(exchange_name),
             _routing_key(routing_key),
             _mq_client(mq_client)
        {   }
            
        ~TransmitServiceImpl() override = default;
        void GetTransmitTarget(google::protobuf::RpcController *controller, const im::NewMessageRequest *request,
                               im::GetTransmitTargetResponse *response, google::protobuf::Closure *done) override
        {
            brpc::ClosureGuard rpc_guard(done);
            auto err_response = [this, response](const std::string& rid, 
                    const std::string& errmsg) -> void {
                response->set_request_id(rid);
                response->set_success(false);
                response->set_error(errmsg);
                return;
            };
                        //从请求中获取关键信息：用户ID，所属会话ID，消息内容
            std::string rid = request->request_id();
            std::string uid = request->user_id();
            std::string chat_ssid = request->chat_session_id();
            const MessageContent &content = request->message();

            LOG_TRACE("接收到消息传输请求，请求ID: {}, 用户ID: {}, 会话ID: {}", rid, uid, chat_ssid);

            // 进行消息组织：发送者-用户子服务获取信息，所属会话，消息内容，产生时间，消息ID
            auto channel = _channels->Choose(_user_service_name);
            if (!channel) {
                LOG_ERROR("{}-{} 没有可供访问的用户子服务节点！", rid, _user_service_name);
                return err_response(rid, "没有可供访问的用户子服务节点！");
            }
            UserService_Stub stub(channel.get());
            GetUserInfoRequest user_request;
            GetUserInfoResponse user_response;
            user_request.set_request_id(rid);
            user_request.set_user_id(uid);
            brpc::Controller cntl;
            stub.GetUserInfo(&cntl, &user_request, &user_response, nullptr);

            if (cntl.Failed()) {
                LOG_ERROR("{} - 获取用户信息失败: {}!", rid, cntl.ErrorText());
                return err_response(rid, "获取用户信息失败!");
            }

            MessageInfo message;
            message.set_message_id(Uuid());
            message.set_chat_session_id(chat_ssid);
            message.set_timestamp(time(nullptr));
            message.mutable_sender()->CopyFrom(user_response.user_info());
            message.mutable_message()->CopyFrom(content);

            LOG_TRACE("消息组织完成，消息ID: {}", message.message_id());

            // 获取消息妆发客户端用户列表
            auto target_list = _mysql_session_member_table->Members(chat_ssid);
            bool ret = _mq_client->Publish(_exchange_name, message.SerializeAsString(), _routing_key);
            if (!ret)
            {
                LOG_ERROR("{} - 持久化消息发布失败: {}!", request->request_id(), cntl.ErrorText());
                return err_response(request->request_id(), "持久化消息发布失败!");
            }

            LOG_TRACE("消息发布成功，目标会话ID: {}, 目标用户数量: {}", chat_ssid, target_list.size());

            response->set_request_id(rid);
            response->set_success(true);
            response->mutable_message()->CopyFrom(message);
            for (auto &target : target_list)
            {
                response->add_target_id_list(target);
            }
        }
    private:
        std::string _user_service_name;
        std::shared_ptr<ServiceManager> _channels;

        std::shared_ptr<ChatSessionMemberTable> _mysql_session_member_table;

        std::string _exchange_name;
        std::string _routing_key;
        std::shared_ptr<MQClient> _mq_client;
    };

    class TransmitServer 
    {
    public:
        using Ptr = std::shared_ptr<TransmitServer>;

        TransmitServer(
                const std::shared_ptr<odb::core::database>& mysql_client,
                const Discovery::Ptr& discovery_client,
                const Register::Ptr&  registry_host,
                const std::shared_ptr<brpc::Server>& server)
            :_service_discoverer(discovery_client),
             _registry_client(registry_host),
             _mysql_client(mysql_client),
             _rpc_server(server)
            {
            }

            ~TransmitServer() = default;

            void Start()
            {
                _rpc_server->RunUntilAskedToQuit();
            }
    private:
        Discovery::Ptr _service_discoverer;
        Register::Ptr  _registry_client;
        std::shared_ptr<odb::core::database> _mysql_client;
        std::shared_ptr<brpc::Server> _rpc_server;
    };

    class TransmiteServerBuilder {
    public:
        //构造mysql客户端对象
        void make_mysql_object(
            const std::string &user,
            const std::string &pswd,
            const std::string &host,
            const std::string &db,
            const std::string &cset,
            int port,
            int conn_pool_count) {
            _mysql_client = ODBFactory::Create(user, pswd, host, db, cset, port, conn_pool_count);
        }
        //用于构造服务发现客户端&信道管理对象
        void make_discovery_object(const std::string &reg_host,
            const std::string &base_service_name,
            const std::string &user_service_name) {
            _user_service_name = user_service_name;
            _mm_channels = std::make_shared<ServiceManager>();
            _mm_channels->Declared(user_service_name);
            LOG_DEBUG("设置用户子服务为需添加管理的子服务：{}", user_service_name);
            auto put_cb = std::bind(&ServiceManager::OnServiceOnline, _mm_channels.get(), std::placeholders::_1, std::placeholders::_2);
            auto del_cb = std::bind(&ServiceManager::OnServiceOffline, _mm_channels.get(), std::placeholders::_1, std::placeholders::_2);
            _service_discoverer = std::make_shared<Discovery>(reg_host, base_service_name, put_cb, del_cb);
        }
        //用于构造服务注册客户端对象
        void make_registry_object(const std::string &reg_host,
            const std::string &service_name,
            const std::string &access_host) {
            _registry_client = std::make_shared<Register>(reg_host);
            _registry_client->Registry(service_name, access_host);
        }
        //用于构造rabbitmq客户端对象
        void make_mq_object(const std::string &user, 
            const std::string &passwd,
            const std::string &host,
            const std::string &exchange_name,
            const std::string &queue_name,
            const std::string &binding_key) {
            _routing_key = binding_key;
            _exchange_name = exchange_name;
            _mq_client = std::make_shared<MQClient>(user, passwd, host);
            _mq_client->DeclareComponents(exchange_name, queue_name, binding_key);
        }
        //构造RPC服务器对象
        void make_rpc_server(uint16_t port, int32_t timeout, uint8_t num_threads) {
            if (!_mq_client) {
                LOG_ERROR("还未初始化消息队列客户端模块！");
                abort();
            }
            if (!_mm_channels) {
                LOG_ERROR("还未初始化信道管理模块！");
                abort();
            }
            if (!_mysql_client) {
                LOG_ERROR("还未初始化Mysql数据库模块！");
                abort();
            }

            _rpc_server = std::make_shared<brpc::Server>();

            TransmitServiceImpl *transmit_service = new TransmitServiceImpl(
                _user_service_name, _mm_channels, _mysql_client, _exchange_name, _routing_key, _mq_client);

            int ret = _rpc_server->AddService(transmit_service, 
                brpc::ServiceOwnership::SERVER_OWNS_SERVICE);
            if (ret == -1) {
                LOG_ERROR("添加Rpc服务失败！");
                abort();
            }
            brpc::ServerOptions options;
            options.idle_timeout_sec = timeout;
            options.num_threads = num_threads;
            ret = _rpc_server->Start(port, &options);
            if (ret == -1) {
                LOG_ERROR("服务启动失败！");
                abort();
            }
        }
        TransmitServer::Ptr build() {
            if (!_service_discoverer) {
                LOG_ERROR("还未初始化服务发现模块！");
                abort();
            }
            if (!_registry_client) {
                LOG_ERROR("还未初始化服务注册模块！");
                abort();
            }
            if (!_rpc_server) {
                LOG_ERROR("还未初始化RPC服务器模块！");
                abort();
            }
            TransmitServer::Ptr server = std::make_shared<TransmitServer>(
                _mysql_client, _service_discoverer, _registry_client, _rpc_server);
            return server;
        }
    private:
        std::string _user_service_name;
        ServiceManager::Ptr _mm_channels;
        Discovery::Ptr _service_discoverer;
        
        std::string _routing_key;
        std::string _exchange_name;
        MQClient::Ptr _mq_client;

        Register::Ptr _registry_client; // 服务注册客户端
        std::shared_ptr<odb::core::database> _mysql_client; //mysql数据库客户端
        std::shared_ptr<brpc::Server> _rpc_server;
};

}