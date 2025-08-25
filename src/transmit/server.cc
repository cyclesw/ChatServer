#include "server.h"
#include "transmit.pb.h"

#include "database/mysql_chat_session_member.h"
#include <odb/core.hxx>
#include <odb/database.hxx>
#include <memory>


namespace im
{
    TransmitServiceImpl::TransmitServiceImpl(const std::string &user_service_name,
                const std::shared_ptr<ServiceManager> &channels,
                const std::shared_ptr<odb::core::database> &mysql_client,
                const std::string &exchange_name,
                const std::string &routing_key,
                const std::shared_ptr<MQClient> &mq_client)
                :_user_service_name(user_service_name),
                _channels(channels),
                _mysql_session_member_table(mysql_client),
                _exchange_name(exchange_name),
                _routing_key(routing_key),
                _mq_client(mq_client)
    {
        ChatSessionMemberTable table(mysql_client);
    }

    TransmitServiceImpl::~TransmitServiceImpl() = default;
    
    void TransmitServiceImpl::GetTransmitTarget(google::protobuf::RpcController *controller, const im::NewMessageRequest *request,
                                   im::GetTransmitTargetResponse *response, google::protobuf::Closure *done)
    {
        
    }
}