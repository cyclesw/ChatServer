cmake_minimum_required(VERSION 3.20)

project(ChatServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(PROTO_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated" CACHE STRING "Directory for generated protobuf files")
set(ODB_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated_odb")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/odb)

add_definitions(-DASIO_STANDALONE)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(STATUS "Use Clang")
    set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

include(cmake/CMakeLists.txt)

#set(MY_GLOBAL_VAR_STRING_NOFORCE "abcdef" CACHE STRING "定义一个STRING缓存变量")


find_package(PkgConfig REQUIRED)
find_package(Boost REQUIRED COMPONENTS random)
find_package(OpenSSL REQUIRED)
find_package(websocketpp REQUIRED)
find_package(spdlog REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(Gflags REQUIRED)
find_package(absl REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(leveldb REQUIRED)
find_package(httplib REQUIRED)
find_package(etcd-cpp-api REQUIRED)
find_package(GTest REQUIRED)
find_package(amqpcpp REQUIRED)
find_package(cpr REQUIRED)
find_package(jsoncpp)
if (NOT jsoncpp_FOUND)
    pkg_check_modules(JSONCPP REQUIRED IMPORTED_TARGET jsoncpp)
    add_library(JsonCpp::JsonCpp ALIAS PkgConfig::JSONCPP)
endif()


pkg_check_modules(REDISPP REQUIRED IMPORTED_TARGET redis++)
pkg_check_modules(BRPC REQUIRED IMPORTED_TARGET brpc)
pkg_check_modules(ODBCPP REQUIRED IMPORTED_TARGET libodb)
pkg_check_modules(ODBCPP_MYSQL REQUIRED IMPORTED_TARGET libodb-mysql)
pkg_check_modules(ODBCPP_BOOST REQUIRED IMPORTED_TARGET libodb-boost)

set(ABSL_LIBS
    absl_str_format_internal
    absl_strings
    absl_strings_internal
    absl_log_initialize
    absl_log_entry
    absl_status
    absl_log_flags
    absl_log_severity
    absl_log_internal_check_op
    absl_log_internal_conditions
    absl_log_internal_message
    absl_log_internal_nullguard
    absl_log_internal_proto
    absl_log_internal_format
    absl_log_internal_globals
    absl_log_internal_log_sink_set
    absl_log_sink
    absl_raw_logging_internal
    absl_log_globals
    utf8_validity
    absl_cord
    absl_cordz_info
    absl_cordz_handle
    absl_cordz_functions
    absl_cord_internal
    absl_crc_cord_state
    absl_crc32c
    absl_crc_internal
    absl_exponential_biased
    absl_synchronization
    absl_time
    absl_time_zone
    absl_int128
    absl_examine_stack
    absl_stacktrace
    absl_symbolize
    absl_demangle_internal
    absl_debugging_internal
    absl_malloc_internal
    absl_throw_delegate
    absl_strerror
    absl_raw_hash_set
    absl_hash
    absl_city
    absl_low_level_hash
    absl_base
    absl_spinlock_wait
)

# include_directories(${Protobuf_INCLUDE_DIRS})
# include_directories(${CMAKE_CURRENT_BINARY_DIR})



message(STATUS "---------------------------------------------------------------------")

add_subdirectory(proto)
add_subdirectory(src)

message(STATUS "---------------------------------------------------------------------")
