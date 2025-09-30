//
// Created by 19396 on 25-5-15.
//
#include "server.h"

#include "../common/database/redis.h"
#include "channel.h"
#include "connection.h"
#include "etcd.h"
#include "log.hpp"

#include "friend.pb.h"
#include "gateway.pb.h"
#include "message.pb.h"
#include "notify.pb.h"
#include "user.pb.h"

#include <brpc/channel.h>
#include <brpc/controller.h>
#include <google/protobuf/message.h>
#include <httplib.h>

#include <memory>
#include <system_error>
#include <utility>

#include "file.pb.h"
#include "speech.pb.h"
#include "transmit.pb.h"

using namespace im;
using namespace im;

namespace
{
    // 公共模板函数处理RPC请求
    template<typename RequestType, typename ResponseType, typename ServiceStub, typename StubMethod>
    void ProcessRpcRequest(const std::string &request_body, const std::string &service_name,
                           const std::string &log_prefix, const std::shared_ptr<ServiceManager> &channels,
                           StubMethod method, httplib::Response &http_response)
    {
        RequestType pb_request;
        ResponseType pb_response;

        auto error_response = [&pb_response, &http_response](const std::string &errmsg)
        {
            pb_response.set_success(false);
            pb_response.set_error(errmsg);
            http_response.set_content(pb_response.SerializeAsString(), "application/x-protobuf");
        };

        // 反序列化请求
        if (!pb_request.ParseFromString(request_body))
        {
            LOG_ERROR("{}请求正文反序列化失败!", log_prefix);
            return error_response(log_prefix + "请求正文反序列化失败");
        }

        // 选择服务通道
        auto channel = channels->Choose(service_name);
        if (!channel)
        {
            LOG_ERROR("{} {} 未找到可用服务节点", pb_request.request_id(), log_prefix);
            return error_response("未找到可用的服务节点");
        }

        // RPC调用
        ServiceStub stub(channel.get());
        brpc::Controller cntl;
        (stub.*method)(&cntl, &pb_request, &pb_response, nullptr);

        // 错误处理
        if (cntl.Failed())
        {
            LOG_ERROR("{} {} 服务调用失败: {}", pb_request.request_id(), log_prefix, cntl.ErrorText());
            return error_response("服务调用失败: " + cntl.ErrorText());
        }

        // 成功响应
        http_response.set_content(pb_response.SerializeAsString(), "application/x-protobuf");
    }
} // namespace

GatewayServer::GatewayServer(int websocket_port, int http_port, const std::shared_ptr<sw::redis::Redis> &redis_client,
                             const std::shared_ptr<ServiceManager> &channels,
                             const std::shared_ptr<Discovery> &service_discoverer, const std::string &user_service_name,
                             const std::string &file_service_name, const std::string &speech_service_name,
                             const std::string &message_service_name, const std::string &transmite_service_name,
                             const std::string &friend_service_name) :
    _redis_session(std::make_shared<Session>(redis_client)), _redis_status(std::make_shared<Status>(redis_client)),
    _user_service_name(user_service_name), _file_service_name(file_service_name),
    _speech_service_name(speech_service_name), _message_service_name(message_service_name),
    _transmite_service_name(transmite_service_name), _friend_service_name(friend_service_name), _channels(channels),
    _service_discoverer(service_discoverer), _connections(std::make_shared<Connection>()),
    _http_server(std::make_shared<httplib::Server>())
{
    _ws_server.set_access_channels(websocketpp::log::alevel::none);
    _ws_server.init_asio();
    _ws_server.set_open_handler([this](websocketpp::connection_hdl hdl) { this->OnOpen(std::move(hdl)); });
    _ws_server.set_close_handler([this](websocketpp::connection_hdl hdl) { this->OnClose(std::move(hdl)); });
    auto websocket_callback = [this](websocketpp::connection_hdl hdl, server_t::message_ptr msg)
    { this->OnMessage(std::move(hdl), std::move(msg)); };

    _ws_server.set_message_handler(websocket_callback);
    ;
    _ws_server.set_reuse_addr(true);
    _ws_server.listen(websocket_port);
    _ws_server.start_accept();

    _http_server->Post(GET_PHONE_VERIFY_CODE,
                       (httplib::Server::Handler) std::bind(&GatewayServer::GetPhoneVerifyCode, this,
                                                            std::placeholders::_1, std::placeholders::_2));
    _http_server->Post(USERNAME_REGISTER,
                       (httplib::Server::Handler) std::bind(&GatewayServer::UserRegister, this, std::placeholders::_1,
                                                            std::placeholders::_2));
    _http_server->Post(USERNAME_LOGIN,
                       (httplib::Server::Handler) std::bind(&GatewayServer::UserLogin, this, std::placeholders::_1,
                                                            std::placeholders::_2));
    _http_server->Post(PHONE_REGISTER,
                       (httplib::Server::Handler) std::bind(&GatewayServer::PhoneRegister, this, std::placeholders::_1,
                                                            std::placeholders::_2));
    _http_server->Post(PHONE_LOGIN, (httplib::Server::Handler) std::bind(&GatewayServer::PhoneLogin, this,
                                                                         std::placeholders::_1, std::placeholders::_2));
    _http_server->Post(GET_USERINFO,
                       (httplib::Server::Handler) std::bind(&GatewayServer::GetUserInfo, this, std::placeholders::_1,
                                                            std::placeholders::_2));
    _http_server->Post(SET_USER_AVATAR,
                       (httplib::Server::Handler) std::bind(&GatewayServer::SetUserAvatar, this, std::placeholders::_1,
                                                            std::placeholders::_2));
    _http_server->Post(SET_USER_NICKNAME,
                       (httplib::Server::Handler) std::bind(&GatewayServer::SetUserNickname, this,
                                                            std::placeholders::_1, std::placeholders::_2));
    _http_server->Post(SET_USER_DESC,
                       (httplib::Server::Handler) std::bind(&GatewayServer::SetUserDescription, this,
                                                            std::placeholders::_1, std::placeholders::_2));
    _http_server->Post(SET_USER_PHONE,
                       (httplib::Server::Handler) std::bind(&GatewayServer::SetUserPhoneNumber, this,
                                                            std::placeholders::_1, std::placeholders::_2));
    _http_server->Post(FRIEND_GET_LIST,
                       (httplib::Server::Handler) std::bind(&GatewayServer::GetFriendList, this, std::placeholders::_1,
                                                            std::placeholders::_2));
    _http_server->Post(FRIEND_APPLY,
                       (httplib::Server::Handler) std::bind(&GatewayServer::FriendAdd, this, std::placeholders::_1,
                                                            std::placeholders::_2));
    _http_server->Post(FRIEND_APPLY_PROCESS,
                       (httplib::Server::Handler) std::bind(&GatewayServer::FriendAddProcess, this,
                                                             std::placeholders::_1, std::placeholders::_2));

    _http_server->Post(FRIEND_REMOVE,
                       (httplib::Server::Handler) std::bind(&GatewayServer::FriendRemove, this, std::placeholders::_1,
                                                            std::placeholders::_2));
    _http_server->Post(FRIEND_SEARCH,
                       (httplib::Server::Handler) std::bind(&GatewayServer::FriendSearch, this, std::placeholders::_1,
                                                            std::placeholders::_2));
    _http_server->Post(FRIEND_GET_PENDING_EV,
                       (httplib::Server::Handler) std::bind(&GatewayServer::GetPendingFriendEventList, this,
                                                            std::placeholders::_1, std::placeholders::_2));
    _http_server->Post(CSS_GET_LIST,
                       (httplib::Server::Handler) std::bind(&GatewayServer::GetChatSessionList, this,
                                                            std::placeholders::_1, std::placeholders::_2));
    _http_server->Post(CSS_CREATE, (httplib::Server::Handler) std::bind(&GatewayServer::ChatSessionCreate, this,
                                                                        std::placeholders::_1, std::placeholders::_2));
    _http_server->Post(CSS_GET_MEMBER,
                       (httplib::Server::Handler) std::bind(&GatewayServer::GetChatSessionMember, this,
                                                            std::placeholders::_1, std::placeholders::_2));
    _http_server->Post(MSG_GET_RANGE,
                       (httplib::Server::Handler) std::bind(&GatewayServer::GetHistoryMessage, this,
                                                            std::placeholders::_1, std::placeholders::_2));
    _http_server->Post(MSG_GET_RECENT,
                       (httplib::Server::Handler) std::bind(&GatewayServer::GetRecentMessage, this,
                                                            std::placeholders::_1, std::placeholders::_2));
    _http_server->Post(MSG_KEY_SEARCH,
                       (httplib::Server::Handler) std::bind(&GatewayServer::SearchMessage, this, std::placeholders::_1,
                                                            std::placeholders::_2));
    _http_server->Post(NEW_MESSAGE, (httplib::Server::Handler) std::bind(&GatewayServer::NewMessage, this,
                                                                         std::placeholders::_1, std::placeholders::_2));
    _http_server->Post(FILE_GET_SINGLE,
                       (httplib::Server::Handler) std::bind(&GatewayServer::GetSingleFile, this, std::placeholders::_1,
                                                            std::placeholders::_2));
    _http_server->Post(FILE_GET_MULTI,
                       (httplib::Server::Handler) std::bind(&GatewayServer::GetMultiFile, this, std::placeholders::_1,
                                                            std::placeholders::_2));
    _http_server->Post(FILE_PUT_SINGLE,
                       (httplib::Server::Handler) std::bind(&GatewayServer::PutSingleFile, this, std::placeholders::_1,
                                                            std::placeholders::_2));
    _http_server->Post(FILE_PUT_MULTI,
                       (httplib::Server::Handler) std::bind(&GatewayServer::PutMultiFile, this, std::placeholders::_1,
                                                            std::placeholders::_2));
    _http_server->Post(SPEECH_RECOGNITION,
                       (httplib::Server::Handler) std::bind(&GatewayServer::SpeechRecognition, this,
                                                            std::placeholders::_1, std::placeholders::_2));
    _http_thread = std::thread([this, http_port]() { _http_server->listen("0.0.0.0", http_port); });
    _http_thread.detach();
}

void GatewayServer::OnOpen(websocketpp::connection_hdl hdl)
{
    LOG_DEBUG("websocket长连接建立成功: {}", (size_t) _ws_server.get_con_from_hdl(std::move(hdl)).get());
}

void GatewayServer::OnClose(websocketpp::connection_hdl hdl)
{
    auto conn = _ws_server.get_con_from_hdl(std::move(hdl));
    std::string uid, ssid;
    bool ret = _connections->GetClient(conn, uid, ssid);
    if (!ret)
    {
        LOG_DEBUG("websocket长连接建立断开");
        return;
    }

    _redis_session->Remove(ssid);
    _redis_status->Remove(uid);
    _connections->Remove(conn);
    LOG_DEBUG("websocket长连接断开: {}{}{}", ssid, uid, (size_t) conn.get());
}

void GatewayServer::OnMessage(websocketpp::connection_hdl hdl, server_t::message_ptr msg)
{
    auto conn = _ws_server.get_con_from_hdl(std::move(hdl));
    ClientAuthenticationRequest request;
    bool ret = request.ParseFromString(msg->get_payload());
    if (!ret)
    {
        LOG_ERROR("长连接身份识别失败: 正文反序列化失败!");
        _ws_server.close(hdl, websocketpp::close::status::unsupported_data, "正文反序列化失败!");
        return;
    }

    std::string ssid = request.session_id();
    auto uid = _redis_session->Uid(ssid);
    if (!uid)
    {
        LOG_ERROR("长连接身份识别失败: 未找到会话信息 {}!", ssid);
        _ws_server.close(hdl, websocketpp::close::status::unsupported_data, "未找到会话信息!");
        return;
    }

    _connections->Insert(conn, *uid, ssid);
    LOG_DEBUG("新增长连接管理: {}-{}-{}", ssid, *uid, (size_t) conn.get());
    KeepAlive(conn);
}

void GatewayServer::KeepAlive(server_t::connection_ptr conn)
{
    if (!conn || conn->get_state() != websocketpp::session::state::value::open)
    {
        LOG_DEBUG("非正常连接状态，结束连接保活");
        return;
    }
    conn->ping(""); // TODO: 作用

    _ws_server.set_timer(60000,
                         [this, conn](const websocketpp::lib::error_code &ec)
                         {
                             if (ec)
                             {
                                 LOG_DEBUG("长连接保活失败: {}", ec.message());
                                 return;
                             }
                             KeepAlive(conn);
                         });
}

void GatewayServer::GetPhoneVerifyCode(const httplib::Request &request, httplib::Response &response)
{
    // 获取http请求正文并序列化
    PhoneVerifyCodeRequest phone_request;
    PhoneVerifyCodeResponse phone_response;

    auto error_response = [&phone_response, &response](const std::string &errmsg)
    {
        phone_response.set_success(false);
        phone_response.set_error(errmsg);
        response.set_content(phone_response.SerializeAsString(), "application/x-protobuf");
    };

    bool ret = phone_request.ParseFromString(request.body);
    if (!ret)
    {
        LOG_ERROR("获取短信验证码请求正文反序列化失败!");
        return error_response("获取短信验证码请求正文反序列化失败");
    }

    // 请求转发给用户子服务进行业务处理
    auto channel = _channels->Choose(_user_service_name);
    if (!channel)
    {
        LOG_ERROR("{} 用户子服务调用失败", phone_request.request_id());
        return error_response("未找到可提供业务处理的用户子服务节点");
    }

    im::UserService_Stub stub(channel.get());
    brpc::Controller cntl;
    stub.GetPhoneVerifyCode(&cntl, &phone_request, &phone_response, nullptr);
    if (cntl.Failed())
    {
        LOG_ERROR("{} 用户子服务调用失败!", phone_request.request_id());
        return error_response("用户子服务调用失败!");
    }

    // 得到用户子服务的响应后，将响应内容进行序列化作为http响应正文
    response.set_content(phone_response.SerializeAsString(), "application/x-protobuf");
}

void GatewayServer::PhoneRegister(const httplib::Request &request, httplib::Response &response)
{
    PhoneRegisterRequest phone_request;
    PhoneRegisterResponse phone_response;

    auto error_response = [&phone_response, &response](const std::string &errmsg)
    {
        phone_response.set_success(false);
        phone_response.set_error(errmsg);
        response.set_content(phone_response.SerializeAsString(), "application/x-protobuf");
    };

    bool ret = phone_request.ParseFromString(request.body);
    if (!ret)
    {
        LOG_ERROR("手机号注册请求正文反序列化失败!");
        return error_response("手机号注册请求正文反序列化失败");
    }

    auto channel = _channels->Choose(_user_service_name);
    if (!channel)
    {
        LOG_ERROR("{} 未找到可提供业务处理的用户子服务节点", phone_request.request_id());
        return error_response("未找到可提供业务处理的用户子服务节点");
    }
    im::UserService_Stub stub(channel.get());
    brpc::Controller cntl;
    stub.PhoneRegister(&cntl, &phone_request, &phone_response, nullptr);
    if (cntl.Failed())
    {
        LOG_ERROR("{} 用户子服务调用失败!", phone_request.request_id());
        return error_response("用户子服务调用失败!");
    }
    // 3. 得到用户子服务的响应后，将响应内容进行序列化作为http响应正文
    response.set_content(phone_response.SerializeAsString(), "application/x-protobuf");
}

void GatewayServer::PhoneLogin(const httplib::Request &request, httplib::Response &response)
{
    // 1. 取出http请求正文，将正文进行反序列化
    PhoneLoginRequest phone_request;
    PhoneLoginResponse phone_response;
    auto err_response = [&phone_request, &phone_response, &response](const std::string &errmsg) -> void
    {
        phone_response.set_success(false);
        phone_response.set_error(errmsg);
        response.set_content(phone_response.SerializeAsString(), "application/x-protbuf");
    };
    bool ret = phone_request.ParseFromString(request.body);
    if (ret == false)
    {
        LOG_ERROR("手机号登录请求正文反序列化失败！");
        return err_response("手机号登录请求正文反序列化失败！");
    }
    // 2. 将请求转发给用户子服务进行业务处理
    auto channel = _channels->Choose(_user_service_name);
    if (!channel)
    {
        LOG_ERROR("{} 未找到可提供业务处理的用户子服务节点！", phone_request.request_id());
        return err_response("未找到可提供业务处理的用户子服务节点！");
    }
    im::UserService_Stub stub(channel.get());
    brpc::Controller cntl;
    stub.PhoneLogin(&cntl, &phone_request, &phone_response, nullptr);
    if (cntl.Failed())
    {
        LOG_ERROR("{} 用户子服务调用失败！", phone_request.request_id());
        return err_response("用户子服务调用失败！");
    }
    // 3. 得到用户子服务的响应后，将响应内容进行序列化作为http响应正文
    response.set_content(phone_response.SerializeAsString(), "application/x-protbuf");
}

void GatewayServer::UserRegister(const httplib::Request &request, httplib::Response &response)
{
    UserRegisterRequest user_request;
    UserRegisterResponse user_response;
    auto error_response = [&user_response, &response](const std::string &errmsg)
    {
        user_response.set_success(false);
        user_response.set_error(errmsg);
        response.set_content(user_response.SerializeAsString(), "application/x-protobuf");
    };

    bool ret = user_request.ParseFromString(request.body);
    if (!ret)
    {
        LOG_ERROR("用户名注册请求正文反序列化失败！");
        return error_response("用户名注册请求正文反序列化失败！");
    }

    auto channel = _channels->Choose(request.body);
    if (!channel)
    {
        LOG_ERROR("{} 未找到可提供业务处理的用户子服务节点", user_request.request_id());
        return error_response("未找到可提供业务处理的用户子服务节点！");
    }

    im::UserService_Stub stub(channel.get());
    brpc::Controller cntl;
    stub.UserRegister(&cntl, &user_request, &user_response, nullptr);
    if (cntl.Failed())
    {
        LOG_ERROR("{} 用户子服务调用失败!", user_request.request_id());
        return error_response("用户子服务调用失败！");
    }

    response.set_content(user_response.SerializeAsString(), "application/x-protobuf");
}

void GatewayServer::UserLogin(const httplib::Request &request, httplib::Response &response)
{
    UserLoginRequest user_request;
    UserLoginResponse user_response;
    auto error_response = [&user_response, &response](const std::string &errmsg)
    {
        user_response.set_success(false);
        user_response.set_error(errmsg);
        response.set_content(user_response.SerializeAsString(), "application/x-protobuf");
    };

    bool ret = user_request.ParseFromString(request.body);
    if (!ret)
    {
        LOG_ERROR("用户名注册请求正文反序列化失败！");
        return error_response("用户名注册请求正文反序列化失败！");
    }

    auto channel = _channels->Choose(request.body);
    if (!channel)
    {
        LOG_ERROR("{} 未找到可提供业务处理的用户子服务节点", user_request.request_id());
        return error_response("未找到可提供业务处理的用户子服务节点！");
    }

    im::UserService_Stub stub(channel.get());
    brpc::Controller cntl;
    stub.UserLogin(&cntl, &user_request, &user_response, nullptr);
    if (cntl.Failed())
    {
        LOG_ERROR("{} 用户子服务调用失败!", user_request.request_id());
        return error_response("用户子服务调用失败！");
    }

    response.set_content(user_response.SerializeAsString(), "application/x-protobuf");
}

void GatewayServer::GetUserInfo(const httplib::Request &request, httplib::Response &response)
{
    // 1. 取出http请求正文，将正文进行反序列化
    GetUserInfoRequest user_request;
    GetUserInfoResponse user_response;
    auto err_response = [&user_request, &user_response, &response](const std::string &errmsg) -> void
    {
        user_response.set_success(false);
        user_response.set_error(errmsg);
        response.set_content(user_response.SerializeAsString(), "application/x-protbuf");
    };
    bool ret = user_request.ParseFromString(request.body);
    if (ret == false)
    {
        LOG_ERROR("获取用户信息请求正文反序列化失败！");
        return err_response("获取用户信息请求正文反序列化失败！");
    }
    // 2. 客户端身份识别与鉴权
    std::string ssid = user_request.session_id();
    auto uid = _redis_session->Uid(ssid);
    if (!uid)
    {
        LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
        return err_response("获取登录会话关联用户信息失败！");
    }
    user_request.set_user_id(*uid);
    // 2. 将请求转发给用户子服务进行业务处理
    auto channel = _channels->Choose(_user_service_name);
    if (!channel)
    {
        LOG_ERROR("{} 未找到可提供业务处理的用户子服务节点！", user_request.request_id());
        return err_response("未找到可提供业务处理的用户子服务节点！");
    }
    im::UserService_Stub stub(channel.get());
    brpc::Controller cntl;
    stub.GetUserInfo(&cntl, &user_request, &user_response, nullptr);
    if (cntl.Failed())
    {
        LOG_ERROR("{} 用户子服务调用失败！", user_request.request_id());
        return err_response("用户子服务调用失败！");
    }
    // 3. 得到用户子服务的响应后，将响应内容进行序列化作为http响应正文
    response.set_content(user_response.SerializeAsString(), "application/x-protbuf");
}

void GatewayServer::SetUserAvatar(const httplib::Request &request, httplib::Response &response)
{
    // 1. 取出http请求正文，将正文进行反序列化
    SetUserAvatarRequest user_request;
    SetUserAvatarResponse user_response;
    auto err_response = [&user_request, &user_response, &response](const std::string &errmsg) -> void
    {
        user_response.set_success(false);
        user_response.set_error(errmsg);
        response.set_content(user_response.SerializeAsString(), "application/x-protbuf");
    };
    bool ret = user_request.ParseFromString(request.body);
    if (ret == false)
    {
        LOG_ERROR("获取用户信息请求正文反序列化失败！");
        return err_response("获取用户信息请求正文反序列化失败！");
    }
    // 2. 客户端身份识别与鉴权
    std::string ssid = user_request.session_id();
    auto uid = _redis_session->Uid(ssid);
    if (!uid)
    {
        LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
        return err_response("获取登录会话关联用户信息失败！");
    }
    user_request.set_user_id(*uid);
    // 2. 将请求转发给用户子服务进行业务处理
    auto channel = _channels->Choose(_user_service_name);
    if (!channel)
    {
        LOG_ERROR("{} 未找到可提供业务处理的用户子服务节点！", user_request.request_id());
        return err_response("未找到可提供业务处理的用户子服务节点！");
    }
    im::UserService_Stub stub(channel.get());
    brpc::Controller cntl;
    stub.SetUserAvatar(&cntl, &user_request, &user_response, nullptr);
    if (cntl.Failed())
    {
        LOG_ERROR("{} 用户子服务调用失败！", user_request.request_id());
        return err_response("用户子服务调用失败！");
    }
    // 3. 得到用户子服务的响应后，将响应内容进行序列化作为http响应正文
    response.set_content(user_response.SerializeAsString(), "application/x-protbuf");
}

void GatewayServer::SetUserNickname(const httplib::Request &request, httplib::Response &response)
{
    // 1. 取出http请求正文，将正文进行反序列化
    SetUserNicknameRequest user_request;
    SetUserNicknameResponse user_response;
    auto err_response = [&user_request, &user_response, &response](const std::string &errmsg) -> void
    {
        user_response.set_success(false);
        user_response.set_error(errmsg);
        response.set_content(user_response.SerializeAsString(), "application/x-protbuf");
    };
    bool ret = user_request.ParseFromString(request.body);
    if (ret == false)
    {
        LOG_ERROR("获取用户信息请求正文反序列化失败！");
        return err_response("获取用户信息请求正文反序列化失败！");
    }
    // 2. 客户端身份识别与鉴权
    std::string ssid = user_request.session_id();
    auto uid = _redis_session->Uid(ssid);
    if (!uid)
    {
        LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
        return err_response("获取登录会话关联用户信息失败！");
    }
    user_request.set_user_id(*uid);
    // 2. 将请求转发给用户子服务进行业务处理
    auto channel = _channels->Choose(_user_service_name);
    if (!channel)
    {
        LOG_ERROR("{} 未找到可提供业务处理的用户子服务节点！", user_request.request_id());
        return err_response("未找到可提供业务处理的用户子服务节点！");
    }
    im::UserService_Stub stub(channel.get());
    brpc::Controller cntl;
    stub.SetUserNickname(&cntl, &user_request, &user_response, nullptr);
    if (cntl.Failed())
    {
        LOG_ERROR("{} 用户子服务调用失败！", user_request.request_id());
        return err_response("用户子服务调用失败！");
    }
    // 3. 得到用户子服务的响应后，将响应内容进行序列化作为http响应正文
    response.set_content(user_response.SerializeAsString(), "application/x-protbuf");
}

void GatewayServer::SetUserDescription(const httplib::Request &request, httplib::Response &response)
{
    // 1. 取出http请求正文，将正文进行反序列化
    SetUserDescriptionRequest user_request;
    SetUserDescriptionResponse user_response;
    auto err_response = [&user_request, &user_response, &response](const std::string &errmsg) -> void
    {
        user_response.set_success(false);
        user_response.set_error(errmsg);
        response.set_content(user_response.SerializeAsString(), "application/x-protbuf");
    };
    bool ret = user_request.ParseFromString(request.body);
    if (ret == false)
    {
        LOG_ERROR("获取用户信息请求正文反序列化失败！");
        return err_response("获取用户信息请求正文反序列化失败！");
    }
    // 2. 客户端身份识别与鉴权
    std::string ssid = user_request.session_id();
    auto uid = _redis_session->Uid(ssid);
    if (!uid)
    {
        LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
        return err_response("获取登录会话关联用户信息失败！");
    }
    user_request.set_user_id(*uid);
    // 2. 将请求转发给用户子服务进行业务处理
    auto channel = _channels->Choose(_user_service_name);
    if (!channel)
    {
        LOG_ERROR("{} 未找到可提供业务处理的用户子服务节点！", user_request.request_id());
        return err_response("未找到可提供业务处理的用户子服务节点！");
    }
    im::UserService_Stub stub(channel.get());
    brpc::Controller cntl;
    stub.SetUserDescription(&cntl, &user_request, &user_response, nullptr);
    if (cntl.Failed())
    {
        LOG_ERROR("{} 用户子服务调用失败！", user_request.request_id());
        return err_response("用户子服务调用失败！");
    }
    // 3. 得到用户子服务的响应后，将响应内容进行序列化作为http响应正文
    response.set_content(user_response.SerializeAsString(), "application/x-protbuf");
}

void GatewayServer::SetUserPhoneNumber(const httplib::Request &request, httplib::Response &response)
{
    // 1. 取出http请求正文，将正文进行反序列化
    SetUserPhoneNumberRequest user_request;
    SetUserPhoneNumberResponse user_response;
    auto err_response = [&user_request, &user_response, &response](const std::string &errmsg) -> void
    {
        user_response.set_success(false);
        user_response.set_error(errmsg);
        response.set_content(user_response.SerializeAsString(), "application/x-protbuf");
    };
    bool ret = user_request.ParseFromString(request.body);
    if (ret == false)
    {
        LOG_ERROR("获取用户信息请求正文反序列化失败！");
        return err_response("获取用户信息请求正文反序列化失败！");
    }
    // 2. 客户端身份识别与鉴权
    std::string ssid = user_request.session_id();
    auto uid = _redis_session->Uid(ssid);
    if (!uid)
    {
        LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
        return err_response("获取登录会话关联用户信息失败！");
    }
    user_request.set_user_id(*uid);
    // 2. 将请求转发给用户子服务进行业务处理
    auto channel = _channels->Choose(_user_service_name);
    if (!channel)
    {
        LOG_ERROR("{} 未找到可提供业务处理的用户子服务节点！", user_request.request_id());
        return err_response("未找到可提供业务处理的用户子服务节点！");
    }
    im::UserService_Stub stub(channel.get());
    brpc::Controller cntl;
    stub.SetUserPhoneNumber(&cntl, &user_request, &user_response, nullptr);
    if (cntl.Failed())
    {
        LOG_ERROR("{} 用户子服务调用失败！", user_request.request_id());
        return err_response("用户子服务调用失败！");
    }
    // 3. 得到用户子服务的响应后，将响应内容进行序列化作为http响应正文
    response.set_content(user_response.SerializeAsString(), "application/x-protbuf");
}

std::shared_ptr<im::GetUserInfoResponse> GatewayServer::GetUserInfoWithRespone(const std::string &rid,
                                                                               const std::string &uid)
{
    GetUserInfoRequest user_request;
    auto user_response = std::make_shared<GetUserInfoResponse>();
    user_request.set_request_id(rid);
    user_request.set_user_id(uid);
    // 2. 将请求转发给用户子服务进行业务处理
    auto channel = _channels->Choose(_user_service_name);
    if (!channel)
    {
        LOG_ERROR("{} 未找到可提供业务处理的用户子服务节点！", user_request.request_id());
        return std::shared_ptr<GetUserInfoResponse>();
    }

    im::UserService_Stub stub(channel.get());
    brpc::Controller cntl;
    stub.GetUserInfo(&cntl, &user_request, user_response.get(), nullptr);
    if (cntl.Failed())
    {
        LOG_ERROR("{} 用户子服务调用失败！", user_request.request_id());
        return std::shared_ptr<GetUserInfoResponse>();
    }

    return user_response;
}

void GatewayServer::FriendAdd(const httplib::Request &request, httplib::Response &response)
{
    // 1. 正文的反序列化，提取关键要素：登录会话ID
    FriendAddRequest friend_request;
    FriendAddResponse friend_response;
    auto err_response = [&friend_request, &friend_response, &response](const std::string &errmsg) -> void
    {
        friend_response.set_success(false);
        friend_response.set_error(errmsg);
        response.set_content(friend_response.SerializeAsString(), "application/x-protbuf");
    };
    bool ret = friend_request.ParseFromString(request.body);
    if (ret == false)
    {
        LOG_ERROR("申请好友请求正文反序列化失败！");
        return err_response("申请好友请求正文反序列化失败！");
    }
    // 2. 客户端身份识别与鉴权
    std::string ssid = friend_request.session_id();
    auto uid = _redis_session->Uid(ssid);
    if (!uid)
    {
        LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
        return err_response("获取登录会话关联用户信息失败！");
    }
    friend_request.set_user_id(*uid);
    // 3. 将请求转发给好友子服务进行业务处理
    auto channel = _channels->Choose(_friend_service_name);
    if (!channel)
    {
        LOG_ERROR("{} 未找到可提供业务处理的用户子服务节点！", friend_request.request_id());
        return err_response("未找到可提供业务处理的用户子服务节点！");
    }
    im::FriendService_Stub stub(channel.get());
    brpc::Controller cntl;
    stub.FriendAdd(&cntl, &friend_request, &friend_response, nullptr);
    if (cntl.Failed())
    {
        LOG_ERROR("{} 好友子服务调用失败！", friend_request.request_id());
        return err_response("好友子服务调用失败！");
    }
    // 4. 若业务处理成功 --- 且获取被申请方长连接成功，则向被申请放进行好友申请事件通知
    auto conn = _connections->GetConnection(friend_request.respondent_id());
    if (friend_response.success() && conn)
    {
        LOG_DEBUG("找到被申请人 {} 长连接，对其进行好友申请通知", friend_request.respondent_id());
        auto user_rsp = GetUserInfoWithRespone(friend_request.request_id(), *uid);
        if (!user_rsp)
        {
            LOG_ERROR("{} 获取当前客户端用户信息失败！", friend_request.request_id());
            return err_response("获取当前客户端用户信息失败！");
        }
        NotifyMessage notify;
        notify.set_notify_type(NotifyType::FRIEND_ADD_APPLY_NOTIFY);
        notify.mutable_friend_add_apply()->mutable_user_info()->CopyFrom(user_rsp->user_info());
        conn->send(notify.SerializeAsString(),
                   websocketpp::frame::opcode::value::binary); // TODO: 需要处理发送失败的情况
    }
    // 5. 向客户端进行响应
    response.set_content(friend_response.SerializeAsString(), "application/x-protbuf");
}

void GatewayServer::FriendAddProcess(const httplib::Request &request, httplib::Response &response)
{
    // 好友申请前的处理
    FriendAddProcessRequest friend_request;
    FriendAddProcessResponse friend_response;
    auto err_response = [&friend_request, &friend_response, &response](const std::string &errmsg) -> void
    {
        friend_response.set_success(false);
        friend_response.set_error(errmsg);
        response.set_content(friend_response.SerializeAsString(), "application/x-protbuf");
    };
    bool ret = friend_request.ParseFromString(request.body);
    if (ret == false)
    {
        LOG_ERROR("申请好友请求正文反序列化失败！");
        return err_response("申请好友请求正文反序列化失败！");
    }
    // 2. 客户端身份识别与鉴权
    std::string ssid = friend_request.session_id();
    auto uid = _redis_session->Uid(ssid);
    if (!uid)
    {
        LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
        return err_response("获取登录会话关联用户信息失败！");
    }
    friend_request.set_user_id(*uid);
    // 3. 将请求转发给好友子服务进行业务处理
    auto channel = _channels->Choose(_friend_service_name);
    if (!channel)
    {
        LOG_ERROR("{} 未找到可提供业务处理的用户子服务节点！", friend_request.request_id());
        return err_response("未找到可提供业务处理的用户子服务节点！");
    }
    im::FriendService_Stub stub(channel.get());
    brpc::Controller cntl;
    stub.FriendAddProcess(&cntl, &friend_request, &friend_response, nullptr);
    if (cntl.Failed())
    {
        LOG_ERROR("{} 好友子服务调用失败！", friend_request.request_id());
        return err_response("好友子服务调用失败！");
    }

    if (friend_response.success())
    {
        auto process_user_rsp = GetUserInfoWithRespone(friend_request.request_id(), *uid);
        if (!process_user_rsp)
        {
            LOG_ERROR("{} 获取用户信息失败！", friend_request.request_id());
            return err_response("获取用户信息失败！");
        }
        auto apply_user_rsp = GetUserInfoWithRespone(friend_request.request_id(), friend_request.apply_user_id());

        if (!process_user_rsp)
        {
            LOG_ERROR("{} 获取用户信息失败！", friend_request.request_id());
            return err_response("获取用户信息失败！");
        }

        auto process_conn = _connections->GetConnection(*uid);
        if (process_conn)
            LOG_DEBUG("找到处理人的长连接！");
        else
            LOG_DEBUG("未找到处理人的长连接！");

        auto apply_conn = _connections->GetConnection(friend_request.apply_user_id());
        if (apply_conn)
            LOG_DEBUG("找到申请人的长连接！");
        else
            LOG_DEBUG("未找到申请人的长连接！");
        // 4. 将处理结果给申请人进行通知
        if (apply_conn)
        {
            NotifyMessage notify;
            notify.set_notify_type(NotifyType::FRIEND_ADD_PROCESS_NOTIFY);
            auto process_result = notify.mutable_friend_process_result();
            process_result->mutable_user_info()->CopyFrom(process_user_rsp->user_info());
            process_result->set_agree(friend_request.agree());
            apply_conn->send(notify.SerializeAsString(), websocketpp::frame::opcode::value::binary);
            LOG_DEBUG("对申请人进行申请处理结果通知！");
        }
        // 5. 若处理结果是同意 --- 会伴随着单聊会话的创建 -- 因此需要对双方进行会话创建的通知
        if (friend_request.agree() && apply_conn)
        { // 对申请人的通知---会话信息就是处理人信息
            NotifyMessage notify;
            notify.set_notify_type(NotifyType::CHAT_SESSION_CREATE_NOTIFY);
            auto chat_session = notify.mutable_new_chat_session_info();
            chat_session->mutable_chat_session_info()->set_single_chat_friend_id(*uid);
            chat_session->mutable_chat_session_info()->set_chat_session_id(friend_response.new_session_id());
            chat_session->mutable_chat_session_info()->set_chat_session_name(process_user_rsp->user_info().nickname());
            chat_session->mutable_chat_session_info()->set_avatar(process_user_rsp->user_info().avatar());
            apply_conn->send(notify.SerializeAsString(), websocketpp::frame::opcode::value::binary);
            LOG_DEBUG("对申请人进行会话创建通知！");
        }
        if (friend_request.agree() && process_conn)
        { // 对处理人的通知 --- 会话信息就是申请人信息
            NotifyMessage notify;
            notify.set_notify_type(NotifyType::CHAT_SESSION_CREATE_NOTIFY);
            auto chat_session = notify.mutable_new_chat_session_info();
            chat_session->mutable_chat_session_info()->set_single_chat_friend_id(friend_request.apply_user_id());
            chat_session->mutable_chat_session_info()->set_chat_session_id(friend_response.new_session_id());
            chat_session->mutable_chat_session_info()->set_chat_session_name(apply_user_rsp->user_info().nickname());
            chat_session->mutable_chat_session_info()->set_avatar(apply_user_rsp->user_info().avatar());
            process_conn->send(notify.SerializeAsString(), websocketpp::frame::opcode::value::binary);
            LOG_DEBUG("对处理人进行会话创建通知！");
        }
    }
    // 6. 对客户端进行响应
    response.set_content(friend_response.SerializeAsString(), "application/x-protbuf");
}

void GatewayServer::FriendRemove(const httplib::Request &request, httplib::Response &response)
{
                    // 1. 正文的反序列化，提取关键要素：登录会话ID
                FriendRemoveRequest req;
                FriendRemoveResponse rsp;
                auto err_response = [&req, &rsp, &response](const std::string &errmsg) -> void {
                    rsp.set_success(false);
                    rsp.set_error(errmsg);
                    response.set_content(rsp.SerializeAsString(), "application/x-protbuf");
                };
                bool ret = req.ParseFromString(request.body);
                if (ret == false) {
                    LOG_ERROR("删除好友请求正文反序列化失败！");
                    return err_response("删除好友请求正文反序列化失败！");
                }
                // 2. 客户端身份识别与鉴权
                std::string ssid = req.session_id();
                auto uid = _redis_session->Uid(ssid);
                if (!uid) {
                    LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
                    return err_response("获取登录会话关联用户信息失败！");
                }
                req.set_user_id(*uid);
                // 3. 将请求转发给好友子服务进行业务处理
                auto channel = _channels->Choose(_friend_service_name);
                if (!channel) {
                    LOG_ERROR("{} 未找到可提供业务处理的用户子服务节点！", req.request_id());
                    return err_response("未找到可提供业务处理的用户子服务节点！");
                }
                im::FriendService_Stub stub(channel.get());
                brpc::Controller cntl;
                stub.FriendRemove(&cntl, &req, &rsp, nullptr);
                if (cntl.Failed()) {
                    LOG_ERROR("{} 好友子服务调用失败！", req.request_id());
                    return err_response("好友子服务调用失败！");
                }
                // 4. 若业务处理成功 --- 且获取被申请方长连接成功，则向被申请放进行好友申请事件通知
                auto conn = _connections->GetConnection(req.peer_id());
                if (rsp.success() && conn) {
                    LOG_ERROR("对被删除人 {} 进行好友删除通知！", req.peer_id());
                    NotifyMessage notify;
                    notify.set_notify_type(NotifyType::FRIEND_REMOVE_NOTIFY);
                    notify.mutable_friend_remove()->set_user_id(*uid);
                    conn->send(notify.SerializeAsString(), websocketpp::frame::opcode::value::binary);
                }
                // 5. 向客户端进行响应
                response.set_content(rsp.SerializeAsString(), "application/x-protbuf");

}

void GatewayServer::FriendSearch(const httplib::Request &request, httplib::Response &response)
{
    GetPendingFriendEventListRequest req;
    GetPendingFriendEventListResponse rsp;
    auto err_response = [&req, &rsp, &response](const std::string &errmsg) -> void {
        rsp.set_success(false);
        rsp.set_error(errmsg);
        response.set_content(rsp.SerializeAsString(), "application/x-protbuf");
    };
    bool ret = req.ParseFromString(request.body);
    if (ret == false) {
        LOG_ERROR("获取待处理好友申请请求正文反序列化失败！");
        return err_response("获取待处理好友申请请求正文反序列化失败！");
    }
    // 2. 客户端身份识别与鉴权
    std::string ssid = req.session_id();
    auto uid = _redis_session->Uid(ssid);
    if (!uid) {
        LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
        return err_response("获取登录会话关联用户信息失败！");
    }
    req.set_user_id(*uid);
    // 3. 将请求转发给好友子服务进行业务处理
    auto channel = _channels->Choose(_friend_service_name);
    if (!channel) {
        LOG_ERROR("{} 未找到可提供业务处理的用户子服务节点！", req.request_id());
        return err_response("未找到可提供业务处理的用户子服务节点！");
    }
    im::FriendService_Stub stub(channel.get());
    brpc::Controller cntl;
    stub.GetPendingFriendEventList(&cntl, &req, &rsp, nullptr);
    if (cntl.Failed()) {
        LOG_ERROR("{} 好友子服务调用失败！", req.request_id());
        return err_response("好友子服务调用失败！");
    }
    // 5. 向客户端进行响应
    response.set_content(rsp.SerializeAsString(), "application/x-protbuf");

}

void GatewayServer::GetFriendList(const httplib::Request &request, httplib::Response &response)
{
    //1. 取出http请求正文，将正文进行反序列化
    GetFriendListRequest req;
    GetFriendListResponse rsp;
    auto err_response = [&req, &rsp, &response](const std::string &errmsg) -> void {
        rsp.set_success(false);
        rsp.set_error(errmsg);
        response.set_content(rsp.SerializeAsString(), "application/x-protbuf");
    };
    bool ret = req.ParseFromString(request.body);
    if (ret == false) {
        LOG_ERROR("获取好友列表请求正文反序列化失败！");
        return err_response("获取好友列表请求正文反序列化失败！");
    }
    //2. 客户端身份识别与鉴权
    std::string ssid = req.session_id();
    auto uid = _redis_session->Uid(ssid);
    if (!uid) {
        LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
        return err_response("获取登录会话关联用户信息失败！");
    }
    req.set_user_id(*uid);
    //2. 将请求转发给好友子服务进行业务处理
    auto channel = _channels->Choose(_friend_service_name);
    if (!channel) {
        LOG_ERROR("{} 未找到可提供业务处理的用户子服务节点！", req.request_id());
        return err_response("未找到可提供业务处理的用户子服务节点！");
    }
    im::FriendService_Stub stub(channel.get());
    brpc::Controller cntl;
    stub.GetFriendList(&cntl, &req, &rsp, nullptr);
    if (cntl.Failed()) {
        LOG_ERROR("{} 好友子服务调用失败！", req.request_id());
        return err_response("好友子服务调用失败！");
    }
    //3. 得到用户子服务的响应后，将响应内容进行序列化作为http响应正文
    response.set_content(rsp.SerializeAsString(), "application/x-protbuf");

}

void GatewayServer::GetPendingFriendEventList(const httplib::Request &request, httplib::Response &response)
{
    GetPendingFriendEventListRequest req;
    GetPendingFriendEventListResponse rsp;
    auto err_response = [&req, &rsp, &response](const std::string &errmsg) -> void {
        rsp.set_success(false);
        rsp.set_error(errmsg);
        response.set_content(rsp.SerializeAsString(), "application/x-protbuf");
    };
    bool ret = req.ParseFromString(request.body);
    if (ret == false) {
        LOG_ERROR("获取待处理好友申请请求正文反序列化失败！");
        return err_response("获取待处理好友申请请求正文反序列化失败！");
    }
    // 2. 客户端身份识别与鉴权
    std::string ssid = req.session_id();
    auto uid = _redis_session->Uid(ssid);
    if (!uid) {
        LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
        return err_response("获取登录会话关联用户信息失败！");
    }
    req.set_user_id(*uid);
    // 3. 将请求转发给好友子服务进行业务处理
    auto channel = _channels->Choose(_friend_service_name);
    if (!channel) {
        LOG_ERROR("{} 未找到可提供业务处理的用户子服务节点！", req.request_id());
        return err_response("未找到可提供业务处理的用户子服务节点！");
    }
    im::FriendService_Stub stub(channel.get());
    brpc::Controller cntl;
    stub.GetPendingFriendEventList(&cntl, &req, &rsp, nullptr);
    if (cntl.Failed()) {
        LOG_ERROR("{} 好友子服务调用失败！", req.request_id());
        return err_response("好友子服务调用失败！");
    }
    // 5. 向客户端进行响应
    response.set_content(rsp.SerializeAsString(), "application/x-protbuf");

}

void GatewayServer::GetChatSessionList(const httplib::Request &request, httplib::Response &response)
{
    GetChatSessionListRequest req;
    GetChatSessionListResponse rsp;
    auto err_response = [&req, &rsp, &response](const std::string &errmsg) -> void {
        rsp.set_success(false);
        rsp.set_error(errmsg);
        response.set_content(rsp.SerializeAsString(), "application/x-protbuf");
    };
    bool ret = req.ParseFromString(request.body);
    if (ret == false) {
        LOG_ERROR("获取聊天会话列表请求正文反序列化失败！");
        return err_response("获取聊天会话列表请求正文反序列化失败！");
    }
    // 2. 客户端身份识别与鉴权
    std::string ssid = req.session_id();
    auto uid = _redis_session->Uid(ssid);
    if (!uid) {
        LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
        return err_response("获取登录会话关联用户信息失败！");
    }
    req.set_user_id(*uid);
    // 3. 将请求转发给好友子服务进行业务处理
    auto channel = _channels->Choose(_friend_service_name);
    if (!channel) {
        LOG_ERROR("{} 未找到可提供业务处理的用户子服务节点！", req.request_id());
        return err_response("未找到可提供业务处理的用户子服务节点！");
    }
    im::FriendService_Stub stub(channel.get());
    brpc::Controller cntl;
    stub.GetChatSessionList(&cntl, &req, &rsp, nullptr);
    if (cntl.Failed()) {
        LOG_ERROR("{} 好友子服务调用失败！", req.request_id());
        return err_response("好友子服务调用失败！");
    }
    // 5. 向客户端进行响应
    response.set_content(rsp.SerializeAsString(), "application/x-protbuf");

}

void GatewayServer::GetChatSessionMember(const httplib::Request &request, httplib::Response &response)
{
    GetChatSessionMemberRequest req;
    GetChatSessionMemberResponse rsp;
    auto err_response = [&req, &rsp, &response](const std::string &errmsg) -> void {
        rsp.set_success(false);
        rsp.set_error(errmsg);
        response.set_content(rsp.SerializeAsString(), "application/x-protbuf");
    };
    bool ret = req.ParseFromString(request.body);
    if (ret == false) {
        LOG_ERROR("获取聊天会话成员请求正文反序列化失败！");
        return err_response("获取聊天会话成员请求正文反序列化失败！");
    }
    // 2. 客户端身份识别与鉴权
    std::string ssid = req.session_id();
    auto uid = _redis_session->Uid(ssid);
    if (!uid) {
        LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
        return err_response("获取登录会话关联用户信息失败！");
    }
    req.set_user_id(*uid);
    // 3. 将请求转发给好友子服务进行业务处理
    auto channel = _channels->Choose(_friend_service_name);
    if (!channel) {
        LOG_ERROR("{} 未找到可提供业务处理的用户子服务节点！", req.request_id());
        return err_response("未找到可提供业务处理的用户子服务节点！");
    }
    im::FriendService_Stub stub(channel.get());
    brpc::Controller cntl;
    stub.GetChatSessionMember(&cntl, &req, &rsp, nullptr);
    if (cntl.Failed()) {
        LOG_ERROR("{} 好友子服务调用失败！", req.request_id());
        return err_response("好友子服务调用失败！");
    }
    // 5. 向客户端进行响应
    response.set_content(rsp.SerializeAsString(), "application/x-protbuf");
}

void GatewayServer::ChatSessionCreate(const httplib::Request &request, httplib::Response &response)
{
                    ChatSessionCreateRequest req;
                ChatSessionCreateResponse rsp;
                auto err_response = [&req, &rsp, &response](const std::string &errmsg) -> void {
                    rsp.set_success(false);
                    rsp.set_error(errmsg);
                    response.set_content(rsp.SerializeAsString(), "application/x-protbuf");
                };
                bool ret = req.ParseFromString(request.body);
                if (ret == false) {
                    LOG_ERROR("创建聊天会话请求正文反序列化失败！");
                    return err_response("创建聊天会话请求正文反序列化失败！");
                }
                // 2. 客户端身份识别与鉴权
                std::string ssid = req.session_id();
                auto uid = _redis_session->Uid(ssid);
                if (!uid) {
                    LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
                    return err_response("获取登录会话关联用户信息失败！");
                }
                req.set_user_id(*uid);
                // 3. 将请求转发给好友子服务进行业务处理
                auto channel = _channels->Choose(_friend_service_name);
                if (!channel) {
                    LOG_ERROR("{} 未找到可提供业务处理的用户子服务节点！", req.request_id());
                    return err_response("未找到可提供业务处理的用户子服务节点！");
                }
                im::FriendService_Stub stub(channel.get());
                brpc::Controller cntl;
                stub.ChatSessionCreate(&cntl, &req, &rsp, nullptr);
                if (cntl.Failed()) {
                    LOG_ERROR("{} 好友子服务调用失败！", req.request_id());
                    return err_response("好友子服务调用失败！");
                }
                // 4. 若业务处理成功 --- 且获取被申请方长连接成功，则向被申请放进行好友申请事件通知
                if (rsp.success()){
                    for (int i = 0; i < req.member_id_list_size(); i++) {
                        auto conn = _connections->GetConnection(req.member_id_list(i));
                        if (!conn) {
                            LOG_DEBUG("未找到群聊成员 {} 长连接", req.member_id_list(i));
                            continue;
                        }
                        NotifyMessage notify;
                        notify.set_notify_type(NotifyType::CHAT_SESSION_CREATE_NOTIFY);
                        auto chat_session = notify.mutable_new_chat_session_info();
                        chat_session->mutable_chat_session_info()->CopyFrom(rsp.chat_session_info());
                        conn->send(notify.SerializeAsString(), websocketpp::frame::opcode::value::binary);
                        LOG_DEBUG("对群聊成员 {} 进行会话创建通知", req.member_id_list(i));
                    }
                }
                // 5. 向客户端进行响应
                rsp.clear_chat_session_info();
                response.set_content(rsp.SerializeAsString(), "application/x-protbuf");

}

void GatewayServer::GetHistoryMessage(const httplib::Request &request, httplib::Response &response)
{
                    GetHistoryMessageRequest req;
                GetHistoryMessageResponse rsp;
                auto err_response = [&req, &rsp, &response](const std::string &errmsg) -> void {
                    rsp.set_success(false);
                    rsp.set_error(errmsg);
                    response.set_content(rsp.SerializeAsString(), "application/x-protbuf");
                };
                bool ret = req.ParseFromString(request.body);
                if (ret == false) {
                    LOG_ERROR("获取区间消息请求正文反序列化失败！");
                    return err_response("获取区间消息请求正文反序列化失败！");
                }
                // 2. 客户端身份识别与鉴权
                std::string ssid = req.session_id();
                auto uid = _redis_session->Uid(ssid);
                if (!uid) {
                    LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
                    return err_response("获取登录会话关联用户信息失败！");
                }
                req.set_user_id(*uid);
                // 3. 将请求转发给好友子服务进行业务处理
                auto channel = _channels->Choose(_message_service_name);
                if (!channel) {
                    LOG_ERROR("{} 未找到可提供业务处理的用户子服务节点！", req.request_id());
                    return err_response("未找到可提供业务处理的用户子服务节点！");
                }
                im::MessageStorageService_Stub stub(channel.get());
                brpc::Controller cntl;
                stub.GetHistoryMessage(&cntl, &req, &rsp, nullptr);
                if (cntl.Failed()) {
                    LOG_ERROR("{} 消息存储子服务调用失败！", req.request_id());
                    return err_response("消息存储子服务调用失败！");
                }
                // 5. 向客户端进行响应
                response.set_content(rsp.SerializeAsString(), "application/x-protbuf");
}

void GatewayServer::GetRecentMessage(const httplib::Request &request, httplib::Response &response)
{
    GetRecentMessageRequest req;
    GetRecentMessageResponse rsp;
    auto err_response = [&req, &rsp, &response](const std::string &errmsg) -> void {
        rsp.set_success(false);
        rsp.set_error(errmsg);
        response.set_content(rsp.SerializeAsString(), "application/x-protbuf");
    };
    bool ret = req.ParseFromString(request.body);
    if (ret == false) {
        LOG_ERROR("获取最近消息请求正文反序列化失败！");
        return err_response("获取最近消息请求正文反序列化失败！");
    }
    // 2. 客户端身份识别与鉴权
    std::string ssid = req.session_id();
    auto uid = _redis_session->Uid(ssid);
    if (!uid) {
        LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
        return err_response("获取登录会话关联用户信息失败！");
    }
    req.set_user_id(*uid);
    // 3. 将请求转发给好友子服务进行业务处理
    auto channel = _channels->Choose(_message_service_name);
    if (!channel) {
        LOG_ERROR("{} 未找到可提供业务处理的用户子服务节点！", req.request_id());
        return err_response("未找到可提供业务处理的用户子服务节点！");
    }
    im::MessageStorageService_Stub stub(channel.get());
    brpc::Controller cntl;
    stub.GetRecentMessage(&cntl, &req, &rsp, nullptr);
    if (cntl.Failed()) {
        LOG_ERROR("{} 消息存储子服务调用失败！", req.request_id());
        return err_response("消息存储子服务调用失败！");
    }
    // 5. 向客户端进行响应
    response.set_content(rsp.SerializeAsString(), "application/x-protbuf");

}

void GatewayServer::SearchMessage(const httplib::Request &request, httplib::Response &response)
{
    MessageSearchRequest req;
    MessageSearchResponse rsp;
    auto err_response = [&req, &rsp, &response](const std::string &errmsg) -> void {
        rsp.set_success(false);
        rsp.set_error(errmsg);
        response.set_content(rsp.SerializeAsString(), "application/x-protbuf");
    };
    bool ret = req.ParseFromString(request.body);
    if (ret == false) {
        LOG_ERROR("消息搜索请求正文反序列化失败！");
        return err_response("消息搜索请求正文反序列化失败！");
    }
    // 2. 客户端身份识别与鉴权
    std::string ssid = req.session_id();
    auto uid = _redis_session->Uid(ssid);
    if (!uid) {
        LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
        return err_response("获取登录会话关联用户信息失败！");
    }
    req.set_user_id(*uid);
    // 3. 将请求转发给好友子服务进行业务处理
    auto channel = _channels->Choose(_message_service_name);
    if (!channel) {
        LOG_ERROR("{} 未找到可提供业务处理的用户子服务节点！", req.request_id());
        return err_response("未找到可提供业务处理的用户子服务节点！");
    }
    im::MessageStorageService_Stub stub(channel.get());
    brpc::Controller cntl;
    stub.MessageSearch(&cntl, &req, &rsp, nullptr);
    if (cntl.Failed()) {
        LOG_ERROR("{} 消息存储子服务调用失败！", req.request_id());
        return err_response("消息存储子服务调用失败！");
    }
    // 5. 向客户端进行响应
    response.set_content(rsp.SerializeAsString(), "application/x-protbuf");
}

void GatewayServer::NewMessage(const httplib::Request &request, httplib::Response &response)
{
                NewMessageRequest req;
                NewMessageResponse rsp;//这是给客户端的响应
                GetTransmitTargetResponse target_rsp;//这是请求子服务的响应
                auto err_response = [&req, &rsp, &response](const std::string &errmsg) -> void {
                    rsp.set_success(false);
                    rsp.set_error(errmsg);
                    response.set_content(rsp.SerializeAsString(), "application/x-protbuf");
                };
                bool ret = req.ParseFromString(request.body);
                if (ret == false) {
                    LOG_ERROR("新消息请求正文反序列化失败！");
                    return err_response("新消息请求正文反序列化失败！");
                }
                // 2. 客户端身份识别与鉴权
                std::string ssid = req.session_id();
                auto uid = _redis_session->Uid(ssid);
                if (!uid) {
                    LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
                    return err_response("获取登录会话关联用户信息失败！");
                }
                req.set_user_id(*uid);
                // 3. 将请求转发给好友子服务进行业务处理
                auto channel = _channels->Choose(_transmite_service_name);
                if (!channel) {
                    LOG_ERROR("{} 未找到可提供业务处理的用户子服务节点！", req.request_id());
                    return err_response("未找到可提供业务处理的用户子服务节点！");
                }
                im::MessageTransmitService_Stub stub(channel.get());
                brpc::Controller cntl;
                stub.GetTransmitTarget(&cntl, &req, &target_rsp, nullptr);
                if (cntl.Failed()) {
                    LOG_ERROR("{} 消息转发子服务调用失败！", req.request_id());
                    return err_response("消息转发子服务调用失败！");
                }
                // 4. 若业务处理成功 --- 且获取被申请方长连接成功，则向被申请放进行好友申请事件通知
                if (target_rsp.success()){
                    for (int i = 0; i < target_rsp.target_id_list_size(); i++) {
                        std::string notify_uid = target_rsp.target_id_list(i);
                        if (notify_uid == *uid) continue; //不通知自己
                        auto conn = _connections->GetConnection(notify_uid);
                        if (!conn) { continue;}
                        NotifyMessage notify;
                        notify.set_notify_type(NotifyType::CHAT_MESSAGE_NOTIFY);
                        auto msg_info = notify.mutable_new_message_info();
                        msg_info->mutable_message_info()->CopyFrom(target_rsp.message());
                        conn->send(notify.SerializeAsString(), websocketpp::frame::opcode::value::binary);
                    }
                }
                // 5. 向客户端进行响应
                rsp.set_request_id(req.request_id());
                rsp.set_success(target_rsp.success());
                rsp.set_error(target_rsp.error());
                response.set_content(rsp.SerializeAsString(), "application/x-protbuf");

}

void GatewayServer::GetSingleFile(const httplib::Request &request, httplib::Response &response)
{
    GetSingleFileRequest req;
    GetSingleFileResponse rsp;
    auto err_response = [&req, &rsp, &response](const std::string &errmsg) -> void {
        rsp.set_success(false);
        rsp.set_error(errmsg);
        response.set_content(rsp.SerializeAsString(), "application/x-protbuf");
    };
    bool ret = req.ParseFromString(request.body);
    if (ret == false) {
        LOG_ERROR("单文件下载请求正文反序列化失败！");
        return err_response("单文件下载请求正文反序列化失败！");
    }
    // 2. 客户端身份识别与鉴权
    std::string ssid = req.session_id();
    auto uid = _redis_session->Uid(ssid);
    if (!uid) {
        LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
        return err_response("获取登录会话关联用户信息失败！");
    }
    req.set_user_id(*uid);
    // 3. 将请求转发给好友子服务进行业务处理
    auto channel = _channels->Choose(_file_service_name);
    if (!channel) {
        LOG_ERROR("{} 未找到可提供业务处理的用户子服务节点！", req.request_id());
        return err_response("未找到可提供业务处理的用户子服务节点！");
    }
    im::FileService_Stub stub(channel.get());
    brpc::Controller cntl;
    stub.GetSingleFile(&cntl, &req, &rsp, nullptr);
    if (cntl.Failed()) {
        LOG_ERROR("{} 文件存储子服务调用失败！", req.request_id());
        return err_response("文件存储子服务调用失败！");
    }
    // 5. 向客户端进行响应
    response.set_content(rsp.SerializeAsString(), "application/x-protbuf");

}

void GatewayServer::GetMultiFile(const httplib::Request &request, httplib::Response &response)
{
    GetMultiFileRequest req;
    GetMultiFileResponse rsp;
    auto err_response = [&req, &rsp, &response](const std::string &errmsg) -> void {
        rsp.set_success(false);
        rsp.set_error(errmsg);
        response.set_content(rsp.SerializeAsString(), "application/x-protbuf");
    };
    bool ret = req.ParseFromString(request.body);
    if (ret == false) {
        LOG_ERROR("单文件下载请求正文反序列化失败！");
        return err_response("单文件下载请求正文反序列化失败！");
    }
    // 2. 客户端身份识别与鉴权
    std::string ssid = req.session_id();
    auto uid = _redis_session->Uid(ssid);
    if (!uid) {
        LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
        return err_response("获取登录会话关联用户信息失败！");
    }
    req.set_user_id(*uid);
    // 3. 将请求转发给好友子服务进行业务处理
    auto channel = _channels->Choose(_file_service_name);
    if (!channel) {
        LOG_ERROR("{} 未找到可提供业务处理的用户子服务节点！", req.request_id());
        return err_response("未找到可提供业务处理的用户子服务节点！");
    }
    im::FileService_Stub stub(channel.get());
    brpc::Controller cntl;
    stub.GetMultiFile(&cntl, &req, &rsp, nullptr);
    if (cntl.Failed()) {
        LOG_ERROR("{} 文件存储子服务调用失败！", req.request_id());
        return err_response("文件存储子服务调用失败！");
    }
    // 5. 向客户端进行响应
    response.set_content(rsp.SerializeAsString(), "application/x-protbuf");

}

void GatewayServer::PutSingleFile(const httplib::Request &request, httplib::Response &response)
{
    PutSingleFileRequest req;
    PutSingleFileResponse rsp;
    auto err_response = [&req, &rsp, &response](const std::string &errmsg) -> void {
        rsp.set_success(false);
        rsp.set_error(errmsg);
        response.set_content(rsp.SerializeAsString(), "application/x-protbuf");
    };
    bool ret = req.ParseFromString(request.body);
    if (ret == false) {
        LOG_ERROR("单文件上传请求正文反序列化失败！");
        return err_response("单文件上传请求正文反序列化失败！");
    }
    // 2. 客户端身份识别与鉴权
    std::string ssid = req.session_id();
    auto uid = _redis_session->Uid(ssid);
    if (!uid) {
        LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
        return err_response("获取登录会话关联用户信息失败！");
    }
    req.set_user_id(*uid);
    // 3. 将请求转发给好友子服务进行业务处理
    auto channel = _channels->Choose(_file_service_name);
    if (!channel) {
        LOG_ERROR("{} 未找到可提供业务处理的用户子服务节点！", req.request_id());
        return err_response("未找到可提供业务处理的用户子服务节点！");
    }
    im::FileService_Stub stub(channel.get());
    brpc::Controller cntl;
    stub.PutSingleFile(&cntl, &req, &rsp, nullptr);
    if (cntl.Failed()) {
        LOG_ERROR("{} 文件存储子服务调用失败！", req.request_id());
        return err_response("文件存储子服务调用失败！");
    }
    // 5. 向客户端进行响应
    response.set_content(rsp.SerializeAsString(), "application/x-protbuf");

}

void GatewayServer::PutMultiFile(const httplib::Request &request, httplib::Response &response)
{
    PutMultiFileRequest req;
    PutMultiFileResponse rsp;
    auto err_response = [&req, &rsp, &response](const std::string &errmsg) -> void {
        rsp.set_success(false);
        rsp.set_error(errmsg);
        response.set_content(rsp.SerializeAsString(), "application/x-protbuf");
    };
    bool ret = req.ParseFromString(request.body);
    if (ret == false) {
        LOG_ERROR("批量文件上传请求正文反序列化失败！");
        return err_response("批量文件上传请求正文反序列化失败！");
    }
    // 2. 客户端身份识别与鉴权
    std::string ssid = req.session_id();
    auto uid = _redis_session->Uid(ssid);
    if (!uid) {
        LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
        return err_response("获取登录会话关联用户信息失败！");
    }
    req.set_user_id(*uid);
    // 3. 将请求转发给好友子服务进行业务处理
    auto channel = _channels->Choose(_file_service_name);
    if (!channel) {
        LOG_ERROR("{} 未找到可提供业务处理的用户子服务节点！", req.request_id());
        return err_response("未找到可提供业务处理的用户子服务节点！");
    }
    im::FileService_Stub stub(channel.get());
    brpc::Controller cntl;
    stub.PutMultiFile(&cntl, &req, &rsp, nullptr);
    if (cntl.Failed()) {
        LOG_ERROR("{} 文件存储子服务调用失败！", req.request_id());
        return err_response("文件存储子服务调用失败！");
    }
    // 5. 向客户端进行响应
    response.set_content(rsp.SerializeAsString(), "application/x-protbuf");

}

void GatewayServer::SpeechRecognition(const httplib::Request &request, httplib::Response &response)
{
    LOG_DEBUG("收到语音转文字请求！");
    SpeechRecognitionRequest req;
    SpeechRecognitionResponse rsp;
    auto err_response = [&req, &rsp, &response](const std::string &errmsg) -> void {
        rsp.set_success(false);
        rsp.set_error(errmsg);
        response.set_content(rsp.SerializeAsString(), "application/x-protbuf");
    };
    bool ret = req.ParseFromString(request.body);
    if (ret == false) {
        LOG_ERROR("语音识别请求正文反序列化失败！");
        return err_response("语音识别请求正文反序列化失败！");
    }
    // 2. 客户端身份识别与鉴权
    std::string ssid = req.session_id();
    auto uid = _redis_session->Uid(ssid);
    if (!uid) {
        LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
        return err_response("获取登录会话关联用户信息失败！");
    }
    req.set_user_id(*uid);
    // 3. 将请求转发给好友子服务进行业务处理
    auto channel = _channels->Choose(_speech_service_name);
    if (!channel) {
        LOG_ERROR("{} 未找到可提供业务处理的用户子服务节点！", req.request_id());
        return err_response("未找到可提供业务处理的用户子服务节点！");
    }
    im::SpeechService_Stub stub(channel.get());
    brpc::Controller cntl;
    stub.SpeechRecognition(&cntl, &req, &rsp, nullptr);
    if (cntl.Failed()) {
        LOG_ERROR("{} 语音识别子服务调用失败！", req.request_id());
        return err_response("语音识别子服务调用失败！");
    }
    // 5. 向客户端进行响应
    response.set_content(rsp.SerializeAsString(), "application/x-protbuf");

}
void GatewayServer::Start()
{
    _ws_server.run();
}

void GatewayServerBuilder::MakeRedisObject(const std::string &host, int port, int db, bool keepAlive)
{
    _redis_client = RedisClientFactory::Create(host, port, db, keepAlive);
}

void GatewayServerBuilder::MakeDiscoveryObject(const std::string &reg_host, const std::string &base_service_name,
                                               const std::string &file_service_name,
                                               const std::string &speech_service_name,
                                               const std::string &message_service_name,
                                               const std::string &friend_service_name,
                                               const std::string &user_service_name,
                                               const std::string &transmite_service_name)
{
    _file_service_name = file_service_name;
    _speech_service_name = speech_service_name;
    _message_service_name = message_service_name;
    _friend_service_name = friend_service_name;
    _user_service_name = user_service_name;
    _transmite_service_name = transmite_service_name;

    _channels = std::make_shared<ServiceManager>();
    _channels->Declared(_file_service_name);
    _channels->Declared(_speech_service_name);
    _channels->Declared(_message_service_name);
    _channels->Declared(_friend_service_name);
    _channels->Declared(_user_service_name);
    _channels->Declared(_transmite_service_name);

    auto channels = _channels.get();

    Discovery::NotifyCallback callback =
            std::bind(&ServiceManager::OnServiceOnline, channels, std::placeholders::_1, std::placeholders::_2);
    auto put_cb = [channels](const std::string &arg1, const std::string &arg2)
    { channels->OnServiceOnline(arg1, arg2); };
    auto del_cb = [channels](const std::string &arg1, const std::string &arg2)
    { channels->OnServiceOffline(arg1, arg2); };

    _service_discoverer = std::make_shared<Discovery>(reg_host, base_service_name, put_cb, del_cb);
}

void GatewayServerBuilder::MakeServerObject(int websocket_port, int http_port)
{
    _websocket_port = websocket_port;
    _http_port = http_port;
}
std::shared_ptr<GatewayServer> GatewayServerBuilder::Build()
{
    if (!_redis_client) {
        LOG_ERROR("还未初始化Redis客户端模块！");
        abort();
    }
    if (!_service_discoverer) {
        LOG_ERROR("还未初始化服务发现模块！");
        abort();
    }
    if (!_channels) {
        LOG_ERROR("还未初始化信道管理模块！");
        abort();
    }
    GatewayServerPtr server = std::make_shared<GatewayServer>(
        _websocket_port, _http_port, _redis_client, _channels,
        _service_discoverer, _user_service_name, _file_service_name,
        _speech_service_name, _message_service_name,
        _transmite_service_name, _friend_service_name);
    return server;
}
