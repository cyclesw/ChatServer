add_executable(gateway_server
        main.cc
        connection.cc
        server.cc

        ../common/channel.cc
        ../common/etcd.cc
        ../common/database/redis.cc
)

target_link_libraries(gateway_server
        ${GFLAGS_LIBRARIES}
        ${ETCD_CPP_LIBRARIES}
        ${GFLAGS_LIBRARIES}
        ${BRPC_LIBRARIES}
        ${REDISPP_LIBRARIES}

        gateway_proto
        friend_proto
        user_proto
        transmit_proto
        message_proto
        notify_proto
        file_proto
        speech_proto

        spdlog::spdlog
        leveldb::leveldb

        amqpcpp
)
