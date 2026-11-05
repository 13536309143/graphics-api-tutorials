cmake_minimum_required(VERSION 3.24)
project(VulkanNvproCourse LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/Vulkan-Headers/include/vulkan/vulkan.h")
    set(course_headers_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/Vulkan-Headers")
else()
    FetchContent_Declare(
        course_headers
        URL https://codeload.github.com/KhronosGroup/Vulkan-Headers/zip/refs/tags/vulkan-sdk-1.4.341.0
        URL_HASH SHA256=1a0ea8ce1f1f08bcf9a7e4962646ba6b2a09b40108ca516fc6a006e45c77cb65)
    FetchContent_MakeAvailable(course_headers)
endif()
set(Vulkan_INCLUDE_DIR
    "${course_headers_SOURCE_DIR}/include"
    CACHE PATH "Project-pinned Vulkan headers" FORCE)
find_package(Vulkan 1.4.341 REQUIRED COMPONENTS glslc)
set(Vulkan_VOLK_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/external/nvpro_core2/third_party/volk"
    CACHE PATH "Use matching bundled Volk" FORCE)
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/nvpro_core2/nvapp/application.hpp")
    message(FATAL_ERROR "Initialize dependencies: git submodule update --init --recursive")
endif()
set(NVPRO2_ENABLE_DEFAULT
    OFF
    CACHE BOOL "" FORCE)
foreach(lib nvutils nvapp nvgui nvvk)
    set(NVPRO2_ENABLE_${lib}
        ON
        CACHE BOOL "" FORCE)
endforeach()
add_subdirectory(external/nvpro_core2 EXCLUDE_FROM_ALL)
if(EXISTS
   "${CMAKE_CURRENT_SOURCE_DIR}/external/Vulkan-Utility-Libraries/include/vulkan/vk_enum_string_helper.h"
)
    set(course_utils_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/Vulkan-Utility-Libraries")
else()
    FetchContent_Declare(
        course_utils
        URL https://codeload.github.com/KhronosGroup/Vulkan-Utility-Libraries/zip/refs/tags/vulkan-sdk-1.4.341.0
        URL_HASH SHA256=61de60ed4c2a6c10b1c7091a472f1fdc4fbab5031f8fe1f85d4a72e49552b779
        SOURCE_SUBDIR header-only-no-config)
    FetchContent_GetProperties(course_utils)
    if(NOT course_utils_POPULATED)
        FetchContent_MakeAvailable(course_utils)
    endif()
endif()
target_include_directories(nvvk PRIVATE "${course_utils_SOURCE_DIR}/include")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Each chapter owns its C++ and shader sources. Only third-party targets are reused.
include(CTest)
file(GLOB projects CONFIGURE_DEPENDS "lessons/*" "chapters/*")
foreach(dir IN LISTS projects)
    if(EXISTS "${dir}/CMakeLists.txt")
        add_subdirectory("${dir}")
    endif()
endforeach()
