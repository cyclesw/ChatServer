find_package(GTest REQUIRED)

add_executable(websocketpp_test websocketpp/test.cc)
add_executable(proto_test protobuf/test.cc)
add_executable(httplib_test httplib/test.cc)
add_executable(gflags_test gflags/test.cc)
add_executable(speech_test speech/test.cc )
add_executable(speech_client_test speech/speech_client.cc ../etcd.cc ../channel.cc)
add_executable(file_client_test file/test.cc ../etcd.cc ../channel.cc)


# message(STATUS "absl: ${ABSL_LIBS}")
include_directories(${CMAKE_SOURCE_DIR}/src/common)
include_directories(${CMAKE_SOURCE_DIR}/thirdparty)

target_link_libraries(file_client_test 
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
target_link_libraries(speech_client_test 
        ${GFLAGS_LIBRARIES}
        ${PROTOBUF_LIBRARIES}
        ${ETCD_CPP_LIBRARIES}
        ${BRPC_LIBRARIES}
        ${ABSL_LIBS}

        speech_proto

        JsonCpp::JsonCpp
        leveldb::leveldb
        spdlog::spdlog
        curl
        crypto
)
target_link_libraries(websocketpp_test websocketpp::websocketpp)
target_link_libraries(proto_test 
        ${PROTOBUF_LIBRARIES}
        ${ABSL_LIBS}

        user_proto
        base_proto
)
target_link_libraries(httplib_test ${HTTPLIB_LIBRARY})
target_link_libraries(gflags_test ${GFLAGS_LIBRARIES})
target_link_libraries(speech_test 
        ${GFLAGS_LIBRARIES}

        JsonCpp::JsonCpp
        curl
        crypto
        spdlog::spdlog
)