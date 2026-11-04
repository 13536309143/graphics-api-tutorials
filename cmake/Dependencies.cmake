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
