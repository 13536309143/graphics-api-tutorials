cmake_minimum_required(VERSION 3.24)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(dx35_srgb_view LANGUAGES C CXX)
    include(CTest)
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(cmake/Dependencies.cmake)
course_dependencies()
course_asset_dependencies()
add_executable(dx35_srgb_view main.cpp)
target_include_directories(dx35_srgb_view PRIVATE support third_party/stb "${COURSE_GLM_INCLUDE}")
target_link_libraries(dx35_srgb_view PRIVATE d3d12 dxgi dxguid d3dcompiler glfw assimp::assimp)
target_compile_definitions(
    dx35_srgb_view PRIVATE UNICODE _UNICODE NOMINMAX GLFW_INCLUDE_NONE GLM_FORCE_DEPTH_ZERO_TO_ONE
                           COURSE_RUNTIME_NAME="dx35_srgb_view.resources")
if(MSVC)
    target_compile_options(dx35_srgb_view PRIVATE /utf-8 /W4 /wd4324 /permissive-)
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(dx35_srgb_view PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                    "${CMAKE_BINARY_DIR}/bin")
endif()
set(resources "$<TARGET_FILE_DIR:dx35_srgb_view>/dx35_srgb_view.resources")
add_custom_target(
    dx35_srgb_view_resources
    COMMAND ${CMAKE_COMMAND} -E make_directory "${resources}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
            "${resources}/shaders")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET dx35_srgb_view_resources
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "${resources}/assets")
endif()
add_dependencies(dx35_srgb_view dx35_srgb_view_resources)
if(BUILD_TESTING)
    add_test(NAME dx35_srgb_view COMMAND $<TARGET_FILE:dx35_srgb_view> --smoke)
    set_tests_properties(dx35_srgb_view PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
endif()
