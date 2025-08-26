#include "etcd.h"
#include "channel.h"
#include "log.hpp"
#include "utils.h"
#include "transmit.pb.h"

#include <brpc/server.h>
#include <brpc/channel.h>
#include <gflags/gflags.h>
#include <gtest/gtest.h>

DEFINE_bool(run_mode, false, "程序的运行模式，false-调试； true-发布；");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

DEFINE_string(etcd_host, "http://127.0.0.1:2379", "服务注册中心地址");
DEFINE_string(base_service, "/service", "服务监控根目录");
DEFINE_string(transmite_service, "/service/transmite_service", "服务监控根目录");

namespace {

class SpeechClientTest : public ::testing::Test {
protected:
    static im::ServiceManager::Ptr sm_;
    static im::Discovery::Ptr      dclient_;

    static void SetUpTestSuite() {
        // 初始化一次即可
        sm_ = std::make_shared<im::ServiceManager>();
        sm_->Declared(FLAGS_transmite_service);

        auto put_cb = std::bind(&im::ServiceManager::OnServiceOnline,
                                sm_.get(), std::placeholders::_1, std::placeholders::_2);
        auto del_cb = std::bind(&im::ServiceManager::OnServiceOffline,
                                sm_.get(), std::placeholders::_1, std::placeholders::_2);

        dclient_ = std::make_shared<im::Discovery>(FLAGS_etcd_host,
                                                   FLAGS_base_service,
                                                   put_cb, del_cb);
    }

    void send_message(const im::MessageType type,
                      const std::string&  uid,
                      const std::string&  sid,
                      const std::string&  content,
                      const std::string&  filename = "") {
        auto channel = sm_->Choose(FLAGS_transmite_service);
        ASSERT_TRUE(channel) << "获取通信信道失败！";

        im::MsgTransmitService_Stub stub(channel.get());
        im::NewMessageRequest       req;
        im::GetTransmitTargetResponse rsp;
        brpc::Controller            cntl;

        req.set_request_id(im::Uuid());
        req.set_user_id(uid);
        req.set_chat_session_id(sid);
        req.mutable_message()->set_message_type(type);

        switch (type) {
        case im::MessageType::STRING:
            req.mutable_message()->mutable_string_message()->set_content(content);
            break;
        case im::MessageType::IMAGE:
            req.mutable_message()->mutable_image_message()->set_image_content(content);
            break;
        case im::MessageType::SPEECH:
            req.mutable_message()->mutable_speech_message()->set_file_contents(content);
            break;
        case im::MessageType::FILE:
            req.mutable_message()->mutable_file_message()->set_file_contents(content);
            req.mutable_message()->mutable_file_message()->set_file_name(filename);
            req.mutable_message()->mutable_file_message()->set_file_size(content.size());
            break;
        default:
            FAIL() << "未支持的消息类型！";
        }

        stub.GetTransmitTarget(&cntl, &req, &rsp, nullptr);
        if (cntl.Failed()) {
            LOG_ERROR("RPC failed: {}", cntl.ErrorText());
        }
        if (!rsp.success()) {
            LOG_ERROR("Service error: {}", rsp.error());
        }
    }
};

/* static */ im::ServiceManager::Ptr SpeechClientTest::sm_;
/* static */ im::Discovery::Ptr      SpeechClientTest::dclient_;

//-------------- 用例 --------------
TEST_F(SpeechClientTest, SendStringMessage) {
    send_message(im::MessageType::STRING,
                 "731f-50086884-0000",
                 "会话ID1",
                 "吃饭了吗？");
}

TEST_F(SpeechClientTest, SendImageMessage) {
    send_message(im::MessageType::IMAGE,
                 "c4dc-68239a9a-0001",
                 "会话ID1",
                 "可爱表情图片数据");
}

TEST_F(SpeechClientTest, SendSpeechMessage) {
    send_message(im::MessageType::SPEECH,
                 "731f-50086884-0000",
                 "会话ID1",
                 "动听猪叫声数据");
}

TEST_F(SpeechClientTest, SendFileMessage) {
    send_message(im::MessageType::FILE,
                 "731f-50086884-0000",
                 "0d90-755571d8-0003",
                 "猪爸爸的文件数据",
                 "猪爸爸的文件名称");
}

}  // namespace

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    google::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}