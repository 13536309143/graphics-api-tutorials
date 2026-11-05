cmake_minimum_required(VERSION 3.24)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(dx26_ch_staging LANGUAGES C CXX)
    include(CTest)
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(cmake/Dependencies.cmake)
course_dependencies()
course_asset_dependencies()
add_executable(dx26_ch_staging main.cpp)
target_include_directories(dx26_ch_staging PRIVATE support third_party/stb "${COURSE_GLM_INCLUDE}")
target_link_libraries(dx26_ch_staging PRIVATE d3d12 dxgi dxguid d3dcompiler glfw assimp::assimp)
target_compile_definitions(
    dx26_ch_staging PRIVATE UNICODE _UNICODE NOMINMAX GLFW_INCLUDE_NONE GLM_FORCE_DEPTH_ZERO_TO_ONE
                            COURSE_RUNTIME_NAME="dx26_ch_staging.resources")
if(MSVC)
    target_compile_options(dx26_ch_staging PRIVATE /utf-8 /W4 /wd4324 /permissive-)
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(dx26_ch_staging PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                     "${CMAKE_BINARY_DIR}/bin")
endif()
set(resources "$<TARGET_FILE_DIR:dx26_ch_staging>/dx26_ch_staging.resources")
add_custom_target(
    dx26_ch_staging_resources
    COMMAND ${CMAKE_COMMAND} -E make_directory "${resources}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
            "${resources}/shaders")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET dx26_ch_staging_resources
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "${resources}/assets")
endif()
add_dependencies(dx26_ch_staging dx26_ch_staging_resources)
if(BUILD_TESTING)
    add_test(NAME dx26_ch_staging COMMAND $<TARGET_FILE:dx26_ch_staging> --smoke)
    set_tests_properties(dx26_ch_staging PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
endif()
