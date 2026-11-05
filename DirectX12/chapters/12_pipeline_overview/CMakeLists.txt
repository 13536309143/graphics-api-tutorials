cmake_minimum_required(VERSION 3.24)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(dx12_ch_pipeline_overview LANGUAGES C CXX)
    include(CTest)
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(cmake/Dependencies.cmake)
course_dependencies()
add_executable(dx12_ch_pipeline_overview main.cpp)
target_include_directories(dx12_ch_pipeline_overview PRIVATE support third_party/stb
                                                             "${COURSE_GLM_INCLUDE}")
target_link_libraries(dx12_ch_pipeline_overview PRIVATE d3d12 dxgi dxguid d3dcompiler glfw)
target_compile_definitions(
    dx12_ch_pipeline_overview
    PRIVATE UNICODE _UNICODE NOMINMAX GLFW_INCLUDE_NONE GLM_FORCE_DEPTH_ZERO_TO_ONE
            COURSE_RUNTIME_NAME="dx12_ch_pipeline_overview.resources")
if(MSVC)
    target_compile_options(dx12_ch_pipeline_overview PRIVATE /utf-8 /W4 /wd4324 /permissive-)
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(dx12_ch_pipeline_overview PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                               "${CMAKE_BINARY_DIR}/bin")
endif()
set(resources "$<TARGET_FILE_DIR:dx12_ch_pipeline_overview>/dx12_ch_pipeline_overview.resources")
add_custom_target(
    dx12_ch_pipeline_overview_resources
    COMMAND ${CMAKE_COMMAND} -E make_directory "${resources}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
            "${resources}/shaders")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET dx12_ch_pipeline_overview_resources
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "${resources}/assets")
endif()
add_dependencies(dx12_ch_pipeline_overview dx12_ch_pipeline_overview_resources)
if(BUILD_TESTING)
    add_test(NAME dx12_ch_pipeline_overview COMMAND $<TARGET_FILE:dx12_ch_pipeline_overview>
                                                    --smoke)
    set_tests_properties(dx12_ch_pipeline_overview PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
endif()
