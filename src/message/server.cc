#include "server.h"
#include "elastic.h"

#include "database/mysql_message.h"

#include <brpc/channel.h>

namespace im
{
    MessageServiceImpl::MessageServiceImpl(const std::shared_ptr<elasticlient::Client> &es_client,
                const std::shared_ptr<odb::core::database> &mysql_client,
                const std::shared_ptr<ServiceManager> &channel_manager,
                const std::string &file_service_name,
                const std::string &user_service_name)
            :_es_message(std::make_shared<ESMessage>(es_client)),
             _mysql_message(std::make_shared<MessageTable>(mysql_client)),
             _file_service_name(file_service_name),
             _user_service_name(user_service_name),
             _channels(channel_manager)
        {
            _es_message->CreateIndex();
        }
    
    MessageServiceImpl::~MessageServiceImpl()
    {
        
    }
    
    void MessageServiceImpl::GetHistoryMessage(::google::protobuf::RpcController *controller, const ::im::GetHistoryMessageRequest *request, ::im::GetHistoryMessageResponse *response, ::google::protobuf::Closure *done)
    {
        
    }
    
    void MessageServiceImpl::GetRecentMessage(::google::protobuf::RpcController *controller, const ::im::GetRecentMessageRequest *request, ::im::GetRecentMessageResponse *response, ::google::protobuf::Closure *done)
    {
        
    }
    
    void MessageServiceImpl::MessageSearch(::google::protobuf::RpcController *controller, const ::im::MessageSearchRequest *request, ::im::MessageSearchResponse *response, ::google::protobuf::Closure *done)
    {
        
    }
    
    void MessageServiceImpl::OnMessage(const char* body, size_t size)
    {
        
    }
    
    MessageServer::MessageServer(const MQClient::Ptr &mq_client,
                const Discovery::Ptr service_discoverer, 
                const Register::Ptr &reg_client,
                const std::shared_ptr<elasticlient::Client> &es_client,
                const std::shared_ptr<odb::core::database> &mysql_client,
                const std::shared_ptr<brpc::Server> &server)
    {
        
    }
    
    void MessageServer::Start()
    {
        
    }
    
    void MessageServerBuilder::make_es_object(const std::vector<std::string> host_list)
    {
        
    }
    
    void MessageServerBuilder::make_mysql_object(const std::string &user,
                const std::string &pswd,
                const std::string &host,
                const std::string &db,
                const std::string &cset,
                int port,
                int conn_pool_count)
    {
        
    }
    
    void MessageServerBuilder::make_discovery_object(const std::string &reg_host,
                const std::string &base_service_name,
                const std::string &file_service_name,
                const std::string &user_service_name)
    {
        
    }
    
    void MessageServerBuilder::make_registry_object(const std::string &reg_host,
                const std::string &service_name,
                const std::string &access_host)
    {
        
    }
    
    void MessageServerBuilder::make_mq_object(const std::string &user, 
                const std::string &passwd,
                const std::string &host,
                const std::string &exchange_name,
                const std::string &queue_name,
                const std::string &binding_key)
    {
        
    }
    
    void MessageServerBuilder::make_rpc_server(uint16_t port, int32_t timeout, uint8_t num_threads)
    {
        
    }
    
    MessageServer::ptr MessageServerBuilder::build()
    {
        
    }
}