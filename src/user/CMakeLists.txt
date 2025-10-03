add_executable(user_server
        user_server.cc
        server.cc
        ../common/es.cc
        ../common/etcd.cc
        ../common/dms.cc
        ../common/channel.cc
        ../common/database/user_table.cc
        ../common/database/redis.cc
)


target_link_libraries(user_server PUBLIC
        ${PROTOBUF_LIBRARIES}
        ${BRPC_LIBRARIES}
        ${GFLAGS_LIBRARIES}
        ${ETCD_CPP_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        ${REDISPP_LIBRARIES}
        ${ODBCPP_LIBRARIES}
        ${ODBCPP_MYSQL_LIBRARIES}
        ${ODBCPP_BOOST_LIBRARIES}
        ${ABSL_LIBS}

        user_proto
        file_proto
        odb_user

        spdlog::spdlog
        leveldb::leveldb
        cpr::cpr

        JsonCpp::JsonCpp

        elasticlient
)

if (BUILD_TEST)

    add_executable(user_test_1
            test/mysql_test.cc
            ../common/database/user_table.cc
    )
    add_executable(redis_test_1
            test/redis_test.cc
            ../common/database/redis.cc
    )
    add_executable(user_client
            test/user_client.cc
            ../common/etcd.cc
            ../common/dms.cc
            ../common/channel.cc
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
            ${REDISPP_LIBRARIES}
            spdlog::spdlog
    )
    target_link_libraries(user_client
            ${PROTOBUF_LIBRARIES}
            ${GFLAGS_LIBRARIES}
            ${ETCD_CPP_LIBRARIES}
            ${BRPC_LIBRARIES}
            ${ABSL_LIBS}

            user_proto
            odb_user

            spdlog::spdlog
            leveldb::leveldb
            GTest::gtest

            PkgConfig::ODBCPP
            PkgConfig::ODBCPP_MYSQL
            PkgConfig::ODBCPP_BOOST
    )
endif ()
