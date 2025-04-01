add_executable(etcd_discovery ./discovery.cc
        server.cc)
add_executable(etcd_registry ./registry.cc)
add_executable(brpc_example server.cc)

target_link_libraries(etcd_discovery Helpers gflags)
target_link_libraries(etcd_registry Helpers gflags)
target_link_libraries(brpc_example PkgConfig::brpc)