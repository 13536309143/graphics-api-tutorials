if(TARGET nvpro2::nvapp)
    return()
endif()
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
FetchContent_Declare(
    nvpro_core2
    GIT_REPOSITORY https://github.com/nvpro-samples/nvpro_core2.git
    GIT_TAG 71ab4212aa3dd8e3b6cb659ed691b59062dc5f11)
FetchContent_GetProperties(nvpro_core2)
if(NOT nvpro_core2_POPULATED)
    FetchContent_Populate(nvpro_core2)
endif()
set(NVPRO2_ENABLE_DEFAULT
    OFF
    CACHE BOOL "" FORCE)
foreach(lib nvutils nvapp nvgui nvvk)
    set(NVPRO2_ENABLE_${lib}
        ON
        CACHE BOOL "" FORCE)
endforeach()
set(Vulkan_VOLK_DIR
    "${nvpro_core2_SOURCE_DIR}/third_party/volk"
    CACHE PATH "Pinned Volk" FORCE)
add_subdirectory("${nvpro_core2_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/nvpro-core2" EXCLUDE_FROM_ALL)
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
