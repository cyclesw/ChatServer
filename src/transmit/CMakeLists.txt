add_executable(transmit_server
        ./server.cc
        transmit_server.cc
        ../common/database/mysql_chat_session_member.cc
        ../common/database/mysql_chat_session.cc
        ../common/rabbitmq.cc
)


target_link_libraries(transmit_server
        ${BRPC_LIBRARIES}
        ${PROTOBUF_LIBRARIES}
        ${GFLAGS_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        ${REDISPP_LIBRARIES}
        ${ETCD_CPP_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        ${ODBCPP_LIBRARIES}
        ${ODBCPP_MYSQL_LIBRARIES}
        ${ODBCPP_BOOST_LIBRARIES}
        ${ABSL_LIBS}

        user_proto
        transmit_proto
        odb_chat_session
        odb_chat_session_member

        spdlog::spdlog
        leveldb::leveldb
        amqpcpp

        ev
)
