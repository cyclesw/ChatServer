add_executable(dms_exam
        test.cc
        ../../common/dms.cc
)
#message(STATUS "${HTTPLIB_LIBRARIES}")

target_link_libraries(
        dms_exam

        httplib::httplib
        spdlog::spdlog
)