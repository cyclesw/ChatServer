//
// Created by 19396 on 25-5-16.
//
#include <gflags/gflags.h>
#include <iostream>

DEFINE_string(config, "default.conf", "Path to config file");  // 字符串类型
DEFINE_int32(port, 8080, "Server port");                      // 整数类型
DEFINE_bool(debug, false, "Enable debug mode");               // 布尔类型

int main(int argc, char** argv) {
    // 1. 解析命令行参数
    gflags::SetUsageMessage("gflags test");
    gflags::ParseCommandLineFlags(&argc, &argv, true);
   // 2. 使用参数
    std::cout << "Config path: " << FLAGS_config << std::endl;
    std::cout << "Server port: " << FLAGS_port << std::endl;

    if (FLAGS_debug) {
        std::cout << "Debug mode ON" << std::endl;
    } else {
        std::cout << "Debug mode OFF" << std::endl;
    }

    // 3. 程序逻辑...
    std::cout << "Starting server on port " << FLAGS_port << "..." << std::endl;

    // 4. 清理（可选）
    gflags::ShutDownCommandLineFlags();
    return 0;
}
