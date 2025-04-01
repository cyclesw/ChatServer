pkg_check_modules(Catch2WithMain IMPORTED_TARGET catch2-with-main)

if (Catch2WithMain_FOUND EQUAL FALSE)
    message(STATUS "CatchWithMain Not Found")
    include(FetchContent)
    FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.6.0
    )
    FetchContent_MakeAvailable(Catch2)
endif ()


set(TESTS_SOURCES
        etcd_test.cc
)

string(REPLACE ".cc" "" TESTS_NAMES "${TESTS_SOURCES}")

foreach (name ${TESTS_NAMES})
    add_executable(${name} ${name}.cc)
    target_include_directories(${name} PUBLIC ${PROJECT_SOURCE_DIR}/src/include)
    target_link_libraries(${name} Catch2WithMain)
endforeach ()

target_link_libraries(etcd_test etcd-cpp-api)

