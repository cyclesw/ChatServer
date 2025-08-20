add_subdirectory(odb)
add_subdirectory(example)
add_subdirectory(speech)
add_subdirectory(user)
add_subdirectory(file)


add_executable(gateway
        common/log.hpp
        common/redis.hpp
        main.cc
        connection.cc
        channel.cc
        server.cc
        etcd.cc
)


target_include_directories(gateway PUBLIC "gateway")

target_link_libraries(gateway PUBLIC
        ${BRPC_LIBRARIES}
        ${PROTOBUF_LIBRARIES}
        ${GFLAGS_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        ${REDISPP_LIBRARIES}
        ${ETCD_CPP_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        ${ABSL_LIBS}

        # protobuf libs
        base_proto
        user_proto
        gateway_proto
        friend_proto
        message_proto
        notify_proto

        # un
        websocketpp::websocketpp
        httplib::httplib
        spdlog::spdlog

        leveldb::leveldb
)