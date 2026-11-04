cmake_minimum_required(VERSION 3.24)
project(GraphicsApiTutorials LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(GRAPHICS_TRACK
    launcher
    CACHE STRING "launcher, opengl, vulkan-native, vulkan-nvpro, dx12")
if(GRAPHICS_TRACK STREQUAL "opengl")
    add_subdirectory(OpenGL)
elseif(GRAPHICS_TRACK STREQUAL "vulkan-native")
    add_subdirectory(Vulkan/native)
elseif(GRAPHICS_TRACK STREQUAL "vulkan-nvpro")
    add_subdirectory(Vulkan/nvpro)
elseif(GRAPHICS_TRACK STREQUAL "dx12")
    add_subdirectory(DirectX12)
elseif(GRAPHICS_TRACK STREQUAL "launcher")
    if(NOT WIN32)
        message(
            FATAL_ERROR
                "The native click-to-run reader is Windows-only; browse Markdown on other platforms"
        )
    endif()
    add_executable(course_browser WIN32 tools/course_browser.cpp)
    target_link_libraries(course_browser PRIVATE ws2_32 shell32 bcrypt)
    target_compile_definitions(course_browser PRIVATE UNICODE _UNICODE)
    set_target_properties(course_browser PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                    "${CMAKE_BINARY_DIR}/launcher/$<CONFIG>")
    if(MSVC)
        target_compile_options(course_browser PRIVATE /utf-8 /W4 /permissive-)
    endif()
else()
    message(FATAL_ERROR "Unknown GRAPHICS_TRACK")
endif()
enable_testing()
