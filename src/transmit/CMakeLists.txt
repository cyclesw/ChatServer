add_executable(transmit_server
        transmit_server.cc
        ../common/database/mysql_chat_session_member.cc
        ../common/database/mysql_chat_session.cc
        ../common/rabbitmq.cc
        ../common/channel.cc
        ../common/etcd.cc
)


target_link_libraries(transmit_server
        ${GFLAGS_LIBRARIES}
        ${BRPC_LIBRARIES}
        ${REDISPP_LIBRARIES}
        ${ETCD_CPP_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        ${ODBCPP_LIBRARIES}
        ${ODBCPP_MYSQL_LIBRARIES}

        user_proto
        transmit_proto
        odb_chat_session
        odb_chat_session_member

        spdlog::spdlog
        leveldb::leveldb
        amqpcpp

        ev
)


add_executable(transmit_client
        test/transmit_client.cc
        ../common/channel.cc
        ../common/etcd.cc
)

target_link_libraries(transmit_client
        ${GFLAGS_LIBRARIES}
        ${BRPC_LIBRARIES}
        ${ETCD_CPP_LIBRARIES}

        transmit_proto

        GTest::gtest
        spdlog::spdlog
        leveldb::leveldb
        amqpcpp
)

add_executable(mysql_chat_test
        test/test.cc
        ../common/database/mysql_chat_session_member.cc
        ../common/database/mysql_chat_session.cc
)

target_link_libraries(mysql_chat_test
        ${GFLAGS_LIBRARIES}
        ${ODBCPP_LIBRARIES}
        ${ODBCPP_MYSQL_LIBRARIES}

        odb_chat_session
        odb_chat_session_member

        spdlog::spdlog
)


