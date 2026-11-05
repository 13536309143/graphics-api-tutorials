cmake_minimum_required(VERSION 3.24)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(nv06_pbr_viewport LANGUAGES C CXX)
    include(CTest)
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(cmake/Dependencies.cmake)
include(cmake/Nvpro.cmake)
find_package(Vulkan REQUIRED COMPONENTS glslc)
add_executable(nv06_pbr_viewport main.cpp)
target_include_directories(nv06_pbr_viewport PRIVATE support third_party/stb
                                                     "${COURSE_GLM_INCLUDE}")
target_link_libraries(nv06_pbr_viewport PRIVATE nvpro2::nvapp)
target_compile_definitions(
    nv06_pbr_viewport PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE
                              COURSE_RUNTIME_NAME="nv06_pbr_viewport.resources")
if(MSVC)
    target_compile_options(nv06_pbr_viewport PRIVATE /utf-8 /W4 /wd4324 /permissive-)
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(nv06_pbr_viewport PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                       "${CMAKE_BINARY_DIR}/bin")
endif()
set(resources "$<TARGET_FILE_DIR:nv06_pbr_viewport>/nv06_pbr_viewport.resources")
file(GLOB stage_files CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert"
     "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag" "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.comp")
set(stage_outputs)
foreach(stage IN LISTS stage_files)
    get_filename_component(stage_name "${stage}" NAME)
    set(output "${CMAKE_CURRENT_BINARY_DIR}/spv/${stage_name}.spv")
    add_custom_command(
        OUTPUT "${output}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/spv"
        COMMAND Vulkan::glslc --target-env=vulkan1.3 "${stage}" -o "${output}"
        DEPENDS "${stage}"
        VERBATIM)
    list(APPEND stage_outputs "${output}")
endforeach()
add_custom_target(nv06_pbr_viewport_stages DEPENDS ${stage_outputs})
add_custom_target(
    nv06_pbr_viewport_resources
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/spv"
            "${resources}/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_BINARY_DIR}/spv"
            "${resources}/shaders")
add_dependencies(nv06_pbr_viewport_resources nv06_pbr_viewport_stages)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET nv06_pbr_viewport_resources
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "${resources}/assets")
endif()
add_dependencies(nv06_pbr_viewport nv06_pbr_viewport_resources)
if(BUILD_TESTING)
    add_test(NAME nv06_pbr_viewport COMMAND $<TARGET_FILE:nv06_pbr_viewport> --smoke)
    set_tests_properties(nv06_pbr_viewport PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
endif()
