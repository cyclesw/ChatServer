file(GLOB PROTO_FILES *.proto)


set(PROTO_DEPENDS_user "base.proto")
set(PROTO_DEPENDS_notify "base.proto")
set(PROTO_DEPENDS_message "base.proto")
set(PROTO_DEPENDS_friend "base.proto")
set(PROTO_DEPENDS_file "base.proto")
set(PROTO_DEPENDS_transmit "base.proto")

# 为文件添加DEPENDS
foreach(FILE ${PROTO_FILES})
    get_filename_component(FILE_NAME ${FILE} NAME_WE)

    # 构造依赖文件的绝对路径列表
    set(DEPENDS_LIST "")
    if(DEFINED PROTO_DEPENDS_${FILE_NAME})
        foreach(DEP ${PROTO_DEPENDS_${FILE_NAME}})
            list(APPEND DEPENDS_LIST "${CMAKE_CURRENT_SOURCE_DIR}/${DEP}")
        endforeach()
    endif()

    generate_proto(FILE "${FILE}"
            OUTPUT_DIR "${PROTO_DIR}/${FILE_NAME}"
            DEPENDS ${DEPENDS_LIST}  # 添加依赖
    )
endforeach()
