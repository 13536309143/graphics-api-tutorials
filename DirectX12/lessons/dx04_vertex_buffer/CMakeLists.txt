cmake_minimum_required(VERSION 3.24)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(dx04_vertex_buffer LANGUAGES C CXX)
    include(CTest)
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(cmake/Dependencies.cmake)
course_dependencies()
add_executable(dx04_vertex_buffer main.cpp)
target_include_directories(dx04_vertex_buffer PRIVATE support third_party/stb
                                                      "${COURSE_GLM_INCLUDE}")
target_link_libraries(dx04_vertex_buffer PRIVATE d3d12 dxgi dxguid d3dcompiler glfw)
target_compile_definitions(
    dx04_vertex_buffer
    PRIVATE UNICODE _UNICODE NOMINMAX GLFW_INCLUDE_NONE GLM_FORCE_DEPTH_ZERO_TO_ONE
            COURSE_RUNTIME_NAME="dx04_vertex_buffer.resources")
if(MSVC)
    target_compile_options(dx04_vertex_buffer PRIVATE /utf-8 /W4 /wd4324 /permissive-)
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(dx04_vertex_buffer PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                        "${CMAKE_BINARY_DIR}/bin")
endif()
set(resources "$<TARGET_FILE_DIR:dx04_vertex_buffer>/dx04_vertex_buffer.resources")
add_custom_target(
    dx04_vertex_buffer_resources
    COMMAND ${CMAKE_COMMAND} -E make_directory "${resources}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
            "${resources}/shaders")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET dx04_vertex_buffer_resources
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "${resources}/assets")
endif()
add_dependencies(dx04_vertex_buffer dx04_vertex_buffer_resources)
if(BUILD_TESTING)
    add_test(NAME dx04_vertex_buffer COMMAND $<TARGET_FILE:dx04_vertex_buffer> --smoke)
    set_tests_properties(dx04_vertex_buffer PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
endif()
