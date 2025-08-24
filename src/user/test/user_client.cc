#include "etcd.h"
#include "channel.h"
#include "utils.h"

#include "user.pb.h"
#include "base.pb.h"

#include <gtest/gtest.h>
#include <gflags/gflags.h>
#include <brpc/channel.h>


DEFINE_bool(run_mode, false, "程序的运行模式，false-调试； true-发布；");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

DEFINE_string(etcd_host, "http://127.0.0.1:2379", "服务注册中心地址");
DEFINE_string(base_service, "/service", "服务监控根目录");
DEFINE_string(user_service, "/service/user_service", "服务监控根目录");

im::ServiceManager::Ptr user_channels;

im::UserInfo user_info;

std::string login_ssid;
std::string new_nickname = "乌云乌云快走开";

TEST(用户子服务测试, 用户登录测试) {
    auto channel = user_channels->Choose(FLAGS_user_service);//获取通信信道
    ASSERT_TRUE(channel);

    im::UserLoginRequest req;
    req.set_request_id(im::Uuid());
    req.set_nickname("cyclesw");
    req.set_password("123456");
    im::UserLoginResponse rsp;
    brpc::Controller cntl;
    im::UserService_Stub stub(channel.get());
    stub.UserLogin(&cntl, &req, &rsp, nullptr);
    LOG_INFO("LoginResponse: {}", rsp.error());
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
    login_ssid = rsp.login_session_id();
}
TEST(用户子服务测试, 用户头像设置测试)
{
    auto channel = user_channels->Choose(FLAGS_user_service);//获取通信信道
    ASSERT_TRUE(channel);

    im::SetUserAvatarRequest request;
    request.set_request_id(im::Uuid());
    request.set_user_id(user_info.user_id());
    request.set_session_id(login_ssid);
    request.set_avatar(user_info.avatar());
    im::SetUserAvatarResponse response;
    brpc::Controller cntl;
    im::UserService_Stub stub(channel.get());
    stub.SetUserAvatar(&cntl, &request, &response, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(response.success());
}
TEST(用户子服务测试, 用户签名设置测试) {
    auto channel = user_channels->Choose(FLAGS_user_service);//获取通信信道
    ASSERT_TRUE(channel);

    im::SetUserDescriptionRequest req;
    req.set_request_id(im::Uuid());
    req.set_user_id(user_info.user_id());
    req.set_session_id(login_ssid);
    req.set_description(user_info.description());
    im::SetUserDescriptionResponse rsp;
    brpc::Controller cntl;
    im::UserService_Stub stub(channel.get());
    stub.SetUserDescription(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
}
TEST(用户子服务测试, 用户昵称设置测试) {
    auto channel = user_channels->Choose(FLAGS_user_service);//获取通信信道
    ASSERT_TRUE(channel);

    im::SetUserNicknameRequest req;
    req.set_request_id(im::Uuid());
    req.set_user_id(user_info.user_id());
    req.set_session_id(login_ssid);
    req.set_nickname(new_nickname);
    im::SetUserNicknameResponse rsp;
    brpc::Controller cntl;
    im::UserService_Stub stub(channel.get());
    stub.SetUserNickname(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
}


TEST(用户子服务测试, 用户信息获取测试) {
    auto channel = user_channels->Choose(FLAGS_user_service);//获取通信信道
    ASSERT_TRUE(channel);

    im::GetUserInfoRequest req;
    req.set_request_id(im::Uuid());
    req.set_user_id(user_info.user_id());
    req.set_session_id(login_ssid);
    im::GetUserInfoResponse rsp;
    brpc::Controller cntl;
    im::UserService_Stub stub(channel.get());
    stub.GetUserInfo(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
    ASSERT_EQ(user_info.user_id(), rsp.user_info().user_id());
    ASSERT_EQ(new_nickname, rsp.user_info().nickname());
    ASSERT_EQ(user_info.description(), rsp.user_info().description());
    ASSERT_EQ("18475266821", rsp.user_info().phone());
    ASSERT_EQ(user_info.avatar(), rsp.user_info().avatar());
}

void SetUserAvatar(const std::string& uid, const std::string& avatar)
{
    auto channel = user_channels->Choose(FLAGS_user_service);//获取通信信道
    ASSERT_TRUE(channel);
    im::SetUserAvatarRequest req;
    req.set_request_id(im::Uuid());
    req.set_user_id(uid);
    req.set_session_id(login_ssid);
    req.set_avatar(avatar);
    im::SetUserAvatarResponse rsp;
    brpc::Controller cntl;
    im::UserService_Stub stub(channel.get());
    stub.SetUserAvatar(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
}

TEST(用户子服务测试, 批量用户信息获取测试)
{
    SetUserAvatar("用户ID1", "小猪佩奇的头像数据");
    SetUserAvatar("用户ID2", "小猪乔治的头像数据");
    auto channel = user_channels->Choose(FLAGS_user_service);//获取通信信道
    ASSERT_TRUE(channel);

    im::GetMultiUserInfoRequest req;
    req.set_request_id(im::Uuid());
    req.add_users_id("用户ID1");
    req.add_users_id("用户ID2");
    req.add_users_id("1d56-513d8e49-0002");
    im::GetMultiUserInfoResponse rsp;
    brpc::Controller cntl;
    im::UserService_Stub stub(channel.get());
    stub.GetMultiUserInfo(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
    auto users_map = rsp.mutable_users_info();
    im::UserInfo fuser = users_map->at("1d56-513d8e49-0002");
    ASSERT_EQ(fuser.user_id(), "1d56-513d8e49-0002");
    ASSERT_EQ(fuser.nickname(), "乌云乌云快走开");
    ASSERT_EQ(fuser.description(), "cycle of sorrow");
    ASSERT_EQ(fuser.phone(), "18475266821");
    ASSERT_EQ(fuser.avatar(), "avatar");

    im::UserInfo puser = (*users_map)["用户ID1"];
    ASSERT_EQ(puser.user_id(), "用户ID1");
    ASSERT_EQ(puser.nickname(), "小猪佩奇");
    ASSERT_EQ(puser.description(), "这是一只小猪");
    ASSERT_EQ(puser.phone(), "手机号1");
    ASSERT_EQ(puser.avatar(), "小猪佩奇的头像数据");

    im::UserInfo quser = (*users_map)["用户ID2"];
    ASSERT_EQ(quser.user_id(), "用户ID2");
    ASSERT_EQ(quser.nickname(), "小猪乔治");
    ASSERT_EQ(quser.description(), "这是一只小小猪");
    ASSERT_EQ(quser.phone(), "手机号2");
    ASSERT_EQ(quser.avatar(), "小猪乔治的头像数据");
}

std::string code_id;
void get_code() {
    auto channel = user_channels->Choose(FLAGS_user_service);//获取通信信道
    ASSERT_TRUE(channel);

    im::PhoneVerifyCodeRequest req;
    req.set_request_id(im::Uuid());
    req.set_phone_number(user_info.phone());
    im::PhoneVerifyCodeResponse rsp;
    brpc::Controller cntl;
    im::UserService_Stub stub(channel.get());
    stub.GetPhoneVerifyCode(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
    code_id = rsp.verify_code_id();
}
TEST(用户子服务测试, 手机号登录) {
    std::this_thread::sleep_for(std::chrono::seconds(3));
    get_code();
    auto channel = user_channels->Choose(FLAGS_user_service);//获取通信信道
    ASSERT_TRUE(channel);

    im::PhoneLoginRequest req;
    req.set_request_id(im::Uuid());
    req.set_phone_number(user_info.phone());
    req.set_verify_code_id(code_id);
    std::cout << "手机号登录，输入验证码：" << std::endl;
    std::string code;
    std::cin >> code;
    req.set_verify_code(code);
    im::PhoneLoginResponse rsp;
    brpc::Controller cntl;
    im::UserService_Stub stub(channel.get());
    stub.PhoneLogin(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
    std::cout << "手机登录会话ID：" << rsp.login_session_id() << std::endl;
}
TEST(用户子服务测试, 手机号设置) {
    std::this_thread::sleep_for(std::chrono::seconds(10));
    get_code();
    auto channel = user_channels->Choose(FLAGS_user_service);//获取通信信道
    ASSERT_TRUE(channel);

    im::SetUserPhoneNumberRequest req;
    req.set_request_id(im::Uuid());
    std::cout << "手机号设置时，输入用户ID：" << std::endl;
    std::string user_id;
    std::cin >> user_id;
    req.set_user_id(user_id);
    req.set_phone_number("18888888888");
    req.set_phone_verify_code_id(code_id);
    std::cout << "手机号设置时，输入验证码：" << std::endl;
    std::string code;
    std::cin >> code;
    req.set_phone_verify_code(code);
    im::SetUserPhoneNumberResponse rsp;
    brpc::Controller cntl;
    im::UserService_Stub stub(channel.get());
    stub.SetUserPhoneNumber(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
}


int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    google::ParseCommandLineFlags(&argc, &argv, true);

    //1. 先构造Rpc信道管理对象
    user_channels = std::make_shared<im::ServiceManager>();
    user_channels->Declared(FLAGS_user_service);
    auto put_cb = std::bind(&im::ServiceManager::OnServiceOnline, user_channels.get(), std::placeholders::_1, std::placeholders::_2);
    auto del_cb = std::bind(&im::ServiceManager::OnServiceOffline, user_channels.get(), std::placeholders::_1, std::placeholders::_2);

    //2. 构造服务发现对象
    im::Discovery::Ptr dclient = std::make_shared<im::Discovery>(FLAGS_etcd_host, FLAGS_base_service, put_cb, del_cb);

    user_info.set_nickname("cyclesw");
    user_info.set_user_id("1d56-513d8e49-0002");
    user_info.set_description("cycle of sorrow");
    user_info.set_phone("18475266821");
    user_info.set_avatar("avatar");
    LOG_INFO("开始测试！");

    return RUN_ALL_TESTS();
}