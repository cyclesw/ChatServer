cmake_minimum_required(VERSION 3.24)

project(ChatSystemServer)

set(CMAKE_CXX_STANDARD  17)
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

if(CMAKE_EXPORT_COMPILE_COMMANDS)
  # set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES 
  #     ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

find_package(PkgConfig REQUIRED)
find_package(spdlog REQUIRED)


add_subdirectory(thirdparty)
add_subdirectory(src)