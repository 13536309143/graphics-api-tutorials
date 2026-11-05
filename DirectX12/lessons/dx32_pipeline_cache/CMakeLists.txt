cmake_minimum_required(VERSION 3.24)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(dx32_pipeline_cache LANGUAGES C CXX)
    include(CTest)
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(cmake/Dependencies.cmake)
course_dependencies()
course_asset_dependencies()
add_executable(dx32_pipeline_cache main.cpp)
target_include_directories(dx32_pipeline_cache PRIVATE support third_party/stb
                                                       "${COURSE_GLM_INCLUDE}")
target_link_libraries(dx32_pipeline_cache PRIVATE d3d12 dxgi dxguid d3dcompiler glfw assimp::assimp)
target_compile_definitions(
    dx32_pipeline_cache
    PRIVATE UNICODE _UNICODE NOMINMAX GLFW_INCLUDE_NONE GLM_FORCE_DEPTH_ZERO_TO_ONE
            COURSE_RUNTIME_NAME="dx32_pipeline_cache.resources")
if(MSVC)
    target_compile_options(dx32_pipeline_cache PRIVATE /utf-8 /W4 /wd4324 /permissive-)
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(dx32_pipeline_cache PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                         "${CMAKE_BINARY_DIR}/bin")
endif()
set(resources "$<TARGET_FILE_DIR:dx32_pipeline_cache>/dx32_pipeline_cache.resources")
add_custom_target(
    dx32_pipeline_cache_resources
    COMMAND ${CMAKE_COMMAND} -E make_directory "${resources}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
            "${resources}/shaders")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET dx32_pipeline_cache_resources
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "${resources}/assets")
endif()
add_dependencies(dx32_pipeline_cache dx32_pipeline_cache_resources)
if(BUILD_TESTING)
    add_test(NAME dx32_pipeline_cache COMMAND $<TARGET_FILE:dx32_pipeline_cache> --smoke)
    set_tests_properties(dx32_pipeline_cache PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
endif()
