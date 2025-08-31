#include "etcd.h"
#include "channel.h"
#include "friend_apply.hxx"
#include "log.hpp"
#include "utils.h"

#include "database/mysql.hpp"
#include "database/mysql_chat_session.h"
#include "database/mysql_apply.h"
#include "database/mysql_relation.h"
#include "database/mysql_chat_session_member.h"

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

std::shared_ptr<odb::database> db_;

class TestClean : public testing::Environment
{
public:
    void SetUp() override
    {
        LOG_INFO("database created");
        db_ = ODBFactory::Create("test", "123456", "127.0.0.1", "im", "utf8", 0, 1);
        odb::transaction tr(db_->begin());
        tr.database().execute("TRUNCATE TABLE relation");
        tr.database().execute("TRUNCATE TABLE friend_apply");
        tr.database().execute("TRUNCATE TABLE chat_session");
        tr.database().execute("TRUNCATE TABLE chat_session_member");
    }
    void TearDown() override
    {
        LOG_INFO("database clean");
        odb::transaction tr(db_->begin());
        tr.database().execute("TRUNCATE TABLE relation");
        tr.database().execute("TRUNCATE TABLE friend_apply");
        tr.database().execute("TRUNCATE TABLE chat_session");
        tr.database().execute("TRUNCATE TABLE chat_session_member");
    }
};


class RelationTableTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        rtb_ = std::make_unique<RelationTable>(db_);
    }

    void TearDown() override
    {
        // db_->execute("TRUNCATE TABLE relation");
    }

    std::unique_ptr<RelationTable> rtb_;
};

TEST_F(RelationTableTest, InsertAndFriend)
{
    rtb_->Insert("用户ID1", "用户ID2");
    rtb_->Insert("用户ID1", "用户ID3");
    auto friends = rtb_->Friends("用户ID1");
    for (auto &info : friends)
    {
        LOG_INFO("friend: {}", info);
    }

    EXPECT_EQ(friends.size(), 2);
    EXPECT_TRUE(std::find(friends.begin(), friends.end(), "用户ID2") != friends.end());
    EXPECT_TRUE(std::find(friends.begin(), friends.end(), "用户ID3") != friends.end());
}

TEST_F(RelationTableTest, Remove)
{
    rtb_->Insert("A", "B");
    EXPECT_TRUE(rtb_->Exists("A", "B"));

    rtb_->Remove("A", "B");
    EXPECT_FALSE(rtb_->Exists("A", "B"));
}

class FriendApplyTableTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        fatb_ = std::make_unique<FriendApplyTable>(db_);
    }
    void TearDown() override
    {
        // db_->execute("TRUNCATE TABLE friend_apply");
    }


    std::unique_ptr<FriendApplyTable> fatb_;
};

TEST_F(FriendApplyTableTest, InsertAndExists)
{
    fatb_->Insert(FriendApply("uuid1", "用户ID1", "用户ID2"));
    fatb_->Insert(FriendApply("uuid2", "用户ID1", "用户ID3"));
    fatb_->Insert(FriendApply("uuid3", "用户ID2", "用户ID3"));

    auto res = fatb_->ApplyUsers("用户ID2");
    for (auto &uid : res)
    {
        LOG_INFO("UID: {}", uid);
    }

    EXPECT_TRUE(fatb_->Exists("用户ID1", "用户ID2"));
    EXPECT_TRUE(fatb_->Exists("用户ID1", "用户ID3"));
    EXPECT_TRUE(fatb_->Exists("用户ID2", "用户ID3"));
}

TEST_F(FriendApplyTableTest, Remove)
{
    fatb_->Insert(FriendApply("uuid4", "A", "B"));
    EXPECT_TRUE(fatb_->Exists("A", "B"));

    fatb_->Remove("A", "B");
    EXPECT_FALSE(fatb_->Exists("A", "B"));
}


class ChatSessionMemberTableTest: public testing::Test
{
protected:
    void SetUp() override
    {
        csmt_ = std::make_unique<ChatSessionMemberTable>(db_);
    }
    std::unique_ptr<ChatSessionMemberTable> csmt_;
};

TEST_F(ChatSessionMemberTableTest, AppendAndMembers)
{
    ChatSessionMember csm1("会话ID1", "用户ID1");
    csmt_->Append(csm1);

    ChatSessionMember csm2("会话ID1", "用户ID2");
    csmt_->Append(csm2);

    ChatSessionMember csm3("会话ID2", "用户ID1");
    csmt_->Append(csm3);

    ChatSessionMember csm4("会话ID2", "用户ID2");
    csmt_->Append(csm4);

    ChatSessionMember csm5("会话ID2", "用户ID3");
    csmt_->Append(csm5);

    auto members = csmt_->Members("会话ID2");
    for (auto& member : members)
    {
        LOG_INFO("UID: {}", member);
    }
    EXPECT_EQ(members.size(), 3);
}

class ChatSessionTableTest: public ::testing::Test
{
protected:
    void SetUp() override
    {
        cstb_ = std::make_unique<ChatSessionTable>(db_);
    }

    std::unique_ptr<ChatSessionTable> cstb_;
};

TEST_F(ChatSessionTableTest, InsertAndSelect)
{
    ChatSession cs1("会话ID1", "会话名称1", ChatSessionType::SINGLE);
    cstb_->Insert(cs1);
    ChatSession cs2("会话ID2", "会话名称2", ChatSessionType::GROUP);
    cstb_->Insert(cs2);

    auto res = cstb_->Select("会话ID1");
    LOG_INFO("session_id: {}", res->chat_session_id());
    LOG_INFO("session_name: {}", res->chat_session_name());
    LOG_INFO("session_type: {}", (int)res->chat_session_type());

    EXPECT_EQ(res->chat_session_id(), "会话ID1");
    EXPECT_EQ(res->chat_session_name(), "会话名称1");
    EXPECT_EQ(res->chat_session_type(), ChatSessionType::SINGLE);
}

TEST_F(ChatSessionTableTest, RemoveBySessionId)
{
    cstb_->Insert(ChatSession("sid2", "to_be_removed", ChatSessionType::GROUP));
    EXPECT_TRUE(cstb_->Select("sid2"));

    cstb_->Remove("sid2");
    EXPECT_FALSE(cstb_->Select("sid2"));
}

TEST_F(ChatSessionTableTest, GetSingleChatSession)
{
    // 假设 Insert 时内部会将会话与成员写入关系表
    // cstb_->Insert(ChatSession("s1", "", ChatSessionType::SINGLE));
    // 此处需 mock 或保证内部有对应关系
    auto res = cstb_->GetSingleChatSession("用户ID1");
    for (auto& info : res)
    {
        LOG_INFO("chat_session_id: {}", info.chat_session_id);
        LOG_INFO("friend_id: {}", info.chat_session_id);
    }
    EXPECT_FALSE(res.empty());
}

TEST_F(ChatSessionTableTest, GetGroupChatSession)
{
    // cstb_->Insert(ChatSession("用户ID1", "group1", ChatSessionType::GROUP));
    // 同样需保证内部关系
    auto res = cstb_->GetGroupChatSession("用户ID1");
    for (auto& info : res)
    {
        LOG_INFO("chat_session_id: {}", info.chat_session_id);
        LOG_INFO("chat_session_name: {}", info.chat_session_name);
    }
    EXPECT_FALSE(res.empty());
}


int main(int argc, char** argv)
{
    using namespace im;
    testing::InitGoogleTest(&argc, argv);
    google::ParseCommandLineFlags(&argc, &argv, true);
    LogSetting setting;
    setting.level = (im::logger::Level)FLAGS_log_level;
    InitLogger(setting);

    ::testing::AddGlobalTestEnvironment(new TestClean);

    return RUN_ALL_TESTS();
}
