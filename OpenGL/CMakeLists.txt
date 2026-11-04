cmake_minimum_required(VERSION 3.24)
project(
    OpenGLCourse
    VERSION 1.0
    LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(GLFW_BUILD_DOCS
    OFF
    CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS
    OFF
    CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES
    OFF
    CACHE BOOL "" FORCE)
set(GLFW_INSTALL
    OFF
    CACHE BOOL "" FORCE)

include(FetchContent)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw/CMakeLists.txt")
    add_subdirectory(third_party/glfw EXCLUDE_FROM_ALL)
else()
    FetchContent_Declare(
        glfw
        URL https://codeload.github.com/glfw/glfw/zip/refs/tags/3.4
        URL_HASH SHA256=a133ddc3d3c66143eba9035621db8e0bcf34dba1ee9514a9e23e96afd39fd57a)
    FetchContent_MakeAvailable(glfw)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glm/glm/glm.hpp")
    set(COURSE_GLM "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glm")
else()
    FetchContent_Declare(
        course_glm
        URL https://codeload.github.com/g-truc/glm/zip/1.0.1
        URL_HASH SHA256=09c5716296787e1f7fcb87b1cbdbf26814ec1288ed6259ccd30d5d9795809fa5)
    FetchContent_MakeAvailable(course_glm)
    set(COURSE_GLM "${course_glm_SOURCE_DIR}")
endif()

add_library(glad STATIC third_party/glad/src/gl.c)
target_include_directories(glad PUBLIC third_party/glad/include)
target_link_libraries(glad PUBLIC ${CMAKE_DL_LIBS})

# 只启用课程需要的导入器，缩短离线构建时间。启用更多格式可修改以下开关。
set(BUILD_SHARED_LIBS
    OFF
    CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS
    OFF
    CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS
    OFF
    CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL
    OFF
    CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS
    OFF
    CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT
    OFF
    CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER
    ON
    CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER
    ON
    CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT
    OFF
    CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT
    ON
    CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB
    ON
    CACHE BOOL "" FORCE)
if(POLICY CMP0175)
    set(CMAKE_POLICY_DEFAULT_CMP0175 OLD)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/assimp/CMakeLists.txt")
    add_subdirectory(third_party/assimp EXCLUDE_FROM_ALL)
else()
    FetchContent_Declare(
        assimp
        URL https://codeload.github.com/assimp/assimp/zip/refs/tags/v5.4.3
        URL_HASH SHA256=795c29716f4ac123b403e53b677e9f32a8605c4a7b2d9904bfaae3f4053b506d)
    FetchContent_MakeAvailable(assimp)
endif()

unset(CMAKE_POLICY_DEFAULT_CMP0175)
add_library(course_images STATIC common/image.cpp)
target_include_directories(course_images PUBLIC third_party/stb)
add_library(course INTERFACE)
target_include_directories(course INTERFACE common "${COURSE_GLM}")
target_link_libraries(course INTERFACE glfw glad course_images assimp::assimp)
target_compile_definitions(course INTERFACE GLFW_INCLUDE_NONE)
if(MSVC)
    target_compile_options(course INTERFACE /utf-8 /W4 /permissive-)
else()
    target_compile_options(course INTERFACE -Wall -Wextra -Wpedantic)
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
include(CTest)
set(shader_output "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
if(CMAKE_CONFIGURATION_TYPES)
    string(APPEND shader_output "/$<CONFIG>")
endif()
add_custom_target(
    course_shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
            "${shader_output}/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "${shader_output}/assets")
file(
    GLOB lesson_dirs
    LIST_DIRECTORIES true
    CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/lessons/*")
foreach(dir IN LISTS lesson_dirs)
    if(IS_DIRECTORY "${dir}")
        get_filename_component(name "${dir}" NAME)
        add_executable(${name} "${dir}/main.cpp")
        target_link_libraries(${name} PRIVATE course)
        # 每次构建都复制着色器，修改 GLSL 后重新构建即可，不依赖运行时工作目录。
        add_dependencies(${name} course_shaders)
        if(BUILD_TESTING)
            add_test(NAME ${name} COMMAND $<TARGET_FILE:${name}> --smoke)
            set_tests_properties(${name} PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE)
        endif()
    endif()
endforeach()

# Windows native offline reader and click-to-run bridge. No Python runtime required.
if(WIN32)
    add_executable(course_browser WIN32 tools/course_browser.cpp)
    target_link_libraries(course_browser PRIVATE ws2_32 shell32 bcrypt)
    target_compile_definitions(course_browser PRIVATE UNICODE _UNICODE)
    set_target_properties(course_browser PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                    "${CMAKE_BINARY_DIR}/launcher/$<CONFIG>")
    if(MSVC)
        target_compile_options(course_browser PRIVATE /utf-8 /W4 /permissive-)
    endif()
endif()
