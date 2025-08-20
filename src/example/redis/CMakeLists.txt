add_executable(redis_exam test.cc)
target_link_libraries(redis_exam
    ${GFLAGS_LIBRARIES}

    PkgConfig::REDISPP
)
