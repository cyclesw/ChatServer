//
// Created by lang liu on 2024/9/1.
//

#ifndef LOG_HPP
#define LOG_HPP

#ifdef Debug
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE_
#endif

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

struct LogSetting 
{
    
};

inline void Init_Logger() 
{
  
}

#endif // LOG_HPP
