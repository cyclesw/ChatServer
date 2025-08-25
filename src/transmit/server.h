#pragma once

#include "transmit.pb.h"
#include <odb/database.hxx>

namespace im
{
    class ServiceManager;
    class ChatSessionMemberTable;
    class MQClient;

    class TransmitServiceImpl: public im::MsgTransmitService
    {
    public:
        TransmitServiceImpl(const std::string &user_service_name,
            const std::shared_ptr<ServiceManager> &channels,
            const std::shared_ptr<odb::core::database> &mysql_client,
            const std::string &exchange_name,
            const std::string &routing_key,
            const std::shared_ptr<MQClient> &mq_client);
            
        ~TransmitServiceImpl() override;
        void GetTransmitTarget(google::protobuf::RpcController *controller, const im::NewMessageRequest *request,
                               im::GetTransmitTargetResponse *response, google::protobuf::Closure *done) override;
    private:
        std::string _user_service_name;
        std::shared_ptr<ServiceManager> _channels;

        std::shared_ptr<ChatSessionMemberTable> _mysql_session_member_table;

        std::string _exchange_name;
        std::string _routing_key;
        std::shared_ptr<MQClient> _mq_client;
    };
}