cmake_minimum_required(VERSION 3.24)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(06_transform LANGUAGES C CXX)
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
add_executable(06_transform main.cpp support/image.cpp)
target_include_directories(06_transform PRIVATE support third_party/stb "${COURSE_GLM_INCLUDE}")
target_link_libraries(06_transform PRIVATE glad glfw)
target_compile_definitions(06_transform PRIVATE GLFW_INCLUDE_NONE
                                                COURSE_RUNTIME_NAME="06_transform.resources")
if(MSVC)
    target_compile_options(06_transform PRIVATE /utf-8 /W4 /wd4324 /permissive-)
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(06_transform PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                  "${CMAKE_BINARY_DIR}/bin")
endif()
set(resources "$<TARGET_FILE_DIR:06_transform>/06_transform.resources")
add_custom_target(
    06_transform_resources
    COMMAND ${CMAKE_COMMAND} -E make_directory "${resources}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
            "${resources}/shaders")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET 06_transform_resources
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "${resources}/assets")
endif()
add_dependencies(06_transform 06_transform_resources)
if(BUILD_TESTING)
    add_test(NAME 06_transform COMMAND $<TARGET_FILE:06_transform> --smoke)
    set_tests_properties(06_transform PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
endif()
