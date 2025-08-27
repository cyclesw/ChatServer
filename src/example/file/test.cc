#include "base.pb.h"
#include "etcd.h"
#include "file.pb.h"
#include "channel.h"
#include "log.hpp"
#include "utils.h"

#include <brpc/channel.h>
#include <brpc/controller.h>
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <thread>



DEFINE_bool(run_mode, false, "程序的运行模式, false-调试； true-发布；");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

DEFINE_string(etcd_host, "http://127.0.0.1:2379", "服务注册中心地址");
DEFINE_string(base_service, "/service", "服务监控根目录");
DEFINE_string(file_service, "/service/file_service", "服务监控根目录");

using namespace im;
im::ServiceChannel::ChannelPtr channel;
std::string single_file_id;

TEST(put_test, single_file_id)
{
    std::string body;
    ASSERT_TRUE(im::ReadFile("readme.txt", body));
    FileService_Stub stub(channel.get());

    PutSingleFileRequest request;
    request.set_request_id("1111");
    auto data = request.mutable_file_data();
    data->set_file_name("readme.txt");
    data->set_file_size(body.size());
    data->set_file_content(body);

    brpc::Controller* cntl = new brpc::Controller();
    im::PutSingleFileResponse *response = new im::PutSingleFileResponse();
    stub.PutSingleFile(cntl, &request, response, nullptr);
    ASSERT_TRUE(response->success());
    ASSERT_EQ(response->file_info().file_name(), "readme.txt");
    ASSERT_EQ(response->file_info().file_size(), body.size());

    single_file_id = response->file_info().file_id();
    LOG_INFO("文件ID: {}", response->file_info().file_id());
}


int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    google::ParseCommandLineFlags(&argc, &argv, true);

    auto sm = std::make_shared<ServiceManager>();
    sm->Declared(FLAGS_file_service);
    auto put_cb = std::bind(&ServiceManager::OnServiceOnline, sm.get(), std::placeholders::_1, std::placeholders::_2);
    auto del_cb = std::bind(&ServiceManager::OnServiceOffline, sm.get(), std::placeholders::_1, std::placeholders::_2);

    Discovery::Ptr dclient = std::make_shared<Discovery>(FLAGS_etcd_host, FLAGS_base_service, put_cb, del_cb);

    channel = sm->Choose(FLAGS_file_service);
    if (!channel)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return -1;
    }

    return RUN_ALL_TESTS();
}