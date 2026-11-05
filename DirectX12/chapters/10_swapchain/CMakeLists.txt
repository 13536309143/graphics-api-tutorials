cmake_minimum_required(VERSION 3.24)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(dx10_ch_swapchain LANGUAGES C CXX)
    include(CTest)
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(cmake/Dependencies.cmake)
course_dependencies()
add_executable(dx10_ch_swapchain main.cpp)
target_include_directories(dx10_ch_swapchain PRIVATE support third_party/stb
                                                     "${COURSE_GLM_INCLUDE}")
target_link_libraries(dx10_ch_swapchain PRIVATE d3d12 dxgi dxguid d3dcompiler glfw)
target_compile_definitions(
    dx10_ch_swapchain
    PRIVATE UNICODE _UNICODE NOMINMAX GLFW_INCLUDE_NONE GLM_FORCE_DEPTH_ZERO_TO_ONE
            COURSE_RUNTIME_NAME="dx10_ch_swapchain.resources")
if(MSVC)
    target_compile_options(dx10_ch_swapchain PRIVATE /utf-8 /W4 /wd4324 /permissive-)
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(dx10_ch_swapchain PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                       "${CMAKE_BINARY_DIR}/bin")
endif()
set(resources "$<TARGET_FILE_DIR:dx10_ch_swapchain>/dx10_ch_swapchain.resources")
add_custom_target(
    dx10_ch_swapchain_resources
    COMMAND ${CMAKE_COMMAND} -E make_directory "${resources}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
            "${resources}/shaders")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET dx10_ch_swapchain_resources
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "${resources}/assets")
endif()
add_dependencies(dx10_ch_swapchain dx10_ch_swapchain_resources)
if(BUILD_TESTING)
    add_test(NAME dx10_ch_swapchain COMMAND $<TARGET_FILE:dx10_ch_swapchain> --smoke)
    set_tests_properties(dx10_ch_swapchain PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
endif()
