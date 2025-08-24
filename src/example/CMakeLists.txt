generate_odb_files(
        INPUT ${CMAKE_CURRENT_SOURCE_DIR}/odb/person.hxx
        OUTPUT_DIR "${ODB_DIR}/person"
        DATABASE mysql
        STD 17
        PROFILE boost/date-time
)

include_directories(${CMAKE_SOURCE_DIR}/src/common)
include_directories(${CMAKE_SOURCE_DIR}/thirdparty)

# 添加所有子目录
add_subdirectory(websocketpp)
add_subdirectory(protobuf)
add_subdirectory(httplib)
add_subdirectory(gflags)
add_subdirectory(speech)
add_subdirectory(file)
add_subdirectory(odb)
add_subdirectory(redis)
add_subdirectory(es)
add_subdirectory(user)
add_subdirectory(dms)
add_subdirectory(brpc)
add_subdirectory(ev)
add_subdirectory(amqpcpp)