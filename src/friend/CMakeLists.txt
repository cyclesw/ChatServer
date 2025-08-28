add_executable(friend_server
    friend_server.cc
    server.cc

    ../common/database/mysql_apply.cc
    ../common/database/mysql_chat_session_member.cc
    ../common/database/mysql_chat_session.cc
    ../common/database/mysql_relation.cc
    ../common/es.cc
    ../common/channel.cc
    ../common/rabbitmq.cc
    ../common/etcd.cc
)

target_link_libraries(friend_server
    ${BRPC_LIBRARIES}
    ${ETCD_CPP_LIBRARIES}
    ${ODBCPP_LIBRARIES}
    ${GFLAGS_LIBRARIES}
    ${ODBCPP_MYSQL_LIBRARIES}
    ${OPENSSL_LIBRARIES}

    odb_friend_apply
    odb_relation
    odb_chat_session_member
    odb_chat_session
    friend_proto
    user_proto
    message_proto

    spdlog::spdlog
    JsonCpp::JsonCpp
    leveldb::leveldb
    amqpcpp
    elasticlient
    ev

    cpr::cpr
)

add_executable(friend_client
    test/client.cc
    ../common/etcd.cc
    ../common/channel.cc

    ../common/database/mysql_apply.cc
    ../common/database/mysql_chat_session.cc
    ../common/database/mysql_chat_session_member.cc
    ../common/database/mysql_relation.cc
)
target_link_libraries(friend_client
    ${GFLAGS_LIBRARIES}
    ${ETCD_CPP_LIBRARIES}
    ${GFLAGS_LIBRARIES}
    ${ODBCPP_MYSQL_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${BRPC_LIBRARIES}

    friend_proto
    odb_relation
    odb_chat_session
    odb_friend_apply
    odb_chat_session_member


    spdlog::spdlog
    GTest::gtest
    leveldb::leveldb
)