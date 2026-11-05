cmake_minimum_required(VERSION 3.24)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(15_model_gltf LANGUAGES C CXX)
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
add_executable(15_model_gltf main.cpp support/image.cpp)
target_include_directories(15_model_gltf PRIVATE support third_party/stb "${COURSE_GLM_INCLUDE}")
target_link_libraries(15_model_gltf PRIVATE glad glfw assimp::assimp)
target_compile_definitions(15_model_gltf PRIVATE GLFW_INCLUDE_NONE
                                                 COURSE_RUNTIME_NAME="15_model_gltf.resources")
if(MSVC)
    target_compile_options(15_model_gltf PRIVATE /utf-8 /W4 /wd4324 /permissive-)
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(15_model_gltf PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                   "${CMAKE_BINARY_DIR}/bin")
endif()
set(resources "$<TARGET_FILE_DIR:15_model_gltf>/15_model_gltf.resources")
add_custom_target(
    15_model_gltf_resources
    COMMAND ${CMAKE_COMMAND} -E make_directory "${resources}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
            "${resources}/shaders")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET 15_model_gltf_resources
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "${resources}/assets")
endif()
add_dependencies(15_model_gltf 15_model_gltf_resources)
if(BUILD_TESTING)
    add_test(NAME 15_model_gltf COMMAND $<TARGET_FILE:15_model_gltf> --smoke)
    set_tests_properties(15_model_gltf PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
endif()
