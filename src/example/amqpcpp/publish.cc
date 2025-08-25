#include "rabbitmq.h"
#include "log.hpp"
#include <gflags/gflags.h>

DEFINE_string(user, "test_user", "rabbitmq访问用户名");
DEFINE_string(pswd, "123456", "rabbitmq访问密码");
DEFINE_string(host, "192.168.10.248:5672", "rabbitmq服务器地址信息 host:port");


DEFINE_bool(run_mode, false, "程序的运行模式, false-调试； true-发布；");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");


int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    LOG_INFO("发布端启动");

    im::MQClient client(FLAGS_user, FLAGS_pswd, FLAGS_host);

    client.DeclareComponents("test-exchange", "test-queue");

    for (int i = 0; i < 10; i++) {
        std::string msg = "Hello Bite-" + std::to_string(i);
        bool ret = client.Publish("test-exchange", msg);
        if (ret == false) 
            LOG_ERROR("publish 失败");
    }

    std::this_thread::sleep_for(std::chrono::seconds(3));
    return 0;
}