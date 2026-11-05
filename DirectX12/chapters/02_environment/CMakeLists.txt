cmake_minimum_required(VERSION 3.24)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(dx02_ch_environment LANGUAGES C CXX)
    include(CTest)
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(cmake/Dependencies.cmake)
course_dependencies()
course_asset_dependencies()
add_executable(dx02_ch_environment main.cpp)
target_include_directories(dx02_ch_environment PRIVATE support third_party/stb
                                                       "${COURSE_GLM_INCLUDE}")
target_link_libraries(dx02_ch_environment PRIVATE d3d12 dxgi dxguid d3dcompiler glfw assimp::assimp)
target_compile_definitions(
    dx02_ch_environment
    PRIVATE UNICODE _UNICODE NOMINMAX GLFW_INCLUDE_NONE GLM_FORCE_DEPTH_ZERO_TO_ONE
            COURSE_RUNTIME_NAME="dx02_ch_environment.resources")
if(MSVC)
    target_compile_options(dx02_ch_environment PRIVATE /utf-8 /W4 /wd4324 /permissive-)
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(dx02_ch_environment PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                         "${CMAKE_BINARY_DIR}/bin")
endif()
set(resources "$<TARGET_FILE_DIR:dx02_ch_environment>/dx02_ch_environment.resources")
add_custom_target(
    dx02_ch_environment_resources
    COMMAND ${CMAKE_COMMAND} -E make_directory "${resources}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
            "${resources}/shaders")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET dx02_ch_environment_resources
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "${resources}/assets")
endif()
add_dependencies(dx02_ch_environment dx02_ch_environment_resources)
if(BUILD_TESTING)
    add_test(NAME dx02_ch_environment COMMAND $<TARGET_FILE:dx02_ch_environment> --smoke)
    set_tests_properties(dx02_ch_environment PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
endif()
