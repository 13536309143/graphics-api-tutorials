cmake_minimum_required(VERSION 3.24)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(dx31_ch_dynamic_offsets LANGUAGES C CXX)
    include(CTest)
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(cmake/Dependencies.cmake)
course_dependencies()
course_asset_dependencies()
add_executable(dx31_ch_dynamic_offsets main.cpp)
target_include_directories(dx31_ch_dynamic_offsets PRIVATE support third_party/stb
                                                           "${COURSE_GLM_INCLUDE}")
target_link_libraries(dx31_ch_dynamic_offsets PRIVATE d3d12 dxgi dxguid d3dcompiler glfw
                                                      assimp::assimp)
target_compile_definitions(
    dx31_ch_dynamic_offsets
    PRIVATE UNICODE _UNICODE NOMINMAX GLFW_INCLUDE_NONE GLM_FORCE_DEPTH_ZERO_TO_ONE
            COURSE_RUNTIME_NAME="dx31_ch_dynamic_offsets.resources")
if(MSVC)
    target_compile_options(dx31_ch_dynamic_offsets PRIVATE /utf-8 /W4 /wd4324 /permissive-)
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(dx31_ch_dynamic_offsets PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                             "${CMAKE_BINARY_DIR}/bin")
endif()
set(resources "$<TARGET_FILE_DIR:dx31_ch_dynamic_offsets>/dx31_ch_dynamic_offsets.resources")
add_custom_target(
    dx31_ch_dynamic_offsets_resources
    COMMAND ${CMAKE_COMMAND} -E make_directory "${resources}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
            "${resources}/shaders")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET dx31_ch_dynamic_offsets_resources
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "${resources}/assets")
endif()
add_dependencies(dx31_ch_dynamic_offsets dx31_ch_dynamic_offsets_resources)
if(BUILD_TESTING)
    add_test(NAME dx31_ch_dynamic_offsets COMMAND $<TARGET_FILE:dx31_ch_dynamic_offsets> --smoke)
    set_tests_properties(dx31_ch_dynamic_offsets PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
endif()
