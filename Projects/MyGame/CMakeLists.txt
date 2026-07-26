cmake_minimum_required(VERSION 3.16)

project(MyGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(AMBER_ENGINE_ROOT "C:/Users/admin/Documents/GitProjects/SDL_GameProgramming-master/AmberEngine" CACHE PATH "Path to AmberEngine root")
if(NOT EXISTS "${AMBER_ENGINE_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR "AMBER_ENGINE_ROOT must point to an AmberEngine checkout")
endif()

set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_SDL_MODULES ON CACHE BOOL "" FORCE)

add_subdirectory("${AMBER_ENGINE_ROOT}" "${CMAKE_BINARY_DIR}/AmberEngine")

add_library(MyGameModule STATIC
    Source/MyGame/MyGameModule.cpp
)

target_include_directories(MyGameModule PUBLIC
    Source/MyGame
)

target_link_libraries(MyGameModule PUBLIC
    AmberBuildConfig
    EngineRuntimeCore
    EngineRuntimeLogging
)

add_executable(MyGameLauncher
    Source/MyGameLauncher/main.cpp
)

target_link_libraries(MyGameLauncher PRIVATE MyGameModule)
