add_executable(brpc_exam_server server.cc)
add_executable(brpc_exam_client client.cc)

generate_proto(
        FILE "echo.proto"
        OUTPUT_DIR "${PROTO_DIR}/echo"
)


target_link_libraries(brpc_exam_server
        ${GFLAGS_LIBRARIES}
        ${PROTOBUF_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        ${BRPC_LIBRARIES}
        ${ABSL_LIBS}

        echo_proto

        curl
        leveldb::leveldb
        spdlog::spdlog
)
target_link_libraries(brpc_exam_client
        ${GFLAGS_LIBRARIES}
        ${PROTOBUF_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        ${BRPC_LIBRARIES}
        ${ABSL_LIBS}

        echo_proto

        curl
        leveldb::leveldb
        spdlog::spdlog
)