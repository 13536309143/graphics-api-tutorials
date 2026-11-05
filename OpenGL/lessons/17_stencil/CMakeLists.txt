cmake_minimum_required(VERSION 3.24)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(17_stencil LANGUAGES C CXX)
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
add_executable(17_stencil main.cpp support/image.cpp)
target_include_directories(17_stencil PRIVATE support third_party/stb "${COURSE_GLM_INCLUDE}")
target_link_libraries(17_stencil PRIVATE glad glfw)
target_compile_definitions(17_stencil PRIVATE GLFW_INCLUDE_NONE
                                              COURSE_RUNTIME_NAME="17_stencil.resources")
if(MSVC)
    target_compile_options(17_stencil PRIVATE /utf-8 /W4 /wd4324 /permissive-)
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(17_stencil PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()
set(resources "$<TARGET_FILE_DIR:17_stencil>/17_stencil.resources")
add_custom_target(
    17_stencil_resources
    COMMAND ${CMAKE_COMMAND} -E make_directory "${resources}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
            "${resources}/shaders")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET 17_stencil_resources
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "${resources}/assets")
endif()
add_dependencies(17_stencil 17_stencil_resources)
if(BUILD_TESTING)
    add_test(NAME 17_stencil COMMAND $<TARGET_FILE:17_stencil> --smoke)
    set_tests_properties(17_stencil PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
endif()
