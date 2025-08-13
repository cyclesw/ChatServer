#include "asr.h"
#include "log.hpp"
#include <gflags/gflags.h>

#include <filesystem>

//TODO: 项目完成关闭api密钥
DEFINE_string(app_id, "116699311", "语音平台应用ID");
DEFINE_string(api_key, "C1MwX3PbmzFE01KkVdIIRZKd", "语音平台API密钥");
DEFINE_string(secret_key, "70GNIUw93umJDJb4qriFALRmDdkopBtk", "语音平台加密密钥");

DEFINE_bool(run_mode, true, "程序的运行模式，true-调试； false-发布；");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");



int main(int argc, char** argv)
{
    using namespace im;
    using namespace im::logger;

    google::ParseCommandLineFlags(&argc, &argv,  true);
    LogSetting setting;
    setting.level = static_cast<Level>(FLAGS_log_level);
    setting.logger_type = FLAGS_run_mode ? LogType::Console : LogType::File;
    setting.logger_name = "gateway";
    InitLogger(setting);

    ASRClient client(FLAGS_app_id, FLAGS_api_key, FLAGS_secret_key);

    std::string file_content;
    aip::get_file_content("16k.pcm", &file_content);

    if (file_content.empty())
    {
        LOG_ERROR("未能读取文件数据, 当前工作目录：{}", std::filesystem::current_path().string());
        return 1;
    }

    std::string message;

    bool ret = client.Recognize(file_content, message);

    LOG_INFO(message);
    return 0;
}