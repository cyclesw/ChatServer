add_executable(amqpcpp_exam
        test.cc
)

target_link_libraries(amqpcpp_exam
        amqpcpp

        ev
)