#include <iostream>
#include <vector>
#include "log.hpp"

int main() 
{
  using namespace ns_chat;
  LogSetting setting;
  setting.logger_name = "CONSOLE";

  Init_Logger(setting);

  std::vector<int> v;
  v.resize(123);
  LOG_TRACE("trace");
  LOG_DEBUG("debug");
  LOG_INFO("info");
  LOG_WARN("warn");
  LOG_ERROR("error");
  LOG_CRITICAL("critical");

  return 0;
}
