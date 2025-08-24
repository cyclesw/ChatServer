#pragma once

#include "base.pb.h"
#include "transmit.pb.h"

namespace im
{
    class ChatSessionMemberTable;
}
namespace im
{
    class ServiceManager;

    class TransmitServiceImpl: public im::MsgTransmitService
    {
    public:
        TransmitServiceImpl();
        ~TransmitServiceImpl() override;
        void GetTransmitTarget(google::protobuf::RpcController *controller, const im::NewMessageRequest *request,
                               im::GetTransmitTargetResponse *response, google::protobuf::Closure *done) override;
    private:
        std::string _user_service_name;
        std::shared_ptr<ServiceManager> _channels;

        std::shared_ptr<ChatSessionMemberTable> _mysql_session_member_table;

        std::string _exchange_name;
        std::string _routing_key;

    };
}