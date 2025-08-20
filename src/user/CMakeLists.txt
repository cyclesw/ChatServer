add_executable(user_server
        user_server.cc
        ../etcd.cc
        ../channel.cc
)
add_executable(user_test_1
        test/mysql_test.cc
)
add_executable(redis_test_1
       test/redis_test.cc
)

target_link_libraries(user_server PUBLIC
        ${BRPC_LIBRARIES}
        ${PROTOBUF_LIBRARIES}
        ${GFLAGS_LIBRARIES}
        ${ETCD_CPP_LIBRARIES}
        ${ABSL_LIBS}

        user_proto
        odb_user

        spdlog::spdlog
        leveldb::leveldb
        PkgConfig::ODBCPP
        PkgConfig::ODBCPP_MYSQL
        PkgConfig::ODBCPP_BOOST
)
target_link_libraries(user_test_1
        ${PROTOBUF_LIBRARIES}
        ${GFLAGS_LIBRARIES}
        ${ETCD_CPP_LIBRARIES}
        ${ABSL_LIBS}

        user_proto
        odb_user

        spdlog::spdlog
        PkgConfig::ODBCPP
        PkgConfig::ODBCPP_MYSQL
        PkgConfig::ODBCPP_BOOST
)
target_link_libraries(redis_test_1
        ${GFLAGS_LIBRARIES}
        PkgConfig::REDISPP
)
