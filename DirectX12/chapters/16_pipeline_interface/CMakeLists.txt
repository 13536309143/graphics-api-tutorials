cmake_minimum_required(VERSION 3.24)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(dx16_ch_pipeline_interface LANGUAGES C CXX)
    include(CTest)
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(cmake/Dependencies.cmake)
course_dependencies()
course_asset_dependencies()
add_executable(dx16_ch_pipeline_interface main.cpp)
target_include_directories(dx16_ch_pipeline_interface PRIVATE support third_party/stb
                                                              "${COURSE_GLM_INCLUDE}")
target_link_libraries(dx16_ch_pipeline_interface PRIVATE d3d12 dxgi dxguid d3dcompiler glfw
                                                         assimp::assimp)
target_compile_definitions(
    dx16_ch_pipeline_interface
    PRIVATE UNICODE _UNICODE NOMINMAX GLFW_INCLUDE_NONE GLM_FORCE_DEPTH_ZERO_TO_ONE
            COURSE_RUNTIME_NAME="dx16_ch_pipeline_interface.resources")
if(MSVC)
    target_compile_options(dx16_ch_pipeline_interface PRIVATE /utf-8 /W4 /wd4324 /permissive-)
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(dx16_ch_pipeline_interface PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                                "${CMAKE_BINARY_DIR}/bin")
endif()
set(resources "$<TARGET_FILE_DIR:dx16_ch_pipeline_interface>/dx16_ch_pipeline_interface.resources")
add_custom_target(
    dx16_ch_pipeline_interface_resources
    COMMAND ${CMAKE_COMMAND} -E make_directory "${resources}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
            "${resources}/shaders")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET dx16_ch_pipeline_interface_resources
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "${resources}/assets")
endif()
add_dependencies(dx16_ch_pipeline_interface dx16_ch_pipeline_interface_resources)
if(BUILD_TESTING)
    add_test(NAME dx16_ch_pipeline_interface COMMAND $<TARGET_FILE:dx16_ch_pipeline_interface>
                                                     --smoke)
    set_tests_properties(dx16_ch_pipeline_interface PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
endif()
