add_executable(speech_exam test.cc)
target_link_libraries(speech_exam
        ${GFLAGS_LIBRARIES}

        JsonCpp::JsonCpp
        curl
        crypto
        spdlog::spdlog
)

add_executable(speech_client_exam speech_client.cc ../../etcd.cc ../../channel.cc)
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
