file(GLOB common_src include "*.h" "*.cc")

add_library(Helpers INTERFACE )
target_sources(Helpers INTERFACE ${common_src})
target_link_libraries(Helpers INTERFACE etcd-cpp-api)

add_executable(ChatServer main.cc)
target_link_libraries(ChatServer spdlog::spdlog)
# target_include_directories(ChatServer PUBLIC include)
# target_link_libraries(ChatServer Helpers)
