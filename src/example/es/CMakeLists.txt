add_executable(es_exam
        test.cc
        ../../common/es.cc
)

target_link_libraries(es_exam
        ${GFLAGS_LIBRARIES}

        elasticlient
        JsonCpp::JsonCpp
        cpr::cpr

        spdlog::spdlog
)