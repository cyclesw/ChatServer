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