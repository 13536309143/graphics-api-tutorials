cmake_minimum_required(VERSION 3.24)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(vk06_texture_descriptor LANGUAGES C CXX)
    include(CTest)
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(cmake/Dependencies.cmake)
course_dependencies()
find_package(Vulkan REQUIRED COMPONENTS glslc)
add_executable(vk06_texture_descriptor main.cpp)
target_include_directories(vk06_texture_descriptor PRIVATE support third_party/stb
                                                           "${COURSE_GLM_INCLUDE}")
target_link_libraries(vk06_texture_descriptor PRIVATE Vulkan::Vulkan glfw)
target_compile_definitions(
    vk06_texture_descriptor PRIVATE GLFW_INCLUDE_VULKAN GLM_FORCE_DEPTH_ZERO_TO_ONE
                                    COURSE_RUNTIME_NAME="vk06_texture_descriptor.resources")
if(MSVC)
    target_compile_options(vk06_texture_descriptor PRIVATE /utf-8 /W4 /wd4324 /permissive-)
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(vk06_texture_descriptor PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                             "${CMAKE_BINARY_DIR}/bin")
endif()
set(resources "$<TARGET_FILE_DIR:vk06_texture_descriptor>/vk06_texture_descriptor.resources")
file(GLOB stage_files CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert"
     "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag" "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.comp")
set(stage_outputs)
foreach(stage IN LISTS stage_files)
    get_filename_component(stage_name "${stage}" NAME)
    set(output "${CMAKE_CURRENT_BINARY_DIR}/spv/${stage_name}.spv")
    add_custom_command(
        OUTPUT "${output}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/spv"
        COMMAND Vulkan::glslc --target-env=vulkan1.3 "${stage}" -o "${output}"
        DEPENDS "${stage}"
        VERBATIM)
    list(APPEND stage_outputs "${output}")
endforeach()
add_custom_target(vk06_texture_descriptor_stages DEPENDS ${stage_outputs})
add_custom_target(
    vk06_texture_descriptor_resources
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/spv"
            "${resources}/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_BINARY_DIR}/spv"
            "${resources}/shaders")
add_dependencies(vk06_texture_descriptor_resources vk06_texture_descriptor_stages)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET vk06_texture_descriptor_resources
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "${resources}/assets")
endif()
add_dependencies(vk06_texture_descriptor vk06_texture_descriptor_resources)
if(BUILD_TESTING)
    add_test(NAME vk06_texture_descriptor COMMAND $<TARGET_FILE:vk06_texture_descriptor> --smoke)
    set_tests_properties(vk06_texture_descriptor PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
endif()
