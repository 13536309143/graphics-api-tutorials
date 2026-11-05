cmake_minimum_required(VERSION 3.24)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(34_gallery LANGUAGES C CXX)
    include(CTest)
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(cmake/Dependencies.cmake)
course_dependencies()
course_asset_dependencies()
if(NOT TARGET glad)
    add_library(glad STATIC third_party/glad/src/gl.c)
    target_include_directories(glad PUBLIC third_party/glad/include)
    target_link_libraries(glad PUBLIC ${CMAKE_DL_LIBS})
endif()
add_executable(34_gallery main.cpp support/image.cpp)
target_include_directories(34_gallery PRIVATE support third_party/stb "${COURSE_GLM_INCLUDE}")
target_link_libraries(34_gallery PRIVATE glad glfw assimp::assimp)
target_compile_definitions(34_gallery PRIVATE GLFW_INCLUDE_NONE
                                              COURSE_RUNTIME_NAME="34_gallery.resources")
if(MSVC)
    target_compile_options(34_gallery PRIVATE /utf-8 /W4 /wd4324 /permissive-)
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(34_gallery PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()
set(resources "$<TARGET_FILE_DIR:34_gallery>/34_gallery.resources")
add_custom_target(
    34_gallery_resources
    COMMAND ${CMAKE_COMMAND} -E make_directory "${resources}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
            "${resources}/shaders")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET 34_gallery_resources
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "${resources}/assets")
endif()
add_dependencies(34_gallery 34_gallery_resources)
if(BUILD_TESTING)
    add_test(NAME 34_gallery COMMAND $<TARGET_FILE:34_gallery> --smoke)
    set_tests_properties(34_gallery PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
endif()
