cmake_minimum_required(VERSION 3.24)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(dx27_indirect_draw LANGUAGES C CXX)
    include(CTest)
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(cmake/Dependencies.cmake)
course_dependencies()
course_asset_dependencies()
add_executable(dx27_indirect_draw main.cpp)
target_include_directories(dx27_indirect_draw PRIVATE support third_party/stb
                                                      "${COURSE_GLM_INCLUDE}")
target_link_libraries(dx27_indirect_draw PRIVATE d3d12 dxgi dxguid d3dcompiler glfw assimp::assimp)
target_compile_definitions(
    dx27_indirect_draw
    PRIVATE UNICODE _UNICODE NOMINMAX GLFW_INCLUDE_NONE GLM_FORCE_DEPTH_ZERO_TO_ONE
            COURSE_RUNTIME_NAME="dx27_indirect_draw.resources")
if(MSVC)
    target_compile_options(dx27_indirect_draw PRIVATE /utf-8 /W4 /wd4324 /permissive-)
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(dx27_indirect_draw PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                        "${CMAKE_BINARY_DIR}/bin")
endif()
set(resources "$<TARGET_FILE_DIR:dx27_indirect_draw>/dx27_indirect_draw.resources")
add_custom_target(
    dx27_indirect_draw_resources
    COMMAND ${CMAKE_COMMAND} -E make_directory "${resources}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
            "${resources}/shaders")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET dx27_indirect_draw_resources
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "${resources}/assets")
endif()
add_dependencies(dx27_indirect_draw dx27_indirect_draw_resources)
if(BUILD_TESTING)
    add_test(NAME dx27_indirect_draw COMMAND $<TARGET_FILE:dx27_indirect_draw> --smoke)
    set_tests_properties(dx27_indirect_draw PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
endif()
