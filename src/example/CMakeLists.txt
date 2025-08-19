find_package(GTest REQUIRED)

generate_odb_files(
        INPUT ${CMAKE_CURRENT_SOURCE_DIR}/odb/person.hxx
        OUTPUT_DIR "${ODB_DIR}/person"
        DATABASE mysql
        STD 17
        PROFILE boost/date-time
)

add_executable(websocketpp_exam websocketpp/test.cc)
add_executable(proto_exam protobuf/test.cc)
add_executable(httplib_exam httplib/test.cc)
add_executable(gflags_exam gflags/test.cc)
add_executable(speech_exam speech/test.cc )
add_executable(speech_client_exam speech/speech_client.cc ../etcd.cc ../channel.cc)
add_executable(file_client_exam file/test.cc ../etcd.cc ../channel.cc)
add_executable(odbexam odb/test.cc
)
add_executable(redis_exam redis/test.cc)

include_directories(${CMAKE_SOURCE_DIR}/src/common)
include_directories(${CMAKE_SOURCE_DIR}/thirdparty)

target_link_libraries(file_client_exam
        ${GFLAGS_LIBRARIES}
        ${ETCD_CPP_LIBRARIES}
        ${BRPC_LIBRARIES}
        ${PROTOBUF_LIBRARIES}
        ${ABSL_LIBS}

        base_proto
        file_proto

        GTest::gtest
        leveldb::leveldb
        spdlog::spdlog  
)
target_link_libraries(speech_client_exam
        ${GFLAGS_LIBRARIES}
        ${PROTOBUF_LIBRARIES}
        ${ETCD_CPP_LIBRARIES}
        ${BRPC_LIBRARIES}
        ${Boost_LIBRARIES}
        ${ABSL_LIBS}

        speech_proto

        JsonCpp::JsonCpp
        leveldb::leveldb
        spdlog::spdlog
        curl
        crypto
)
target_link_libraries(websocketpp_exam websocketpp::websocketpp)
target_link_libraries(proto_exam
        ${PROTOBUF_LIBRARIES}
        ${ABSL_LIBS}

        user_proto
        base_proto
)
target_link_libraries(httplib_exam ${HTTPLIB_LIBRARY})
target_link_libraries(gflags_exam ${GFLAGS_LIBRARIES})
target_link_libraries(speech_exam
        ${GFLAGS_LIBRARIES}

        JsonCpp::JsonCpp
        curl
        crypto
        spdlog::spdlog
)
target_link_libraries(odbexam
        odb_person

        PkgConfig::ODBCPP
        PkgConfig::ODBCPP_MYSQL
        PkgConfig::ODBCPP_BOOST
)
target_link_libraries(redis_exam
    ${GFLAGS_LIBRARIES}

    PkgConfig::REDISPP
)