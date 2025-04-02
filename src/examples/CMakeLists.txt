add_executable(etcd_discovery ./discovery.cc)
add_executable(etcd_registry ./registry.cc)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ./main.proto)

add_executable(brpc_server_example server.cc ${PROTO_SRCS} ${PROTO_HDRS})
add_executable(brpc_client_example client.cc ${PROTO_SRCS} ${PROTO_HDRS})
target_sources(brpc_server_example PUBLIC ${PROTO_HDRS})

target_link_libraries(etcd_discovery Helpers gflags)
target_link_libraries(etcd_registry Helpers gflags)
target_link_libraries(brpc_server_example PkgConfig::brpc OpenSSL::SSL gflags protobuf::libprotobuf)
target_link_libraries(brpc_client_example PkgConfig::brpc OpenSSL::SSL gflags protobuf::libprotobuf)
