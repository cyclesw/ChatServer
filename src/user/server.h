#pragma once

#include "../common/database/user_table.hpp"
#include "dms.h"
#include "elastic.h"
#include "etcd.h"

#include "user.pb.h"

namespace sw::redis
{
    class Redis;
}
namespace brpc
{
   class Server;
}

namespace im
{
    class Codes;
    class Status;
    class Session;
    class ServiceManager;

class UserServiceImpl : public im::UserService
{
  public:
    UserServiceImpl(const DMSClient::Ptr &dm_client, const std::shared_ptr<elasticlient::Client> &es_client,
                    const std::shared_ptr<odb::core::database> &mysql_client,
                    const std::shared_ptr<sw::redis::Redis> &redis_client, const std::shared_ptr<ServiceManager> &channel_manager,
                    const std::string &file_service_name);
    ~UserServiceImpl() override;

    static bool NicknameCheck(const std::string &nickname);

    static bool PasswordCheck(const std::string &password);

    static bool PhoneCheck(const std::string &phone);

    void UserRegister(google::protobuf::RpcController *controller, const im::UserRegisterRequest *request,
                      im::UserRegisterResponse *response, google::protobuf::Closure *done) override;

    void UserLogin(google::protobuf::RpcController *controller, const im::UserLoginRequest *request,
                   im::UserLoginResponse *response, google::protobuf::Closure *done) override;

    void GetPhoneVerifyCode(google::protobuf::RpcController *controller, const im::PhoneVerifyCodeRequest *request,
                            im::PhoneVerifyCodeResponse *response, google::protobuf::Closure *done) override;

    void PhoneRegister(google::protobuf::RpcController *controller, const im::PhoneRegisterRequest *request,
                       im::PhoneRegisterResponse *response, google::protobuf::Closure *done) override;

    void PhoneLogin(google::protobuf::RpcController *controller, const im::PhoneLoginRequest *request,
                    im::PhoneLoginResponse *response, google::protobuf::Closure *done) override;

    void GetUserInfo(google::protobuf::RpcController *controller, const im::GetUserInfoRequest *request,
                     im::GetUserInfoResponse *response, google::protobuf::Closure *done) override;

    void GetMultiUserInfo(google::protobuf::RpcController *controller, const im::GetMultiUserInfoRequest *request,
                          im::GetMultiUserInfoResponse *response, google::protobuf::Closure *done) override;

    void SetUserAvatar(google::protobuf::RpcController *controller, const im::SetUserAvatarRequest *request,
                       im::SetUserAvatarResponse *response, google::protobuf::Closure *done) override;
    void SetUserNickname(google::protobuf::RpcController *controller, const im::SetUserNicknameRequest *request,
                         im::SetUserNicknameResponse *response, google::protobuf::Closure *done) override;
    void SetUserDescription(google::protobuf::RpcController *controller, const im::SetUserDescriptionRequest *request,
                            im::SetUserDescriptionResponse *response, google::protobuf::Closure *done) override;
    void SetUserPhoneNumber(google::protobuf::RpcController *controller, const im::SetUserPhoneNumberRequest *request,
                            im::SetUserPhoneNumberResponse *response, google::protobuf::Closure *done) override;

  private:
    ESUser::Ptr _es_user;
    UserTable::Ptr _mysql_user;
    std::shared_ptr<Session> _redis_session;
    std::shared_ptr<Status> _redis_status;
    std::shared_ptr<Codes> _redis_codes;

    std::string _file_service_name;
    std::shared_ptr<ServiceManager> _channels;
    DMSClient::Ptr _dms_client;
};

class UserServer
{
public:
    using Ptr = std::shared_ptr<UserServer>;

    UserServer(const Discovery::Ptr &service_discoverer, const Register::Ptr &reg_client,
               const std::shared_ptr<elasticlient::Client> &es_client,
               const std::shared_ptr<odb::core::database> &mysql_client,
               const std::shared_ptr<sw::redis::Redis> &redis_client, const std::shared_ptr<brpc::Server> &server);

    ~UserServer();

    void Start() const;

  private:
    Discovery::Ptr _service_discoverer;
    Register::Ptr _registry_client;
    std::shared_ptr<elasticlient::Client> _es_client;
    std::shared_ptr<odb::core::database> _mysql_client;
    std::shared_ptr<sw::redis::Redis> _redis_client;
    std::shared_ptr<brpc::Server> _rpc_server;
};

// TODO: Dms添加多平台适配

class UserServerBuilder
{
  public:
    void MakeESObject(const std::vector<std::string> &host_list);

    void MakeDmsObject(std::string &access_key);

    void MakeMysqlObject(const std::string &user, const std::string &pswd, const std::string &host,
                         const std::string &db, const std::string &cset, int port, int conn_pool_count);

    void MakeRedisObject(const std::string& host,
        int port,
        int db,
        bool keep_alive);

    void MakeDiscoveryObject(const std::string& reg_host,
        const std::string& base_service_name,
        const std::string& file_service_name);

    void MakeRegistryObject(const std::string& reg_host,
        const std::string& service_name,
        const std::string& access_host);

    void MakeRpcServer(uint16_t port, int32_t timeout, uint8_t num_threads);

    UserServer::Ptr Build();
private:
    Register::Ptr _registry_client;

    std::shared_ptr<elasticlient::Client> _es_client;
    std::shared_ptr<odb::core::database> _mysql_client;
    std::shared_ptr<sw::redis::Redis> _redis_client;

    std::string _file_service_name;
    std::shared_ptr<ServiceManager> _channels;
    Discovery::Ptr _service_discoverer;

    std::shared_ptr<DMSClient> _dms_client;

    std::shared_ptr<brpc::Server> _rpc_server;
};
} // namespace im