add_executable(user_exam
        test.cc
)

target_link_libraries(user_exam
        ${GFLAGS_LIBRARIES}

        odb_user

        spdlog::spdlog
        PkgConfig::ODBCPP
        PkgConfig::ODBCPP_MYSQL
        PkgConfig::ODBCPP_BOOST
)
