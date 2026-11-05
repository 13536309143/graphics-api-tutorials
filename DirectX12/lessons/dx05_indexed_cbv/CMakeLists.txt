cmake_minimum_required(VERSION 3.24)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(dx05_indexed_cbv LANGUAGES C CXX)
    include(CTest)
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(cmake/Dependencies.cmake)
course_dependencies()
add_executable(dx05_indexed_cbv main.cpp)
target_include_directories(dx05_indexed_cbv PRIVATE support third_party/stb "${COURSE_GLM_INCLUDE}")
target_link_libraries(dx05_indexed_cbv PRIVATE d3d12 dxgi dxguid d3dcompiler glfw)
target_compile_definitions(
    dx05_indexed_cbv
    PRIVATE UNICODE _UNICODE NOMINMAX GLFW_INCLUDE_NONE GLM_FORCE_DEPTH_ZERO_TO_ONE
            COURSE_RUNTIME_NAME="dx05_indexed_cbv.resources")
if(MSVC)
    target_compile_options(dx05_indexed_cbv PRIVATE /utf-8 /W4 /wd4324 /permissive-)
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(dx05_indexed_cbv PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                      "${CMAKE_BINARY_DIR}/bin")
endif()
set(resources "$<TARGET_FILE_DIR:dx05_indexed_cbv>/dx05_indexed_cbv.resources")
add_custom_target(
    dx05_indexed_cbv_resources
    COMMAND ${CMAKE_COMMAND} -E make_directory "${resources}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
            "${resources}/shaders")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET dx05_indexed_cbv_resources
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "${resources}/assets")
endif()
add_dependencies(dx05_indexed_cbv dx05_indexed_cbv_resources)
if(BUILD_TESTING)
    add_test(NAME dx05_indexed_cbv COMMAND $<TARGET_FILE:dx05_indexed_cbv> --smoke)
    set_tests_properties(dx05_indexed_cbv PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
endif()
