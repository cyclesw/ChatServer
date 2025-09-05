//
// Created by 19396 on 25-5-15.
//

#ifndef SERVER_H
#define SERVER_H

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/roles/server_endpoint.hpp>

#define GET_PHONE_VERIFY_CODE "/service/user/get_phone_verify_code"
#define USERNAME_REGISTER "/service/user/username_register"
#define USERNAME_LOGIN "/service/user/username_login"
#define PHONE_REGISTER "/service/user/phone_register"
#define PHONE_LOGIN "/service/user/phone_login"
#define GET_USERINFO "/service/user/get_user_info"
#define SET_USER_AVATAR "/service/user/set_avatar"
#define SET_USER_NICKNAME "/service/user/set_nickname"
#define SET_USER_DESC "/service/user/set_description"
#define SET_USER_PHONE "/service/user/set_phone"
#define FRIEND_GET_LIST "/service/friend/get_friend_list"
#define FRIEND_APPLY "/service/friend/add_friend_apply"
#define FRIEND_APPLY_PROCESS "/service/friend/add_friend_process"
#define FRIEND_REMOVE "/service/friend/remove_friend"
#define FRIEND_SEARCH "/service/friend/search_friend"
#define FRIEND_GET_PENDING_EV "/service/friend/get_pending_friend_events"
#define CSS_GET_LIST "/service/friend/get_chat_session_list"
#define CSS_CREATE "/service/friend/create_chat_session"
#define CSS_GET_MEMBER "/service/friend/get_chat_session_member"
#define MSG_GET_RANGE "/service/message_storage/get_history"
#define MSG_GET_RECENT "/service/message_storage/get_recent"
#define MSG_KEY_SEARCH "/service/message_storage/search_history"
#define NEW_MESSAGE "/service/message_transmit/new_message"
#define FILE_GET_SINGLE "/service/file/get_single_file"
#define FILE_GET_MULTI "/service/file/get_multi_file"
#define FILE_PUT_SINGLE "/service/file/put_single_file"
#define FILE_PUT_MULTI "/service/file/put_multi_file"
#define SPEECH_RECOGNITION "/service/speech/recognition"

namespace sw::redis
{
class Redis;
}

namespace im
{
class GetUserInfoResponse;

}

namespace httplib
{
class Server;
struct Response;
struct Request;
} // namespace httplib

namespace im
{
class Status;
class Session;
class Discovery;
class ServiceManager;
class Connection;
class GatewayServer;

using server_t = websocketpp::server<websocketpp::config::asio>;

using GatewayServerPtr = std::shared_ptr<GatewayServer>;


class GatewayServer
{
  public:
    GatewayServer(int websocket_port, int http_port, const std::shared_ptr<sw::redis::Redis> &redis_client,
                  const std::shared_ptr<ServiceManager> &channels, const std::shared_ptr<Discovery> &service_discoverer,
                  const std::string &user_service_name, const std::string &file_service_name,
                  const std::string &speech_service_name, const std::string &message_service_name,
                  const std::string &transmite_service_name, const std::string &friend_service_name);

    void Start();

  private:
    void OnOpen(websocketpp::connection_hdl hdl);
    void OnClose(websocketpp::connection_hdl hdl);
    void OnMessage(websocketpp::connection_hdl hdl, server_t::message_ptr msg);
    void KeepAlive(server_t::connection_ptr conn);

    void GetPhoneVerifyCode(const httplib::Request &request, httplib::Response &response);
    void PhoneRegister(const httplib::Request &request, httplib::Response &response);
    void PhoneLogin(const httplib::Request &request, httplib::Response &response);

    void UserRegister(const httplib::Request &request, httplib::Response &response);
    void UserLogin(const httplib::Request &request, httplib::Response &response);
    void GetUserInfo(const httplib::Request &request, httplib::Response &response);
    void SetUserAvatar(const httplib::Request &request, httplib::Response &response);
    void SetUserNickname(const httplib::Request &request, httplib::Response &response);
    void SetUserDescription(const httplib::Request &request, httplib::Response &response);
    void SetUserPhoneNumber(const httplib::Request &request, httplib::Response &response);
    std::shared_ptr<im::GetUserInfoResponse> GetUserInfoWithRespone(const std::string &rid, const std::string &uid);

    void FriendAdd(const httplib::Request &request, httplib::Response &response);
    void FriendAddProcess(const httplib::Request &request, httplib::Response &response);
    void FriendRemove(const httplib::Request &request, httplib::Response &response);
    void FriendSearch(const httplib::Request &request, httplib::Response &response);

    void GetFriendList(const httplib::Request &request, httplib::Response &response);
    void GetPendingFriendEventList(const httplib::Request &request, httplib::Response &response);
    void GetChatSessionList(const httplib::Request &request, httplib::Response &response);
    void GetChatSessionMember(const httplib::Request &request, httplib::Response &response);

    void ChatSessionCreate(const httplib::Request &request, httplib::Response &response);
    void GetHistoryMessage(const httplib::Request &request, httplib::Response &response);
    void GetRecentMessage(const httplib::Request &request, httplib::Response &response);
    void SearchMessage(const httplib::Request &request, httplib::Response &response);
    void NewMessage(const httplib::Request &request, httplib::Response &response);

    void GetSingleFile(const httplib::Request &request, httplib::Response &response);
    void GetMultiFile(const httplib::Request &request, httplib::Response &response);
    void PutSingleFile(const httplib::Request &request, httplib::Response &response);
    void PutMultiFile(const httplib::Request &request, httplib::Response &response);

    void SpeechRecognition(const httplib::Request &request, httplib::Response &response);

  private:
    std::shared_ptr<Session> _redis_session;
    std::shared_ptr<Status> _redis_status;

    std::string _user_service_name;
    std::string _file_service_name;
    std::string _speech_service_name;
    std::string _message_service_name;
    std::string _transmite_service_name;
    std::string _friend_service_name;

    std::shared_ptr<ServiceManager> _channels;
    std::shared_ptr<Discovery> _service_discoverer;
    std::shared_ptr<Connection> _connections;

    server_t _ws_server;
    std::shared_ptr<httplib::Server> _http_server;
    std::thread _http_thread;
};

class GatewayServerBuilder
{
  public:
    void MakeRedisObject(const std::string &host, int port, int db, bool keepAlive);

    void MakeDiscoveryObject(const std::string &reg_host, const std::string &base_service_name,
                             const std::string &file_service_name, const std::string &speech_service_name,
                             const std::string &message_service_name, const std::string &friend_service_name,
                             const std::string &user_service_name, const std::string &transmite_service_name);

    void MakeServerObject(int websocket_port, int http_port);

    std::shared_ptr<GatewayServer> Build();

  private:
    int _websocket_port;
    int _http_port;

    std::shared_ptr<sw::redis::Redis> _redis_client;

    std::string _file_service_name;
    std::string _speech_service_name;
    std::string _message_service_name;
    std::string _friend_service_name;
    std::string _user_service_name;
    std::string _transmite_service_name;

    std::shared_ptr<ServiceManager> _channels;
    std::shared_ptr<Discovery> _service_discoverer;
};

#undef HTTP_REQUEST
#undef HTTP_RESPONSE

}; // namespace gateway

#endif // SERVER_H
