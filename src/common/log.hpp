//
// Created by lang liu on 2024/9/1.
//

#ifndef LOG_HPP
#define LOG_HPP

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include <memory>
#include <spdlog/async.h>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <stdexcept>


namespace im::logger
{

    using Level = spdlog::level::level_enum;
    inline std::shared_ptr<spdlog::logger> g_logger = spdlog::default_logger();

    enum class LogType
    {
        Console = 0,
        File,
    };

    struct LogSetting
    {
        size_t max_size{};
        size_t max_file{};
        std::string logger_name = "default";
        std::string filename = "log";

        Level level = spdlog::level::trace;
        LogType logger_type = LogType::Console;
    };

    inline void InitLogger(const LogSetting &setting)
    {

        switch (setting.logger_type)
        {
            case LogType::File:
            {
                g_logger = spdlog::rotating_logger_mt(setting.logger_name, setting.filename, setting.max_size,
                                                      setting.max_file);
                g_logger->flush_on(spdlog::level::info);
                break;
            }
            case LogType::Console:
            {
                g_logger = spdlog::stdout_color_mt(setting.logger_name);
                g_logger->flush_on(spdlog::level::trace);
                break;
            }
            default:
                throw std::runtime_error("logger_type unknown");
        };
        g_logger->set_pattern("[%^%l%$] [%n] [%Y-%m-%d %H:%M:%S] [%P:%t] [%s:%#] %v");
        g_logger->set_level(setting.level);
    }

#define LOG_TRACE(...) SPDLOG_LOGGER_TRACE(im::logger::g_logger, __VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(im::logger::g_logger, __VA_ARGS__)
#define LOG_INFO(...) SPDLOG_LOGGER_INFO(im::logger::g_logger, __VA_ARGS__)
#define LOG_WARN(...) SPDLOG_LOGGER_WARN(im::logger::g_logger, __VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_LOGGER_ERROR(im::logger::g_logger, __VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(im::logger::g_logger, __VA_ARGS__)

} // namespace im::logger


#endif // LOG_HPP
