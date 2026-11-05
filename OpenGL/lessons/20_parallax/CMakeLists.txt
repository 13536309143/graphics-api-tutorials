cmake_minimum_required(VERSION 3.24)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(20_parallax LANGUAGES C CXX)
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
add_executable(20_parallax main.cpp support/image.cpp)
target_include_directories(20_parallax PRIVATE support third_party/stb "${COURSE_GLM_INCLUDE}")
target_link_libraries(20_parallax PRIVATE glad glfw)
target_compile_definitions(20_parallax PRIVATE GLFW_INCLUDE_NONE
                                               COURSE_RUNTIME_NAME="20_parallax.resources")
if(MSVC)
    target_compile_options(20_parallax PRIVATE /utf-8 /W4 /wd4324 /permissive-)
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(20_parallax PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()
set(resources "$<TARGET_FILE_DIR:20_parallax>/20_parallax.resources")
add_custom_target(
    20_parallax_resources
    COMMAND ${CMAKE_COMMAND} -E make_directory "${resources}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
            "${resources}/shaders")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET 20_parallax_resources
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "${resources}/assets")
endif()
add_dependencies(20_parallax 20_parallax_resources)
if(BUILD_TESTING)
    add_test(NAME 20_parallax COMMAND $<TARGET_FILE:20_parallax> --smoke)
    set_tests_properties(20_parallax PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
endif()
