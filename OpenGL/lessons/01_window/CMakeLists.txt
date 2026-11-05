cmake_minimum_required(VERSION 3.24)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(01_window LANGUAGES C CXX)
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
add_executable(01_window main.cpp support/image.cpp)
target_include_directories(01_window PRIVATE support third_party/stb "${COURSE_GLM_INCLUDE}")
target_link_libraries(01_window PRIVATE glad glfw)
target_compile_definitions(01_window PRIVATE GLFW_INCLUDE_NONE
                                             COURSE_RUNTIME_NAME="01_window.resources")
if(MSVC)
    target_compile_options(01_window PRIVATE /utf-8 /W4 /wd4324 /permissive-)
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(01_window PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()
set(resources "$<TARGET_FILE_DIR:01_window>/01_window.resources")
add_custom_target(
    01_window_resources
    COMMAND ${CMAKE_COMMAND} -E make_directory "${resources}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
            "${resources}/shaders")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET 01_window_resources
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "${resources}/assets")
endif()
add_dependencies(01_window 01_window_resources)
if(BUILD_TESTING)
    add_test(NAME 01_window COMMAND $<TARGET_FILE:01_window> --smoke)
    set_tests_properties(01_window PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
endif()
