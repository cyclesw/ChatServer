//
// Created by lang liu on 2024/9/1.
//

#ifndef LOG_HPP
#define LOG_HPP

#include <memory>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/sink.h>
#include <stdexcept>

#if defined (DEBUG) || defined (_DEBUG)
    #define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#endif

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>


namespace ns_chat 
{

    inline std::shared_ptr<spdlog::logger> g_logger = nullptr;

    enum class LogType
    {
        Console,
        File, 
    };

    struct LogSetting 
    {
        size_t max_size{};
        size_t max_file{};
        std::string logger_name{};
        std::string filename{};

        spdlog::level::level_enum level{};
        LogType logger_type{};
    };

    inline void Init_Logger(LogSetting setting) 
    {
        
        switch (setting.logger_type) 
        {
            case LogType::File:
            {
                g_logger = spdlog::rotating_logger_mt(setting.logger_name, setting.logger_name, setting.max_size,
                                setting.max_file);
                break;
            }
            case LogType::Console:
            {
                g_logger = spdlog::stdout_color_mt(setting.logger_name);
                break;
            }
            default: throw std::runtime_error("logger_type unknown");
        };
        
        g_logger->set_pattern("[%^%l%$] [%Y-%m-%d %H:%M:%S] [%n] [%P:%t] [%s:%#] %v");
    }

    #define LOG_TRACE(...)      SPDLOG_LOGGER_TRACE(ns_chat::g_logger, __VA_ARGS__)
    #define LOG_DEBUG(...)      SPDLOG_LOGGER_DEBUG(ns_chat::g_logger, __VA_ARGS__)
    #define LOG_INFO(...)       SPDLOG_LOGGER_INFO(ns_chat::g_logger, __VA_ARGS__)
    #define LOG_WARN(...)       SPDLOG_LOGGER_WARN(ns_chat::g_logger, __VA_ARGS__)
    #define LOG_ERROR(...)      SPDLOG_LOGGER_ERROR(ns_chat::g_logger, __VA_ARGS__)
    #define LOG_CRITICAL(...)   SPDLOG_LOGGER_CRITICAL(ns_chat::g_logger, __VA_ARGS__)
}

#endif // LOG_HPP
