file(GLOB PROTO_FILES *.proto)

include_directories(${PROTO_DIR})

foreach(FILE ${PROTO_FILES})
    get_filename_component(FILE_NAME ${FILE} NAME_WE)
    GENERATE_PROTOBUF(FILE "${FILE}"
            OUTPUT_DIR "${PROTO_DIR}/${FILE_NAME}"
    )
endforeach ()