cmake_minimum_required(VERSION 3.24)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(dx02_swapchain LANGUAGES C CXX)
    include(CTest)
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(cmake/Dependencies.cmake)
course_dependencies()
add_executable(dx02_swapchain main.cpp)
target_include_directories(dx02_swapchain PRIVATE support third_party/stb "${COURSE_GLM_INCLUDE}")
target_link_libraries(dx02_swapchain PRIVATE d3d12 dxgi dxguid d3dcompiler glfw)
target_compile_definitions(
    dx02_swapchain PRIVATE UNICODE _UNICODE NOMINMAX GLFW_INCLUDE_NONE GLM_FORCE_DEPTH_ZERO_TO_ONE
                           COURSE_RUNTIME_NAME="dx02_swapchain.resources")
if(MSVC)
    target_compile_options(dx02_swapchain PRIVATE /utf-8 /W4 /wd4324 /permissive-)
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(dx02_swapchain PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                    "${CMAKE_BINARY_DIR}/bin")
endif()
set(resources "$<TARGET_FILE_DIR:dx02_swapchain>/dx02_swapchain.resources")
add_custom_target(
    dx02_swapchain_resources
    COMMAND ${CMAKE_COMMAND} -E make_directory "${resources}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
            "${resources}/shaders")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET dx02_swapchain_resources
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "${resources}/assets")
endif()
add_dependencies(dx02_swapchain dx02_swapchain_resources)
if(BUILD_TESTING)
    add_test(NAME dx02_swapchain COMMAND $<TARGET_FILE:dx02_swapchain> --smoke)
    set_tests_properties(dx02_swapchain PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
endif()
