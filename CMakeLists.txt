cmake_minimum_required(VERSION 3.24)

project(ChatSystemServer)

set(CMAKE_CXX_STANDARD  20)
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

if(CMAKE_EXPORT_COMPILE_COMMANDS)
  set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES 
      ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    message(STATUS "Build Debug Version")
    add_definitions(-DDEBUG=1)
endif ()

enable_testing()

option(BUILD_CHAT_SERVER_TESTS "Build ChatServer Tests"  ON)
option(BUILD_CHAT_SERVER_EXAMPLES "Build ChatServer Examples"  ON)

find_package(PkgConfig REQUIRED)
find_package(spdlog REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(gflags REQUIRED)
find_package(etcd-cpp-api CONFIG REQUIRED)

pkg_check_modules(brpc REQUIRED IMPORTED_TARGET brpc)

add_subdirectory(thirdparty)
add_subdirectory(src)
