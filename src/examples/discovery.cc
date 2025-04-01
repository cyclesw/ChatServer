//
// Created by cyclesw on 25-3-31.
//

#include <etcd.h>
#include <log.hpp>
#include <gflags/gflags.h>

DEFINE_string(LOGGER_NAME, "CONSOLE", "spdlog logger name");
DEFINE_string(ETCD_HOST, "http://127.0.0.1:2379", "服务注册中心嫡长子");
DEFINE_string(BASE_SERVICE, "/service", "服务监控根目录");
DEFINE_string(INSTANCE_NAME, "/friend/instance", "当前实例名称");

void online(const std::string& service_name, const std::string& service_host)
{
    LOG_DEBUG("上线服务:{}-{}", service_name, service_host);
}

void offline(const std::string& service_name, const std::string& service_host)
{
    LOG_DEBUG("下线服务:{}-{}", service_name, service_host);
}

int main(int argc, char** argv)
{
    using namespace ns_chat;
    google::ParseCommandLineFlags(&argc, &argv, true);
    LogSetting setting;
    setting.logger_name = FLAGS_LOGGER_NAME;
    Init_Logger(setting);
    LOG_TRACE("etcd-discovery services");

    DiscoveryPtr dclient = std::make_shared<Discovery>(FLAGS_ETCD_HOST,
        FLAGS_BASE_SERVICE,online, offline);

    std::this_thread::sleep_for(std::chrono::seconds(60));

    return 0;
}