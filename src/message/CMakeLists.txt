add_executable(message_server
        message_server.cc
        server.cc
        ../common/database/mysql_message.cc
        ../common/etcd.cc
        ../common/channel.cc
        ../common/rabbitmq.cc
        ../common/es.cc
)

target_link_libraries(message_server
        ${ETCD_CPP_LIBRARIES}
        ${BRPC_LIBRARIES}
        ${GFLAGS_LIBRARIES}
        ${ODBCPP_LIBRARIES}
        ${ODBCPP_MYSQL_LIBRARIES}
        ${ODBCPP_BOOST_LIBRARIES}

        user_proto
        file_proto
        message_proto
        odb_message

        spdlog::spdlog
        leveldb::leveldb
        JsonCpp::JsonCpp
        cpr::cpr

        elasticlient
        amqpcpp
        ev
)

if (BUILD_TEST)
    add_executable(message_client
            test/test.cc
            ../common/etcd.cc
            ../common/channel.cc
            ../common/es.cc
            ../common/database/mysql_message.cc
    )

    target_link_libraries(message_client
            ${ETCD_CPP_LIBRARIES}
            ${GFLAGS_LIBRARIES}
            ${BRPC_LIBRARIES}
            ${ODBCPP_LIBRARIES}
            ${ODBCPP_MYSQL_LIBRARIES}
            ${ODBCPP_BOOST_LIBRARIES}

            user_proto
            message_proto
            odb_message

            GTest::gtest
            spdlog::spdlog
            leveldb::leveldb
            JsonCpp::JsonCpp
            cpr::cpr

            elasticlient
            amqpcpp
            ev
    )
endif ()