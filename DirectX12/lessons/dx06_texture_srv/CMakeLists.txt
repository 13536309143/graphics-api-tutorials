cmake_minimum_required(VERSION 3.24)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(dx06_texture_srv LANGUAGES C CXX)
    include(CTest)
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(cmake/Dependencies.cmake)
course_dependencies()
add_executable(dx06_texture_srv main.cpp)
target_include_directories(dx06_texture_srv PRIVATE support third_party/stb "${COURSE_GLM_INCLUDE}")
target_link_libraries(dx06_texture_srv PRIVATE d3d12 dxgi dxguid d3dcompiler glfw)
target_compile_definitions(
    dx06_texture_srv
    PRIVATE UNICODE _UNICODE NOMINMAX GLFW_INCLUDE_NONE GLM_FORCE_DEPTH_ZERO_TO_ONE
            COURSE_RUNTIME_NAME="dx06_texture_srv.resources")
if(MSVC)
    target_compile_options(dx06_texture_srv PRIVATE /utf-8 /W4 /wd4324 /permissive-)
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(dx06_texture_srv PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                      "${CMAKE_BINARY_DIR}/bin")
endif()
set(resources "$<TARGET_FILE_DIR:dx06_texture_srv>/dx06_texture_srv.resources")
add_custom_target(
    dx06_texture_srv_resources
    COMMAND ${CMAKE_COMMAND} -E make_directory "${resources}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
            "${resources}/shaders")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET dx06_texture_srv_resources
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "${resources}/assets")
endif()
add_dependencies(dx06_texture_srv dx06_texture_srv_resources)
if(BUILD_TESTING)
    add_test(NAME dx06_texture_srv COMMAND $<TARGET_FILE:dx06_texture_srv> --smoke)
    set_tests_properties(dx06_texture_srv PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
endif()
