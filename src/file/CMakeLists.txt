add_executable(file_server
        file_server.cc

        ../common/etcd.cc
        ../common/channel.cc
)
target_link_libraries(file_server PUBLIC
        ${GFLAGS_LIBRARIES}
        ${PROTOBUF_LIBRARIES}
        ${ETCD_CPP_LIBRARIES}
        ${BRPC_LIBRARIES}
        ${ABSL_LIBS}

        file_proto

        curl
        crypto
        leveldb::leveldb
        spdlog::spdlog
)