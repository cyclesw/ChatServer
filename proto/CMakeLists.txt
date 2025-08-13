file(GLOB PROTO_FILES *.proto)

include_directories(${PROTO_DIR})

#TODO: 为文件添加DEPENDS
foreach(FILE ${PROTO_FILES})
    get_filename_component(FILE_NAME ${FILE} NAME_WE)
#    message(STATUS "FILE: ${FILE}")
    GENERATE_PROTOBUF(FILE "${FILE}"
            OUTPUT_DIR "${PROTO_DIR}/${FILE_NAME}"
    )
endforeach ()
