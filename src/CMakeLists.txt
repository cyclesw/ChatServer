file(GLOB common_hrd "include/*.h" "include/*.hpp")
file(GLOB common_src "include/*.cpp")

add_library(Helpers INTERFACE )
target_sources(Helpers INTERFACE ${common_hrd} ${common_src})
target_include_directories(Helpers INTERFACE ./include/)
target_link_libraries(Helpers INTERFACE etcd-cpp-api spdlog::spdlog)

add_executable(ChatServer main.cc)
target_link_libraries(ChatServer Helpers)
