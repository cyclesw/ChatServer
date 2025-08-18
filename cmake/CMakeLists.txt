function(generate_proto)
    set(oneValueArgs FILE OUTPUT_DIR)
    set(multiValueArgs DEPENDS)
    cmake_parse_arguments(GEN_PROTO "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # 获取文件名（不带路径和扩展名）
    get_filename_component(PROTO_NAME ${GEN_PROTO_FILE} NAME_WE)
    get_filename_component(PROTO_ABS_PATH ${GEN_PROTO_FILE} ABSOLUTE)
    if (GEN_PROTO_DEPENDS)
        get_filename_component(PROTO_DEP ${GEN_PROTO_DEPENDS} NAME_WE)
        set(PROTO_DEP ${PROTO_DEP}_proto)
    endif ()


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

    add_custom_target(
            generate_${PROTO_NAME}_proto
            DEPENDS ${GEN_PROTO_SRC} ${GEN_PROTO_HDR}
    )
	add_library(${PROTO_NAME}_proto INTERFACE)
	target_sources(${PROTO_NAME}_proto INTERFACE ${GEN_PROTO_SRC} ${GEN_PROTO_HDR})
	target_include_directories(${PROTO_NAME}_proto INTERFACE ${GEN_PROTO_OUTPUT_DIR})
	target_link_libraries(${PROTO_NAME}_proto INTERFACE ${PROTOBUF_LIBRARIES} ${ABSL_LIBS} ${PROTO_DEP})
    add_dependencies(${PROTO_NAME}_proto generate_${PROTO_NAME}_proto)      # # 添加依赖关系，确保在生成目标之前生成 protobuf 文件

    # 标记生成的文件为自动生成（避免 CMake 报错）
    set_source_files_properties(${GEN_PROTO_SRC} ${GEN_PROTO_HDR} PROPERTIES GENERATED TRUE)
endfunction()

# generate_odb_files.cmake
#
# generate_odb_files(
#     INPUT       <header>
#     OUTPUT_DIR  <dir>
#     DATABASE    <mysql|pgsql|sqlite|...>
#     STD         <11|14|17|20>
#     [PROFILE    <profile>...]
#     [EXTRA_OPTS <extra odb args>...]
# )
#
# 生成一个名为 odb_<basename> 的 INTERFACE 库，使用者只要
# target_link_libraries(<your_target> PRIVATE odb_<basename>)
# 就能拿到所有生成的 .cxx/.hxx 以及 include 路径和链接库。
function(generate_odb_files)
    set(oneValueArgs INPUT OUTPUT_DIR DATABASE STD)
    set(multiValueArgs PROFILE EXTRA_OPTS)
    cmake_parse_arguments(O "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # 基本检查
    if(NOT O_INPUT OR NOT O_OUTPUT_DIR OR NOT O_DATABASE)
        message(FATAL_ERROR "INPUT, OUTPUT_DIR and DATABASE are mandatory")
    endif()
    if(NOT O_STD)
        set(O_STD 17)
    endif()

    # 获取文件名（不含路径、不含扩展名）
    get_filename_component(name_we  "${O_INPUT}" NAME_WE)
    get_filename_component(abs_in   "${O_INPUT}" ABSOLUTE)
    get_filename_component(pre_dir  "${O_INPUT}" DIRECTORY)    # fix up head file can't found

    # 生成的文件列表
    set(out_cxx "${O_OUTPUT_DIR}/${name_we}-odb.cxx")
    set(out_sql "${O_OUTPUT_DIR}/${name_we}.sql")
    set(out_odb "${O_OUTPUT_DIR}/${name_we}-odb.hxx")

    # 构造 odb 命令行参数
    set(odb_args
            -d ${O_DATABASE}
            --std c++${O_STD}
            --generate-query
            --generate-schema
            --output-dir ${O_OUTPUT_DIR}
    )
    foreach(p ${O_PROFILE})
        list(APPEND odb_args --profile ${p})
    endforeach()
    list(APPEND odb_args ${O_EXTRA_OPTS})

    # 确保输出目录存在
    file(MAKE_DIRECTORY ${O_OUTPUT_DIR})


    # 自定义命令：调用 odb 生成源码
    add_custom_command(
            OUTPUT  ${out_cxx} ${out_sql} ${out_odb}
            COMMAND odb
            ARGS    ${odb_args} ${abs_in}
            DEPENDS ${abs_in}
            COMMENT "[ODB] Generating C++ persistence code for ${name_we}.hxx"
            VERBATIM
    )

    # 自定义目标：让生成规则可被依赖
    add_custom_target(odb_${name_we}_code
            DEPENDS ${out_cxx} ${out_sql} ${out_odb}
    )

    # 提供一个接口库，把生成目录塞进 include path
    add_library(odb_${name_we} INTERFACE)
    target_include_directories(odb_${name_we} INTERFACE ${O_OUTPUT_DIR} ${pre_dir})
    target_sources(odb_${name_we} INTERFACE ${out_cxx} ${out_odb} )
    add_dependencies(odb_${name_we} odb_${name_we}_code)

    # 通常还需要 ODB 运行时库；这里把责任推给调用者
    # target_link_libraries(odb_${name_we} INTERFACE odb::odb_mysql)
endfunction()