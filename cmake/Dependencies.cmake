include_guard(GLOBAL)
include(FetchContent)
function(course_dependencies)
    if(NOT TARGET glfw)
        set(GLFW_BUILD_DOCS
            OFF
            CACHE BOOL "" FORCE)
        set(GLFW_BUILD_EXAMPLES
            OFF
            CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS
            OFF
            CACHE BOOL "" FORCE)
        set(GLFW_INSTALL
            OFF
            CACHE BOOL "" FORCE)
        if(EXISTS "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../OpenGL/third_party/glfw/CMakeLists.txt")
            add_subdirectory("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../OpenGL/third_party/glfw"
                             "${CMAKE_BINARY_DIR}/glfw")
        else()
            FetchContent_Declare(
                glfw
                URL https://codeload.github.com/glfw/glfw/zip/refs/tags/3.4
                URL_HASH SHA256=a133ddc3d3c66143eba9035621db8e0bcf34dba1ee9514a9e23e96afd39fd57a)
            FetchContent_MakeAvailable(glfw)
        endif()
    endif()
    if(EXISTS "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../OpenGL/third_party/glm/glm/glm.hpp")
        set(COURSE_GLM_INCLUDE
            "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../OpenGL/third_party/glm"
            CACHE INTERNAL "")
    else()
        FetchContent_Declare(
            course_glm
            URL https://codeload.github.com/g-truc/glm/zip/1.0.1
            URL_HASH SHA256=09c5716296787e1f7fcb87b1cbdbf26814ec1288ed6259ccd30d5d9795809fa5)
        FetchContent_GetProperties(course_glm)
        if(NOT course_glm_POPULATED)
            FetchContent_MakeAvailable(course_glm)
        endif()
        set(COURSE_GLM_INCLUDE
            "${course_glm_SOURCE_DIR}"
            CACHE INTERNAL "")
    endif()
endfunction()

# Shared asset decoding for the native Vulkan and DirectX 12 advanced labs.
function(course_asset_dependencies)
    if(TARGET assimp::assimp)
        return()
    endif()
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
    set(asset_root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../OpenGL")
    if(EXISTS "${asset_root}/third_party/assimp/CMakeLists.txt")
        add_subdirectory("${asset_root}/third_party/assimp" "${CMAKE_BINARY_DIR}/assimp"
                         EXCLUDE_FROM_ALL)
    else()
        FetchContent_Declare(
            assimp
            URL https://codeload.github.com/assimp/assimp/zip/refs/tags/v5.4.3
            URL_HASH SHA256=795c29716f4ac123b403e53b677e9f32a8605c4a7b2d9904bfaae3f4053b506d)
        FetchContent_MakeAvailable(assimp)
    endif()
endfunction()
