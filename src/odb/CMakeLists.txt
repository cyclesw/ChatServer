#TODO generate odb file
file(GLOB ODB_FILES *.hxx)

set(message_PROFILE "boost/date-time")


foreach (FILE ${ODB_FILES})
    get_filename_component(FILE_NAME ${FILE} NAME_WE)

    generate_odb_files(
        INPUT ${FILE}
        OUTPUT_DIR "${ODB_DIR}/${FILE_NAME}"
        DATABASE mysql
        STD 17
        PROFILE ${${FILE_NAME}_PROFILE}
    )
endforeach ()