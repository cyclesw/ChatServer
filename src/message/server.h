#include "etcd.h"
#include "message.pb.h"
#include "rabbitmq.h"
#include "channel.h"

#include <odb/forward.hxx>
#include <brpc/server.h>

namespace elasticlient 
{
    class Client;
}

namespace im
{
    class ServiceManager;
    class MessageTable;
    class ESMessage;

    class MessageServiceImpl : public im::MessageStorageService
    {
    public:
        MessageServiceImpl(
            const std::shared_ptr<elasticlient::Client> &es_client,
            const std::shared_ptr<odb::core::database> &mysql_client,
            const std::shared_ptr<ServiceManager> &channel_manager,
            const std::string &file_service_name,
            const std::string &user_service_name);

        ~MessageServiceImpl();

        void GetHistoryMessage(::google::protobuf::RpcController *controller, const ::im::GetHistoryMessageRequest *request, ::im::GetHistoryMessageResponse *response, ::google::protobuf::Closure *done) override;

        void GetRecentMessage(::google::protobuf::RpcController *controller, const ::im::GetRecentMessageRequest *request, ::im::GetRecentMessageResponse *response, ::google::protobuf::Closure *done) override;

        void MessageSearch(::google::protobuf::RpcController *controller, const ::im::MessageSearchRequest *request, ::im::MessageSearchResponse *response, ::google::protobuf::Closure *done) override;

        void OnMessage(const char* body, size_t size);

    private:
        std::shared_ptr<ESMessage> _es_message;
        std::shared_ptr<MessageTable> _mysql_message;

        std::string _user_service_name;
        std::string _file_service_name;
        std::shared_ptr<ServiceManager> _channels;
    };

    class MessageServer {
    public:
        using ptr = std::shared_ptr<MessageServer>;
        MessageServer(const MQClient::Ptr &mq_client,
            const Discovery::Ptr service_discoverer, 
            const Register::Ptr &reg_client,
            const std::shared_ptr<elasticlient::Client> &es_client,
            const std::shared_ptr<odb::core::database> &mysql_client,
            const std::shared_ptr<brpc::Server> &server);

        void Start();
    private:
        Discovery::Ptr _service_discoverer;
        Register::Ptr _registry_client;
        MQClient::Ptr _mq_client;
        std::shared_ptr<elasticlient::Client> _es_client;
        std::shared_ptr<odb::core::database> _mysql_client;
        std::shared_ptr<brpc::Server> _rpc_server;
    };
    class MessageServerBuilder {
    public:
        void make_es_object(const std::vector<std::string> host_list);
     
        void make_mysql_object(
            const std::string &user,
            const std::string &pswd,
            const std::string &host,
            const std::string &db,
            const std::string &cset,
            int port,
            int conn_pool_count);

        void make_discovery_object(const std::string &reg_host,
            const std::string &base_service_name,
            const std::string &file_service_name,
            const std::string &user_service_name);

    
        void make_registry_object(const std::string &reg_host,
            const std::string &service_name,
            const std::string &access_host);

        void make_mq_object(const std::string &user, 
            const std::string &passwd,
            const std::string &host,
            const std::string &exchange_name,
            const std::string &queue_name,
            const std::string &binding_key);

        void make_rpc_server(uint16_t port, int32_t timeout, uint8_t num_threads);

        MessageServer::ptr build();
    private:
        Register::Ptr _registry_client;

        std::shared_ptr<elasticlient::Client> _es_client;
        std::shared_ptr<odb::core::database> _mysql_client;

        std::string _user_service_name;
        std::string _file_service_name;
        ServiceManager::Ptr _mm_channels;
        Discovery::Ptr _service_discoverer;

        std::string _exchange_name;
        std::string _queue_name;
        MQClient::Ptr _mq_client;

        std::shared_ptr<brpc::Server> _rpc_server;

    };
}