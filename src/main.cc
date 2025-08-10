#include "gateway/server.h"

#include <gflags/gflags.h>
#include <log.hpp>

DEFINE_bool(debug, true, "调试模式");
DEFINE_uint32(log_level, 0, "日志等级");

DEFINE_uint32(http_port, 8080, "http服务监听端口");
DEFINE_uint32(websocket_port, 8081, "websocket服务监听端口");

DEFINE_string(registry_host, "http://127.0.0.1:2379", "服务注册中心地址");
DEFINE_string(base_service, "/service", "服务监控根目录");
DEFINE_string(file_service, "/service/file_service", "文件存储子服务名称");
DEFINE_string(friend_service, "/service/friend_service", "好友管理子服务名称");
DEFINE_string(message_service, "/service/message_service", "消息存储子服务名称");
DEFINE_string(user_service, "/service/user_service", "用户管理子服务名称");
DEFINE_string(speech_service, "/service/speech_service", "语音识别子服务名称");
DEFINE_string(transmite_service, "/service/transmite_service", "转发管理子服务名称");

DEFINE_string(redis_host, "127.0.0.1", "Redis服务器访问地址");
DEFINE_int32(redis_port, 6379, "Redis服务器访问端口");
DEFINE_int32(redis_db, 0, "Redis默认库号");
DEFINE_bool(redis_keep_alive, true, "Redis长连接保活选项");

void T(int, int);
using T_callback = std::function<void(int, int)>;

int main(int argc, char* argv[])
{
    using namespace im;
    using namespace im::logger;
    typedef int connection_hdl;
    typedef int connection_hdl;

    google::ParseCommandLineFlags(&argc, &argv, true);

    LogSetting setting;
    setting.level = static_cast<Level>(FLAGS_log_level);
    setting.logger_type = FLAGS_debug ? LogType::Console : LogType::File;
    setting.logger_name = "gateway";
    InitLogger(setting);

    GatewayServerBuilder builder;
    builder.MakeRedisObject(FLAGS_redis_host, FLAGS_redis_port, FLAGS_redis_db, FLAGS_redis_keep_alive);
    builder.MakeDiscoveryObject(FLAGS_registry_host,
        FLAGS_base_service,
        FLAGS_file_service,
        FLAGS_speech_service,
        FLAGS_message_service,
        FLAGS_friend_service,
        FLAGS_user_service,
        FLAGS_transmite_service);


    LOG_TRACE("Trace");
    LOG_DEBUG("Debug");
    LOG_INFO("Info");
    LOG_WARN("Warn");
    LOG_ERROR("Error");
    LOG_CRITICAL("Critical");
    return 0;
}
