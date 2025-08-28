#include "database/mysql.hpp"
#include "etcd.h"
#include "channel.h"
#include "friend_apply.hxx"
#include "log.hpp"
#include "utils.h"

#include "database/mysql_chat_session.h"
#include "database/mysql_apply.h"
#include "database/mysql_relation.h"

#include "friend.pb.h"

#include <gtest/gtest.h>
#include <gflags/gflags.h>

using namespace im;

DEFINE_bool(run_mode, false, "程序的运行模式, false-调试； true-发布；");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

DEFINE_string(etcd_host, "http://127.0.0.1:2379", "服务注册中心地址");
DEFINE_string(base_service, "/service", "服务监控根目录");
DEFINE_string(friend_service, "/service/friend_service", "服务监控根目录");

class FriendTest :public ::testing::Test 
{
protected:
    std::shared_ptr<odb::core::database> _db;

    void SetUpTestSuite()
    {

    }
}

void r_insert_test(im::RelationTable& tb)
{
    tb.Insert("用户ID1", "用户ID2");
    tb.Insert("用户ID1", "用户ID3");
}

void r_select_test(im::RelationTable& tb)
{
    auto res = tb.Friends("用户ID1");
    for (auto& uid : res)
    {
        LOG_INFO("friend: {}", uid);
    }
}

void r_remove_test(im::RelationTable& tb)
{
   tb.Remove("用户ID2", "用户ID1"); 
}

void r_exists_test(im::RelationTable& tb)
{
    LOG_INFO("用户ID2 与 用户ID1 是否有关系: {}", tb.Exists("用户ID3", "用户ID1"));
    LOG_INFO("用户ID3 与 用户ID1 是否有关系: {}", tb.Exists("用户ID3", "用户ID1"));
}

void a_insert_test(im::FriendApplyTable& tb)
{
    im::FriendApply fa1("uuid1", "用户ID1", "用户ID2");
    tb.Insert(fa1);
    im::FriendApply fa2("uuid2", "用户ID1", "用户ID3");
    tb.Insert(fa2);

    im::FriendApply fa3("uuid3", "用户ID2", "用户ID3");
    tb.Insert(fa3);
}

void a_remove_test(im::FriendApplyTable &tb) {
    tb.Remove("用户ID2", "用户ID3");
}

void a_select_test(im::FriendApplyTable &tb) {
    // im::FriendApply fa3("uuid3", "用户ID2", "用户ID3");
    // tb.Insert(fa3);

    auto res = tb.ApplyUsers("用户ID2");
    for (auto &uid:res) {
        std::cout << uid << std::endl;
    }
}
void a_exists_test(im::FriendApplyTable &tb) {
    std::cout << tb.Exists("731f-50086884-0000", "c4dc-68239a9a-0001") << std::endl;
    std::cout << tb.Exists("31ab-86a1209d-0000", "c4dc-68239a9a-0001") << std::endl;
    std::cout << tb.Exists("053f-04e5e4c5-0001", "c4dc-68239a9a-0001") << std::endl;
}

void c_insert_test(im::ChatSessionTable &tb) {
    im::ChatSession cs1("会话ID1", "会话名称1", im::ChatSessionType::SINGLE);
    tb.Insert(cs1);
    im::ChatSession cs2("会话ID2", "会话名称2", im::ChatSessionType::GROUP);
    tb.Insert(cs2);
}


void c_select_test(im::ChatSessionTable &tb) {
    auto res = tb.Select("会话ID1");
    std::cout << res->chat_session_id() << std::endl;
    std::cout << res->chat_session_name() << std::endl;
    std::cout << (int)res->chat_session_type() << std::endl;
}

void c_single_test(im::ChatSessionTable &tb) {
    auto res = tb.GetSingleChatSession("731f-50086884-0000");
    for (auto &info : res) {
        std::cout << info.chat_session_id << std::endl;
        std::cout << info.friend_id << std::endl;
    }
}
void c_group_test(im::ChatSessionTable &tb) {
    auto res = tb.GetGroupChatSession("用户ID1");
    for (auto &info : res) {
        std::cout << info.chat_session_id << std::endl;
        std::cout << info.chat_session_name << std::endl;
    }
}
void c_remove_test(im::ChatSessionTable &tb) {
    tb.Remove("会话ID3");
}
void c_remove_test2(im::ChatSessionTable &tb) {
    tb.Remove("731f-50086884-0000", "c4dc-68239a9a-0001");
}
 
 

int main(int argc, char** argv)
{
    using namespace im;
    google::ParseCommandLineFlags(&argc, &argv, true);
    LogSetting setting;
    setting.level = (im::logger::Level)FLAGS_log_level;
    InitLogger(setting);

    auto db = ODBFactory::Create("test", "123456", "127.0.0.1", "im", "utf8", 0, 1);
    RelationTable rtb(db);
    FriendApplyTable fatb(db);
    ChatSessionTable cstb(db);


    return 0;
}