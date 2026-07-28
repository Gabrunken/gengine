cmake_minimum_required(VERSION 3.14)
project(gengine)

set(CMAKE_C_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

set(RAYLIB_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

set(GECS_BUILD_TEST OFF CACHE BOOL "" FORCE)

include(FetchContent)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.0
)

FetchContent_Declare(
    gecslib
    GIT_REPOSITORY https://github.com/gabrunken/gecs.git
    GIT_TAG main
)

FetchContent_MakeAvailable(raylib)
FetchContent_MakeAvailable(gecslib)

add_library(genginelib STATIC "./src/gengine.c")

target_include_directories(genginelib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(genginelib PUBLIC gecslib raylib)
target_compile_definitions(genginelib PRIVATE GENGINE_DEBUG_LOG)

option(GENGINE_BUILD_TEST "Build the test" ON)

if(GENGINE_BUILD_TEST)
    add_executable(gengine_test "test/main.c")
    target_link_libraries(gengine_test PRIVATE genginelib)
endif()
