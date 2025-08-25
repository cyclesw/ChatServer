#include "rabbitmq.h"
#include "log.hpp"
#include <chrono>
#include <gflags/gflags.h>
#include <thread>

DEFINE_string(user, "test_user", "rabbitmq访问用户名");
DEFINE_string(pswd, "123456", "rabbitmq访问密码");
DEFINE_string(host, "192.168.10.248:5672", "rabbitmq服务器地址信息 host:port");

DEFINE_bool(run_mode, false, "程序的运行模式, false-调试； true-发布；");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

void Callback(const char* body, size_t size)
{
    std::string msg(body, size);
    msg.assign(body, size);
    LOG_INFO("{}", msg);
}

int main(int argc, char** argv)
{
    using namespace im;
    google::ParseCommandLineFlags(&argc, &argv, true);
    LogSetting setting;
    setting.level = (Level)FLAGS_log_level;
    InitLogger(setting);

    MQClient client(FLAGS_user, FLAGS_pswd, FLAGS_host);
    client.DeclareComponents("test-exchange", "test-queue");
    client.Consume("test-queue", Callback);
    std::this_thread::sleep_for(std::chrono::seconds(60));
    return 0;
}