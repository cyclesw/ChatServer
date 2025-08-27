#include "etcd.h"
#include "channel.h"
#include "utils.h"
#include "database/mysql_message.h"
#include "database/mysql.hpp"

#include "message.pb.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <gflags/gflags.h>
#include <brpc/channel.h>
#include <gtest/gtest.h>

DEFINE_bool(run_mode, false, "程序的运行模式，false-调试； true-发布；");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

DEFINE_string(etcd_host, "http://127.0.0.1:2379", "服务注册中心地址");
DEFINE_string(base_service, "/service", "服务监控根目录");
DEFINE_string(message_service, "/service/message_service", "服务监控根目录");

im::ServiceManager::Ptr sm;

void insert_test(im::MessageTable &tb) {
    im::Message m1("消息ID1", "会话ID1", "用户ID1", 0, boost::posix_time::time_from_string("2002-01-20 23:59:59.000"));
    tb.Insert(m1);
    im::Message m2("消息ID2", "会话ID1", "用户ID2", 0, boost::posix_time::time_from_string("2002-01-21 23:59:59.000"));
    tb.Insert(m2);
    im::Message m3("消息ID3", "会话ID1", "用户ID3", 0, boost::posix_time::time_from_string("2002-01-22 23:59:59.000"));
    tb.Insert(m3);

    im::Message m4("消息ID4", "会话ID2", "用户ID4", 0, boost::posix_time::time_from_string("2002-01-20 23:59:59.000"));
    tb.Insert(m4);
    im::Message m5("消息ID5", "会话ID2", "用户ID5", 0, boost::posix_time::time_from_string("2002-01-21 23:59:59.000"));
    tb.Insert(m5);
    im::Message m6("好想吃盖浇牛肉饭啊", "会话ID1", "用户ID5", 0, boost::posix_time::time_from_string("2002-01-20 23:59:59.000"));
    tb.Insert(m6);
}
void remove_test(im::MessageTable &tb) {
    tb.Remove("会话ID2");
}

void recent_test(im::MessageTable &tb) {
    auto res = tb.Recent("会话ID1", 2);
    auto begin = res.rbegin();
    auto end = res.rend();
    for (; begin != end; ++begin) {
        std::cout << begin->message_id() << std::endl;
        std::cout << begin->session_id() << std::endl;
        std::cout << begin->user_id() << std::endl;
        std::cout << boost::posix_time::to_simple_string(begin->create_time()) << std::endl;
    }
}
void range_test(im::MessageTable &tb) {
    boost::posix_time::ptime stime(boost::posix_time::time_from_string("2002-01-20 23:59:59.000"));
    boost::posix_time::ptime etime(boost::posix_time::time_from_string("2002-01-21 23:59:59.000"));
    auto res = tb.Range("会话ID1", stime, etime);
    for (const auto &m : res) {
        std::cout << m.message_id() << std::endl;
        std::cout << m.session_id() << std::endl;
        std::cout << m.user_id() << std::endl;
        std::cout << boost::posix_time::to_simple_string(m.create_time()) << std::endl;
    }
}
void remove_all(im::MessageTable &tb) {
    tb.Remove("会话ID1");
    tb.Remove("会话ID2");
    tb.Remove("会话ID3");
}





void range_test(const std::string &ssid, 
    const boost::posix_time::ptime &stime,
    const boost::posix_time::ptime &etime) {
    auto channel = sm->Choose(FLAGS_message_service);
    if (!channel) {
        std::cout << "获取通信信道失败！" << std::endl;
        return;
    }
    im::MessageStorageService_Stub stub(channel.get());
    im::GetHistoryMessageRequest req;
    im::GetHistoryMessageResponse rsp;
    req.set_request_id(im::Uuid());
    req.set_chat_session_id(ssid);
    req.set_start_time(boost::posix_time::to_time_t(stime));
    req.set_over_time(boost::posix_time::to_time_t(etime));
    brpc::Controller cntl;
    stub.GetHistoryMessage(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
    for (int i = 0; i < rsp.msg_list_size(); i++) {
        std::cout << "-----------------------获取区间消息--------------------------\n";
        auto msg = rsp.msg_list(i);
        std::cout << msg.message_id() << std::endl;
        std::cout << msg.chat_session_id() << std::endl;
        std::cout << boost::posix_time::to_simple_string(boost::posix_time::from_time_t(msg.timestamp())) << std::endl;
        std::cout << msg.sender().user_id() << std::endl;
        std::cout << msg.sender().nickname() << std::endl;
        std::cout << msg.sender().avatar() << std::endl;
        if (msg.message().message_type() == im::MessageType::STRING) {
            std::cout << "文本消息：" << msg.message().string_message().content() << std::endl;
        }else if (msg.message().message_type() == im::MessageType::IMAGE) {
            std::cout << "图片消息：" << msg.message().image_message().image_content() << std::endl;
        }else if (msg.message().message_type() == im::MessageType::FILE) {
            std::cout << "文件消息：" << msg.message().file_message().file_contents() << std::endl;
            std::cout << "文件名称：" << msg.message().file_message().file_name() << std::endl;
        }else if (msg.message().message_type() == im::MessageType::SPEECH) {
            std::cout << "语音消息：" << msg.message().speech_message().file_contents() << std::endl;
        }else {
            std::cout << "类型错误！！\n";
        }
    }
}


void recent_test(const std::string &ssid, int count) {
    auto channel = sm->Choose(FLAGS_message_service);
    if (!channel) {
        std::cout << "获取通信信道失败！" << std::endl;
        return;
    }
    im::MessageStorageService_Stub stub(channel.get());
    im::GetRecentMessageRequest req;
    im::GetRecentMessageResponse rsp;
    req.set_request_id(im::Uuid());
    req.set_chat_session_id(ssid);
    req.set_msg_count(count);
    brpc::Controller cntl;
    stub.GetRecentMessage(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
    for (int i = 0; i < rsp.msg_list_size(); i++) {
        std::cout << "----------------------获取最近消息---------------------------\n";
        auto msg = rsp.msg_list(i);
        std::cout << msg.message_id() << std::endl;
        std::cout << msg.chat_session_id() << std::endl;
        std::cout << boost::posix_time::to_simple_string(boost::posix_time::from_time_t(msg.timestamp())) << std::endl;
        std::cout << msg.sender().user_id() << std::endl;
        std::cout << msg.sender().nickname() << std::endl;
        std::cout << msg.sender().avatar() << std::endl;
        if (msg.message().message_type() == im::MessageType::STRING) {
            std::cout << "文本消息：" << msg.message().string_message().content() << std::endl;
        }else if (msg.message().message_type() == im::MessageType::IMAGE) {
            std::cout << "图片消息：" << msg.message().image_message().image_content() << std::endl;
        }else if (msg.message().message_type() == im::MessageType::FILE) {
            std::cout << "文件消息：" << msg.message().file_message().file_contents() << std::endl;
            std::cout << "文件名称：" << msg.message().file_message().file_name() << std::endl;
        }else if (msg.message().message_type() == im::MessageType::SPEECH) {
            std::cout << "语音消息：" << msg.message().speech_message().file_contents() << std::endl;
        }else {
            std::cout << "类型错误！！\n";
        }
    }
}


void search_test(const std::string &ssid, const std::string &key) {
    auto channel = sm->Choose(FLAGS_message_service);
    if (!channel) {
        std::cout << "获取通信信道失败！" << std::endl;
        return;
    }
    im::MessageStorageService_Stub stub(channel.get());
    im::MessageSearchRequest req;
    im::MessageSearchResponse rsp;
    req.set_request_id(im::Uuid());
    req.set_chat_session_id(ssid);
    req.set_search_key(key);
    brpc::Controller cntl;
    stub.MessageSearch(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
    for (int i = 0; i < rsp.msg_list_size(); i++) {
        std::cout << "----------------------关键字搜索消息---------------------------\n";
        auto msg = rsp.msg_list(i);
        std::cout << msg.message_id() << std::endl;
        std::cout << msg.chat_session_id() << std::endl;
        std::cout << boost::posix_time::to_simple_string(boost::posix_time::from_time_t(msg.timestamp())) << std::endl;
        std::cout << msg.sender().user_id() << std::endl;
        std::cout << msg.sender().nickname() << std::endl;
        std::cout << msg.sender().avatar() << std::endl;
        if (msg.message().message_type() == im::MessageType::STRING) {
            std::cout << "文本消息：" << msg.message().string_message().content() << std::endl;
        }else if (msg.message().message_type() == im::MessageType::IMAGE) {
            std::cout << "图片消息：" << msg.message().image_message().image_content() << std::endl;
        }else if (msg.message().message_type() == im::MessageType::FILE) {
            std::cout << "文件消息：" << msg.message().file_message().file_contents() << std::endl;
            std::cout << "文件名称：" << msg.message().file_message().file_name() << std::endl;
        }else if (msg.message().message_type() == im::MessageType::SPEECH) {
            std::cout << "语音消息：" << msg.message().speech_message().file_contents() << std::endl;
        }else {
            std::cout << "类型错误！！\n";
        }
    }
}

int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);

    auto db = im::ODBFactory::Create("test", "123456", "127.0.0.1", "im", "utf8", 0, 1);
    im::MessageTable tb(db);
    insert_test(tb);
    remove_test(tb);
    recent_test(tb);
    range_test(tb);

    
    //1. 先构造Rpc信道管理对象
    sm = std::make_shared<im::ServiceManager>();
    sm->Declared(FLAGS_message_service);
    auto put_cb = std::bind(&im::ServiceManager::OnServiceOnline, sm.get(), std::placeholders::_1, std::placeholders::_2);
    auto del_cb = std::bind(&im::ServiceManager::OnServiceOffline, sm.get(), std::placeholders::_1, std::placeholders::_2);
    //2. 构造服务发现对象
    im::Discovery::Ptr dclient = std::make_shared<im::Discovery>(FLAGS_etcd_host, FLAGS_base_service, put_cb, del_cb);
    
    boost::posix_time::ptime stime(boost::posix_time::time_from_string("2002-01-20 23:59:59.000"));
    boost::posix_time::ptime etime(boost::posix_time::time_from_string("2002-01-21 23:59:59.000"));
    range_test("会话ID1", stime, etime);
    recent_test("会话ID1", 2);
    search_test("会话ID1", "盖浇");

    remove_all(tb);
    return 0;
}
