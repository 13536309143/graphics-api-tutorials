cmake_minimum_required(VERSION 3.24)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(dx46_ch_compute LANGUAGES C CXX)
    include(CTest)
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(cmake/Dependencies.cmake)
course_dependencies()
add_executable(dx46_ch_compute main.cpp)
target_include_directories(dx46_ch_compute PRIVATE support third_party/stb "${COURSE_GLM_INCLUDE}")
target_link_libraries(dx46_ch_compute PRIVATE d3d12 dxgi dxguid d3dcompiler glfw)
target_compile_definitions(
    dx46_ch_compute PRIVATE UNICODE _UNICODE NOMINMAX GLFW_INCLUDE_NONE GLM_FORCE_DEPTH_ZERO_TO_ONE
                            COURSE_RUNTIME_NAME="dx46_ch_compute.resources")
if(MSVC)
    target_compile_options(dx46_ch_compute PRIVATE /utf-8 /W4 /wd4324 /permissive-)
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(dx46_ch_compute PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                     "${CMAKE_BINARY_DIR}/bin")
endif()
set(resources "$<TARGET_FILE_DIR:dx46_ch_compute>/dx46_ch_compute.resources")
add_custom_target(
    dx46_ch_compute_resources
    COMMAND ${CMAKE_COMMAND} -E make_directory "${resources}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
            "${resources}/shaders")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET dx46_ch_compute_resources
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "${resources}/assets")
endif()
add_dependencies(dx46_ch_compute dx46_ch_compute_resources)
if(BUILD_TESTING)
    add_test(NAME dx46_ch_compute COMMAND $<TARGET_FILE:dx46_ch_compute> --smoke)
    set_tests_properties(dx46_ch_compute PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
endif()
