cmake_minimum_required(VERSION 3.24)
project(VulkanNativeCourse LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Vulkan 1.3 REQUIRED COMPONENTS glslc)
include(../../cmake/Dependencies.cmake)
course_dependencies()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
add_library(vk_course INTERFACE)
target_include_directories(vk_course INTERFACE common ../common ../../shared
                                               "${COURSE_GLM_INCLUDE}")
target_link_libraries(vk_course INTERFACE Vulkan::Vulkan glfw)
target_compile_definitions(vk_course INTERFACE GLFW_INCLUDE_VULKAN GLM_FORCE_DEPTH_ZERO_TO_ONE)
if(MSVC)
    target_compile_options(vk_course INTERFACE /utf-8 /W4 /permissive- /wd4324)
endif()
set(shader_files triangle.vert scene.vert scene.frag square.comp lab.comp particles.comp)
foreach(shader IN LISTS shader_files)
    add_custom_command(
        OUTPUT "${CMAKE_BINARY_DIR}/spv/${shader}.spv"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/spv"
        COMMAND
            Vulkan::glslc --target-env=vulkan1.3
            "${CMAKE_CURRENT_SOURCE_DIR}/../common/shaders/${shader}" -o
            "${CMAKE_BINARY_DIR}/spv/${shader}.spv"
        DEPENDS "../common/shaders/${shader}")
    list(APPEND shader_outputs "${CMAKE_BINARY_DIR}/spv/${shader}.spv")
endforeach()
add_custom_target(vk_shaders DEPENDS ${shader_outputs})
get_property(multi GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
set(runtime_shader_dir "${CMAKE_BINARY_DIR}/bin")
if(multi)
    string(APPEND runtime_shader_dir "/$<CONFIG>")
endif()
add_custom_target(
    vk_runtime_assets COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/spv"
                              "${runtime_shader_dir}/shaders")
add_dependencies(vk_runtime_assets vk_shaders)
course_asset_dependencies()
add_library(vk_advanced STATIC common/advanced_labs.cpp)
target_link_libraries(vk_advanced PUBLIC vk_course assimp::assimp)
target_include_directories(vk_advanced
                           PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../OpenGL/third_party/stb")
add_custom_target(
    vk_advanced_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../../OpenGL/assets"
            "${runtime_shader_dir}/assets")

include(CTest)
file(GLOB lessons CONFIGURE_DEPENDS "lessons/*")
foreach(dir IN LISTS lessons)
    get_filename_component(name "${dir}" NAME)
    add_executable(${name} "${dir}/main.cpp")
    target_link_libraries(${name} PRIVATE vk_course)
    if(EXISTS "${dir}/advanced.json")
        target_link_libraries(${name} PRIVATE vk_advanced)
        add_dependencies(${name} vk_advanced_assets)
    endif()
    target_compile_definitions(${name} PRIVATE COURSE_SHADER_DIR="${CMAKE_BINARY_DIR}/spv")
    add_dependencies(${name} vk_runtime_assets)
    add_test(NAME ${name} COMMAND ${name} --smoke)
    set_tests_properties(${name} PROPERTIES TIMEOUT 40 RUN_SERIAL TRUE)
endforeach()
