cmake_minimum_required(VERSION 3.24)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(dx21_obj_model LANGUAGES C CXX)
    include(CTest)
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(cmake/Dependencies.cmake)
course_dependencies()
course_asset_dependencies()
add_executable(dx21_obj_model main.cpp)
target_include_directories(dx21_obj_model PRIVATE support third_party/stb "${COURSE_GLM_INCLUDE}")
target_link_libraries(dx21_obj_model PRIVATE d3d12 dxgi dxguid d3dcompiler glfw assimp::assimp)
target_compile_definitions(
    dx21_obj_model PRIVATE UNICODE _UNICODE NOMINMAX GLFW_INCLUDE_NONE GLM_FORCE_DEPTH_ZERO_TO_ONE
                           COURSE_RUNTIME_NAME="dx21_obj_model.resources")
if(MSVC)
    target_compile_options(dx21_obj_model PRIVATE /utf-8 /W4 /wd4324 /permissive-)
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(dx21_obj_model PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                    "${CMAKE_BINARY_DIR}/bin")
endif()
set(resources "$<TARGET_FILE_DIR:dx21_obj_model>/dx21_obj_model.resources")
add_custom_target(
    dx21_obj_model_resources
    COMMAND ${CMAKE_COMMAND} -E make_directory "${resources}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
            "${resources}/shaders")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET dx21_obj_model_resources
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "${resources}/assets")
endif()
add_dependencies(dx21_obj_model dx21_obj_model_resources)
if(BUILD_TESTING)
    add_test(NAME dx21_obj_model COMMAND $<TARGET_FILE:dx21_obj_model> --smoke)
    set_tests_properties(dx21_obj_model PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
endif()
