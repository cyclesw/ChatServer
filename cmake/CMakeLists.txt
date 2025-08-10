function(GENERATE_PROTOBUF)
    set(oneValueArgs FILE OUTPUT_DIR)
    set(multiValueArgs DEPENDS)
    cmake_parse_arguments(GEN_PROTO "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # 获取文件名（不带路径和扩展名）
    get_filename_component(PROTO_NAME ${GEN_PROTO_FILE} NAME_WE)
    get_filename_component(PROTO_ABS_PATH ${GEN_PROTO_FILE} ABSOLUTE)

    # 设置生成的 .cc 和 .h 文件路径
    set(GEN_PROTO_SRC "${GEN_PROTO_OUTPUT_DIR}/${PROTO_NAME}.pb.cc")
    set(GEN_PROTO_HDR "${GEN_PROTO_OUTPUT_DIR}/${PROTO_NAME}.pb.h")


    # 确保输出目录存在
    file(MAKE_DIRECTORY ${GEN_PROTO_OUTPUT_DIR})

    # 添加自定义命令生成 protobuf 文件
    add_custom_command(
            OUTPUT ${GEN_PROTO_SRC} ${GEN_PROTO_HDR}
            COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
            ARGS --cpp_out=${GEN_PROTO_OUTPUT_DIR} -I ${CMAKE_CURRENT_SOURCE_DIR} ${PROTO_ABS_PATH}
            DEPENDS ${PROTO_ABS_PATH} ${GEN_PROTO_DEPENDS}
            COMMENT "Generating C++ protocol buffer code for ${PROTO_NAME}.proto"
            VERBATIM
    )

    # 添加自定义目标，强制生成（不依赖其他目标）
    add_custom_target(
            generate_${PROTO_NAME}_proto ALL  # `ALL` 表示默认构建时执行
            DEPENDS ${GEN_PROTO_SRC} ${GEN_PROTO_HDR}
    )
    
	add_library(${PROTO_NAME}_proto INTERFACE)
	target_sources(${PROTO_NAME}_proto INTERFACE ${GEN_PROTO_SRC} ${GEN_PROTO_HDR})
	target_include_directories(${PROTO_NAME}_proto INTERFACE ${GEN_PROTO_OUTPUT_DIR})
	target_link_libraries(${PROTO_NAME}_proto INTERFACE ${PROTOBUF_LIBRARIES} ${ABSL_LIBS})
    add_dependencies(${PROTO_NAME}_proto generate_${PROTO_NAME}_proto)      # # 添加依赖关系，确保在生成目标之前生成 protobuf 文件

    # 标记生成的文件为自动生成（避免 CMake 报错）
    set_source_files_properties(${GEN_PROTO_SRC} ${GEN_PROTO_HDR} PROPERTIES GENERATED TRUE)
endfunction()