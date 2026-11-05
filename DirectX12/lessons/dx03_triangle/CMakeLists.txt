cmake_minimum_required(VERSION 3.24)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(dx03_triangle LANGUAGES C CXX)
    include(CTest)
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(cmake/Dependencies.cmake)
course_dependencies()
add_executable(dx03_triangle main.cpp)
target_include_directories(dx03_triangle PRIVATE support third_party/stb "${COURSE_GLM_INCLUDE}")
target_link_libraries(dx03_triangle PRIVATE d3d12 dxgi dxguid d3dcompiler glfw)
target_compile_definitions(
    dx03_triangle PRIVATE UNICODE _UNICODE NOMINMAX GLFW_INCLUDE_NONE GLM_FORCE_DEPTH_ZERO_TO_ONE
                          COURSE_RUNTIME_NAME="dx03_triangle.resources")
if(MSVC)
    target_compile_options(dx03_triangle PRIVATE /utf-8 /W4 /wd4324 /permissive-)
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(dx03_triangle PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                   "${CMAKE_BINARY_DIR}/bin")
endif()
set(resources "$<TARGET_FILE_DIR:dx03_triangle>/dx03_triangle.resources")
add_custom_target(
    dx03_triangle_resources
    COMMAND ${CMAKE_COMMAND} -E make_directory "${resources}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
            "${resources}/shaders")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET dx03_triangle_resources
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "${resources}/assets")
endif()
add_dependencies(dx03_triangle dx03_triangle_resources)
if(BUILD_TESTING)
    add_test(NAME dx03_triangle COMMAND $<TARGET_FILE:dx03_triangle> --smoke)
    set_tests_properties(dx03_triangle PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
endif()
