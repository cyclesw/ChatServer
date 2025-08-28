#pragma once

#include "channel.h"
#include "database/mysql_apply.h"
#include "database/mysql_chat_session.h"
#include "database/mysql_chat_session_member.h"
#include "database/mysql_relation.h"
#include "elastic.h"
#include "etcd.h"
#include "friend.pb.h"

#include <brpc/server.h>
#include <memory>
#include <odb/forward.hxx>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace im
{
    class MQClient;

    class FriendServiceImpl : public im::FriendService
    {
    public:
        FriendServiceImpl(const std::shared_ptr<elasticlient::Client> &es_client,
                          const std::shared_ptr<odb::core::database> &mysql_client,
                          const ServiceManager::Ptr &channel_manager, const std::string &user_service_name,
                          const std::string &message_service_name);

        ~FriendServiceImpl() = default;

        void GetFriendList(::google::protobuf::RpcController *controller, const ::im::GetFriendListRequest *request,
                           ::im::GetFriendListResponse *response, ::google::protobuf::Closure *done) override;

        void FriendRemove(::google::protobuf::RpcController *controller, const ::im::FriendRemoveRequest *request,
                          ::im::FriendRemoveResponse *response, ::google::protobuf::Closure *done) override;

        void FriendAdd(::google::protobuf::RpcController *controller, const ::im::FriendAddRequest *request,
                       ::im::FriendAddResponse *response, ::google::protobuf::Closure *done) override;

        void FriendAddProcess(::google::protobuf::RpcController *controller,
                              const ::im::FriendAddProcessRequest *request, ::im::FriendAddProcessResponse *response,
                              ::google::protobuf::Closure *done) override;

        void FriendSearch(::google::protobuf::RpcController *controller, const ::im::FriendSearchRequest *request,
                          ::im::FriendSearchResponse *response, ::google::protobuf::Closure *done) override;

        void GetPendingFriendEventList(::google::protobuf::RpcController *controller,
                                       const ::im::GetPendingFriendEventListRequest *request,
                                       ::im::GetPendingFriendEventListResponse *response,
                                       ::google::protobuf::Closure *done) override;

        void GetChatSessionList(::google::protobuf::RpcController *controller,
                                const ::im::GetChatSessionListRequest *request,
                                ::im::GetChatSessionListResponse *response, ::google::protobuf::Closure *done) override;

        void ChatSessionCreate(::google::protobuf::RpcController *controller,
                               const ::im::ChatSessionCreateRequest *request, ::im::ChatSessionCreateResponse *response,
                               ::google::protobuf::Closure *done) override;

        void GetChatSessionMember(::google::protobuf::RpcController *controller,
                                  const ::im::GetChatSessionMemberRequest *request,
                                  ::im::GetChatSessionMemberResponse *response,
                                  ::google::protobuf::Closure *done) override;

    private:
        bool GetRecentMessage(const std::string &rid, const std::string &cssid, MessageInfo &msg);

        bool GetUserInfo(const std::string &rid, const std::unordered_set<std::string> &uid_list,
                         std::unordered_map<std::string, UserInfo> &user_list);

    private:
        ESUser::Ptr _es_user;

        FriendApplyTable::Ptr _mysql_apply;
        ChatSessionTable::Ptr _mysql_chat_session;
        ChatSessionMemberTable::Ptr _mysql_chat_session_member;
        RelationTable::Ptr _mysql_relation;

        // rpc
        std::string _user_service_name;
        std::string _message_service_name;
        ServiceManager::Ptr _channels;
    };

    class FriendServer
    {
    public:
        using Ptr = std::shared_ptr<FriendServer>;

        FriendServer(const Discovery::Ptr &service_discoverer, const Register::Ptr &reg_client,
                     const std::shared_ptr<elasticlient::Client> &es_client,
                     const std::shared_ptr<odb::core::database> &mysql_client,
                     const std::shared_ptr<brpc::Server> &server);

        ~FriendServer() = default;

        void Start();

    private:
        Discovery::Ptr _service_discoverer;
        Register::Ptr _reg_client;
        std::shared_ptr<elasticlient::Client> _es_client;
        std::shared_ptr<odb::core::database> _mysql_client;
        std::shared_ptr<brpc::Server> _rpc_server;
    };

    class FriendServerBuilder
    {
    public:
        // 构造es客户端对象
        void make_es_object(const std::vector<std::string> host_list);

        void make_mysql_object(const std::string &user, const std::string &pswd, const std::string &host,
                               const std::string &db, const std::string &cset, int port, int conn_pool_count);


        void make_discovery_object(const std::string &reg_host, const std::string &base_service_name,
                                   const std::string &user_service_name, const std::string &message_service_name);

        void make_registry_object(const std::string &reg_host, const std::string &service_name,
                                  const std::string &access_host);

        void make_mq_object(const std::string &user, const std::string &passwd, const std::string &host,
                            const std::string &exchange_name, const std::string &queue_name,
                            const std::string &binding_key);


        void make_rpc_server(uint16_t port, int32_t timeout, uint8_t num_threads);
        FriendServer::Ptr build();

    private:
        Register::Ptr _registry_client;

        std::shared_ptr<elasticlient::Client> _es_client;
        std::shared_ptr<odb::core::database> _mysql_client;

        std::string _user_service_name;
        std::string _message_service_name;
        ServiceManager::Ptr _mm_channels;
        Discovery::Ptr _service_discoverer;

        std::string _exchange_name;
        std::string _queue_name;
        std::shared_ptr<MQClient> _mq_client;


        std::shared_ptr<brpc::Server> _rpc_server;
    };

} // namespace im
