add_executable(rabbitmq_publish_exam
        publish.cc
        ../../common/rabbitmq.cc
)

target_link_libraries(rabbitmq_publish_exam
        ${GFLAGS_LIBRARIES}

        amqpcpp
        spdlog::spdlog
        ev
)

add_executable(amqp_exam
        ./test.cc
)
target_link_libraries(amqp_exam
        amqpcpp
)

add_executable(rabbitmq_consume_exam
        consume.cc
        ../../common/rabbitmq.cc
)
target_link_libraries(rabbitmq_consume_exam
        ${GFLAGS_LIBRARIES}

        amqpcpp
        spdlog::spdlog
        ev
)
