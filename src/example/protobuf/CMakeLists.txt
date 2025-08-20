add_executable(proto_exam test.cc)
target_link_libraries(proto_exam
        ${PROTOBUF_LIBRARIES}
        ${ABSL_LIBS}

        user_proto
        base_proto
)
