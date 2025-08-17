add_subdirectory(odb)
add_subdirectory(example)


#TODO: 是否需要objects

add_executable(speech_server 
        speech/speech_server.cc
        etcd.cc 
)
add_executable(gateway
        common/log.hpp
        common/redis.hpp
        main.cc
        connection.cc
        channel.cc
        server.cc
        etcd.cc
)
add_executable(file_server
        file/file_server.cc

        etcd.cc
        channel.cc
)
add_executable(user_server
        user/user_server.cc
        etcd.cc
        channel.cc
)


target_include_directories(gateway PUBLIC "gateway")


# link
target_link_libraries(file_server PUBLIC
        ${GFLAGS_LIBRARIES}
        ${PROTOBUF_LIBRARIES}
        ${ETCD_CPP_LIBRARIES}
        ${BRPC_LIBRARIES}
        ${ABSL_LIBS}

        file_proto

        curl
        crypto
        leveldb::leveldb
        spdlog::spdlog
)

target_link_libraries(speech_server PUBLIC
        ${ETCD_CPP_LIBRARIES}
        ${BRPC_LIBRARIES}
        ${GFLAGS_LIBRARIES}
        ${PROTOBUF_LIBRARIES}
        ${ABSL_LIBS}

        speech_proto


        JsonCpp::JsonCpp
        curl
        crypto
        leveldb::leveldb
        spdlog::spdlog
)
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