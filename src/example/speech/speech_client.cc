#include "channel.h"
#include "etcd.h"
#include "log.hpp"
#include "speech.pb.h"

#include <brpc/controller.h>
#include <chrono>
#include <filesystem>
#include <gflags/gflags.h>
#include <aip/speech.h>
#include <google/protobuf/service.h>
#include <memory>
#include <string>
#include <thread>
#include <brpc/channel.h>


DEFINE_bool(run_mode, false, "程序的运行模式，false-调试； true-发布；");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

DEFINE_string(etcd_host, "http://127.0.0.1:2379", "服务注册中心地址");
DEFINE_string(base_service, "/service", "服务监控根目录");
DEFINE_string(speech_service, "/service/speech_service", "服务监控根目录");


int main(int argc, char** argv)
{
    using namespace im;
    google::ParseCommandLineFlags(&argc, &argv, true);
    
    auto sm = std::make_shared<ServiceManager>();
    sm->Declared(FLAGS_speech_service);
    auto put_cb = std::bind(&im::ServiceManager::OnServiceOnline, sm.get(), std::placeholders::_1, std::placeholders::_2);
    auto del_cb = std::bind(&im::ServiceManager::OnServiceOffline, sm.get(), std::placeholders::_1, std::placeholders::_2);

    Discovery::Ptr dclient = std::make_shared<im::Discovery>(FLAGS_etcd_host, FLAGS_base_service, put_cb, del_cb);

    auto channel = sm->Choose(FLAGS_speech_service);
    if (!channel)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return -1;
    }

    std::string file_content;
    aip::get_file_content("16k.pcm", &file_content);
    if (file_content.empty())
    {
        LOG_ERROR("Failed to read audio file, pwd: {}", std::filesystem::current_path().string());
        return -1;
    }

    SpeechService_Stub stub(channel.get());
    im::SpeechRecognitionRequest request;
    request.set_speech_content(file_content);
    request.set_request_id("1111111");

    brpc::Controller* cntl = new brpc::Controller();
    SpeechRecognitionResponse * resp = new SpeechRecognitionResponse();
    stub.SpeechRecognition(cntl, &request, resp, nullptr);

    if (cntl->Failed())
    {
        LOG_ERROR("RPC failed: {}", cntl->ErrorText());
        delete cntl;
        delete resp;

        return -1;
    }

    if (!resp->success())
    {
        LOG_ERROR("Speech recognition failed: {}", resp->errmsg());

        return -1;
    }

    LOG_INFO("收到响应: {}", resp->request_id());
    LOG_INFO("收到响应: {}", resp->recognition_result());

    return 0;
}
