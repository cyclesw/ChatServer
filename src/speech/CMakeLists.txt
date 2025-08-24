add_executable(speech_server
        speech_server.cc
        ../common/etcd.cc
)

target_link_libraries(speech_server PUBLIC
        ${ETCD_CPP_LIBRARIES}
        ${BRPC_LIBRARIES}
        ${GFLAGS_LIBRARIES}
        ${PROTOBUF_LIBRARIES}
        ${ABSL_LIBS}

        speech_proto


        JsonCpp::JsonCpp
        curl
        crypto
        leveldb::leveldb
        spdlog::spdlog
)