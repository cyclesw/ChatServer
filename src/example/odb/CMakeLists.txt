add_executable(odb_exam test.cc)
target_link_libraries(odb_exam
        odb_person

        PkgConfig::ODBCPP
        PkgConfig::ODBCPP_MYSQL
        PkgConfig::ODBCPP_BOOST
)
