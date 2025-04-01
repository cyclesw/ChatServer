file(GLOB common_hrd "include/*.h" "include/*.hpp")

set (common_src
        ./etcd.cc
)

add_library(Helpers INTERFACE)
target_sources(Helpers INTERFACE ${common_hrd} ${common_src})
target_include_directories(Helpers INTERFACE ./include/)
target_link_libraries(Helpers INTERFACE etcd-cpp-api spdlog::spdlog)

if (BUILD_CHAT_SERVER_TESTS)
    message("Build Tests")
    add_subdirectory(tests)
endif ()

if (BUILD_CHAT_SERVER_EXAMPLES)
    message("Build Examples")
    add_subdirectory(examples)
endif ()

add_executable(ChatServer main.cc
)
target_link_libraries(ChatServer PUBLIC Helpers)