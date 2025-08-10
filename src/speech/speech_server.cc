//
// Created by 19396 on 25-8-3.
//

#include "speech_server.h"

#include <gflags/gflags.h>

DEFINE_string(app_id, "116699311", "语音平台应用ID");
DEFINE_string(api_key, "C1MwX3PbmzFE01KkVdIIRZKd", "语音平台API密钥");
DEFINE_string(secret_key, "70GNIUw93umJDJb4qriFALRmDdkopBtk", "语音平台加密密钥");

DEFINE_string(registry_host, "http://127.0.0.1:2379", "服务注册中心地址");
DEFINE_string(base_service, "/service", "服务监控根目录");
DEFINE_string(instance_name, "/speech_service/instance", "当前实例名称");
DEFINE_string(access_host, "127.0.0.1:10001", "当前实例的外部访问地址");

DEFINE_int32(listen_port, 10001, "Rpc服务器监听端口");
DEFINE_int32(rpc_timeout, -1, "Rpc调用超时时间");
DEFINE_int32(rpc_threads, 1, "Rpc的IO线程数量");

DEFINE_bool(run_mode, true, "程序的运行模式, true-调试； false-发布；");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

int main(int argc, char** argv)
{
    using namespace im;
    using namespace im::logger;

    google::ParseCommandLineFlags(&argc, &argv, true);
    LogSetting setting;
    setting.level = static_cast<Level>(FLAGS_log_level);
    setting.logger_type = FLAGS_run_mode ? LogType::Console : LogType::File;
    setting.logger_name = "speech";
    InitLogger(setting);

    im::SpeechServerBuilder ssb;
    ssb.MakeAsrObject(FLAGS_app_id, FLAGS_api_key, FLAGS_secret_key);
    ssb.MakeRpcServer(FLAGS_listen_port, FLAGS_rpc_timeout, FLAGS_rpc_threads);
    ssb.MakeRegObject(FLAGS_registry_host, FLAGS_base_service + FLAGS_instance_name, FLAGS_access_host);

    auto server = ssb.Build();
    server->Start();

    return 0;
}