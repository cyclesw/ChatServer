add_executable(file_client_exam test.cc ../../common/etcd.cc ../../common/channel.cc)
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
