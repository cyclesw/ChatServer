//
// Created by cyclesw on 2025/8/31.
//
#include "etcd.h"
#include "channel.h"
#include "utils.h"
#include "friend.pb.h"

#include <brpc/channel.h>
#include <gflags/gflags.h>
#include <gtest/gtest.h>

#include "../../../cmake-build-debug/generated/user/user.pb.h"

DEFINE_bool(run_mode, false, "程序的运行模式，false-调试； true-发布；");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

DEFINE_string(etcd_host, "http://127.0.0.1:2379", "服务注册中心地址");
DEFINE_string(base_service, "/service", "服务监控根目录");
DEFINE_string(friend_service, "/service/friend_service", "服务监控根目录");


im::ServiceManager::Ptr sm;
using namespace im;

// kibana
// POST user/_bulk
// { "index" : { "_id" : "用户ID1" } }
// { "user_id": "用户ID1", "nickname": "黑大帅", "avatar_id": "", "description": "", "phone": "" }
// { "index" : { "_id" : "用户ID2" } }
// { "user_id": "用户ID2", "nickname": "白小美", "avatar_id": "", "description": "", "phone": "" }
// { "index" : { "_id" : "用户ID3" } }
// { "user_id": "用户ID3", "nickname": "灰小强", "avatar_id": "", "description": "", "phone": "" }
// { "index" : { "_id" : "用户ID4" } }
// { "user_id": "用户ID4", "nickname": "李达四", "avatar_id": "", "description": "", "phone": "" }

// Mysql-User  ES 必须有以下数据
const std::string uid1 = "用户ID1";
const std::string nickname1 = "黑大帅";
const std::string uid2 = "用户ID2";
const std::string nickname2 = "白小美";
const std::string uid3 = "用户ID3";
const std::string nickname3 = "灰小强";
const std::string uid4 = "用户ID4";
const std::string nickname4 = "李达四";

const std::string ssid1 = "人类交流群1";


class ServiceInit : public ::testing::Environment
{
public:
    void SetUp() override
    {
        LOG_INFO("初始化服务注册中心对象");
        sm = std::make_shared<im::ServiceManager>();
        sm->Declared(FLAGS_friend_service);
        auto put_cb = std::bind(&ServiceManager::OnServiceOnline, sm.get(), std::placeholders::_1, std::placeholders::_2);
        auto del_cb = std::bind(&ServiceManager::OnServiceOffline, sm.get(), std::placeholders::_1, std::placeholders::_2);
        //2. 构造服务发现对象
        Discovery::Ptr dclient = std::make_shared<Discovery>(FLAGS_etcd_host, FLAGS_base_service, put_cb, del_cb);

    }
    void TearDown() override
    {
        sm.reset();
    }
};

class FriendServiceTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        _channel = sm->Choose(FLAGS_friend_service);
        if (!_channel)
        {
            LOG_ERROR("Fail to choose service");
            ASSERT_TRUE(false);
        }

        _stub = std::make_shared<im::FriendService_Stub>(_channel.get());
    }
    void TearDown() override
    {
    }

    im::ServiceChannel::ChannelPtr _channel;
    std::shared_ptr<im::FriendService_Stub> _stub;
};


TEST_F(FriendServiceTest, Apply)
{
    im::FriendAddRequest req;
    im::FriendAddResponse rsp;
    brpc::Controller cntl;

    req.set_request_id(im::Uuid());
    req.set_user_id(uid2);
    req.set_respondent_id(uid1);
    _stub->FriendAdd(&cntl, &req, &rsp, nullptr);

    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());

    cntl.Reset();
    req.set_request_id(im::Uuid());
    req.set_user_id(uid3);
    req.set_respondent_id(uid1);
    _stub->FriendAdd(&cntl, &req, &rsp, nullptr);

    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());

    cntl.Reset();
    req.set_request_id(im::Uuid());
    req.set_user_id(uid4);
    req.set_respondent_id(uid1);
    _stub->FriendAdd(&cntl, &req, &rsp, nullptr);

    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
}

TEST_F(FriendServiceTest, ApplyList)
{
    im::GetPendingFriendEventListRequest request;
    im::GetPendingFriendEventListResponse response;


    request.set_request_id(im::Uuid());
    request.set_user_id(uid1);
    brpc::Controller cntl;

    _stub->GetPendingFriendEventList(&cntl, &request, &response, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(response.success());

    for (int i = 0; i < response.event_size(); i++) {
        std::cout << "---------------\n";
        std::cout << response.event(i).sender().user_id() << std::endl;
        std::cout << response.event(i).sender().nickname() << std::endl;
        std::cout << response.event(i).sender().avatar() << std::endl;
    }
}

TEST_F(FriendServiceTest, ProcessApply)
{
    FriendAddProcessRequest req;
    FriendAddProcessResponse rsp;

    req.set_request_id(im::Uuid());
    req.set_user_id(uid1);
    req.set_agree(true);
    req.set_apply_user_id(uid2);
    brpc::Controller cntl;
    _stub->FriendAddProcess(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
    LOG_INFO("new session id: {}", rsp.new_session_id());


    cntl.Reset();
    req.set_request_id(im::Uuid());
    req.set_user_id(uid1);
    req.set_agree(false);
    req.set_apply_user_id(uid3);
    _stub->FriendAddProcess(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
    LOG_INFO("new session id: {}", rsp.new_session_id());

    cntl.Reset();
    req.set_request_id(im::Uuid());
    req.set_user_id(uid1);
    req.set_agree(true);
    req.set_apply_user_id(uid4);
    _stub->FriendAddProcess(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
}

TEST_F(FriendServiceTest, Search)
{
    im::FriendSearchRequest req;
    im::FriendSearchResponse rsp;
    req.set_request_id(im::Uuid());
    req.set_user_id(uid1);
    req.set_search_key("小");
    brpc::Controller cntl;
    _stub->FriendSearch(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
    for (int i = 0; i < rsp.user_info_size(); i++) {
        std::cout << "-------------------\n";
        std::cout << rsp.user_info(i).user_id() << std::endl;
        std::cout << rsp.user_info(i).nickname() << std::endl;
        std::cout << rsp.user_info(i).avatar() << std::endl;
    }
}

TEST_F(FriendServiceTest, Remove)
{
    FriendRemoveRequest req;
    FriendRemoveResponse rsp;
    req.set_request_id(Uuid());
    req.set_user_id(uid4);
    req.set_peer_id(uid1);
    brpc::Controller cntl;
    _stub->FriendRemove(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
}

TEST_F(FriendServiceTest, CreateSession)
{
    im::ChatSessionCreateRequest req;
    im::ChatSessionCreateResponse rsp;
    req.set_request_id(im::Uuid());
    req.set_user_id(uid1);
    req.set_chat_session_name("快乐一家人");
    req.add_member_id_list(uid1);
    req.add_member_id_list(uid2);
    req.add_member_id_list(uid3);
    req.add_member_id_list(uid4);
    brpc::Controller cntl;
    _stub->ChatSessionCreate(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
    LOG_INFO("chat_session_id: {}", rsp.chat_session_info().chat_session_id());
    LOG_INFO("chat_session_name: {}", rsp.chat_session_info().chat_session_name());
}

TEST_F(FriendServiceTest, GetSessionMember)
{
    GetChatSessionMemberRequest req;
    GetChatSessionMemberResponse rsp;
    req.set_user_id(uid1);
    req.set_chat_session_id(ssid1);
    brpc::Controller cntl;
    _stub->GetChatSessionMember(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
    for (int i = 0; i < rsp.member_info_list_size(); i++) {
        std::cout << "-------------------\n";
        std::cout << rsp.member_info_list(i).user_id() << std::endl;
        std::cout << rsp.member_info_list(i).nickname() << std::endl;
        std::cout << rsp.member_info_list(i).avatar() << std::endl;
    }
}

TEST_F(FriendServiceTest, GetSessionList)
{
    GetChatSessionListRequest req;
    GetChatSessionListResponse rsp;
    req.set_request_id(im::Uuid());
    req.set_user_id(uid1);
    brpc::Controller cntl;
    LOG_INFO("发送获取聊天会话列表请求！！");
    _stub->GetChatSessionList(&cntl, &req, &rsp, nullptr);
    LOG_INFO("请求发送完毕1！！");
    ASSERT_FALSE(cntl.Failed());
    LOG_INFO("请求发送完毕2！！");
    ASSERT_TRUE(rsp.success());
    LOG_INFO("请求发送完毕，且成功！！");
    for (int i = 0; i < rsp.chat_session_info_list_size(); i++) {
        LOG_INFO("-------------------");
        LOG_INFO("single_chat_friend_id: {}", rsp.chat_session_info_list(i).single_chat_friend_id());
        LOG_INFO("chat_session_id: {}", rsp.chat_session_info_list(i).chat_session_id());
        LOG_INFO("chat_session_name: {}", rsp.chat_session_info_list(i).chat_session_name());
        LOG_INFO("avatar: {}", rsp.chat_session_info_list(i).avatar());
        LOG_INFO("消息内容：");
        LOG_INFO("  message_id: {}", rsp.chat_session_info_list(i).prev_message().message_id());
        LOG_INFO("  chat_session_id: {}", rsp.chat_session_info_list(i).prev_message().chat_session_id());
        LOG_INFO("  timestamp: {}", rsp.chat_session_info_list(i).prev_message().timestamp());
        LOG_INFO("  sender_user_id: {}", rsp.chat_session_info_list(i).prev_message().sender().user_id());
        LOG_INFO("  sender_nickname: {}", rsp.chat_session_info_list(i).prev_message().sender().nickname());
        LOG_INFO("  sender_avatar: {}", rsp.chat_session_info_list(i).prev_message().sender().avatar());
        LOG_INFO("  file_name: {}", rsp.chat_session_info_list(i).prev_message().message().file_message().file_name());
        LOG_INFO("  file_contents: {}", rsp.chat_session_info_list(i).prev_message().message().file_message().file_contents());
    }
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    google::ParseCommandLineFlags(&argc, &argv, true);

    ::testing::AddGlobalTestEnvironment(new ServiceInit);

    return RUN_ALL_TESTS();
}