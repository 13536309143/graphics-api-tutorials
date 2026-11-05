cmake_minimum_required(VERSION 3.24)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(04_uniform LANGUAGES C CXX)
    include(CTest)
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(cmake/Dependencies.cmake)
course_dependencies()
if(NOT TARGET glad)
    add_library(glad STATIC third_party/glad/src/gl.c)
    target_include_directories(glad PUBLIC third_party/glad/include)
    target_link_libraries(glad PUBLIC ${CMAKE_DL_LIBS})
endif()
add_executable(04_uniform main.cpp support/image.cpp)
target_include_directories(04_uniform PRIVATE support third_party/stb "${COURSE_GLM_INCLUDE}")
target_link_libraries(04_uniform PRIVATE glad glfw)
target_compile_definitions(04_uniform PRIVATE GLFW_INCLUDE_NONE
                                              COURSE_RUNTIME_NAME="04_uniform.resources")
if(MSVC)
    target_compile_options(04_uniform PRIVATE /utf-8 /W4 /wd4324 /permissive-)
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(04_uniform PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()
set(resources "$<TARGET_FILE_DIR:04_uniform>/04_uniform.resources")
add_custom_target(
    04_uniform_resources
    COMMAND ${CMAKE_COMMAND} -E make_directory "${resources}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
            "${resources}/shaders")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET 04_uniform_resources
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "${resources}/assets")
endif()
add_dependencies(04_uniform 04_uniform_resources)
if(BUILD_TESTING)
    add_test(NAME 04_uniform COMMAND $<TARGET_FILE:04_uniform> --smoke)
    set_tests_properties(04_uniform PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
endif()
