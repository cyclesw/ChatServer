add_executable(odbexam test.cc)
target_link_libraries(odbexam
        odb_person

        PkgConfig::ODBCPP
        PkgConfig::ODBCPP_MYSQL
        PkgConfig::ODBCPP_BOOST
)
