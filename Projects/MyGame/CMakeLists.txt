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
set(AMBER_BUILD_PROJECTS OFF CACHE BOOL "" FORCE)

add_subdirectory("${AMBER_ENGINE_ROOT}" "${CMAKE_BINARY_DIR}/AmberEngine")

add_library(MyGameModule STATIC
    Source/MyGame/MyGameModule.cpp
    Source/MyGame/MyGameModule.h
)

target_include_directories(MyGameModule PUBLIC
    Source/MyGame
)

set_target_properties(MyGameModule PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    FOLDER "Projects/MyGame"
)

target_link_libraries(MyGameModule PUBLIC
    AmberBuildConfig
    EngineRuntimeCore
    EngineRuntimeECS
    EngineRuntimeLogging
)
if(COMMAND amber_enable_pch)
    amber_enable_pch(MyGameModule)
endif()

add_library(MyGameModulePlugin SHARED
    Source/MyGame/MyGameModulePlugin.cpp
)

target_link_libraries(MyGameModulePlugin PRIVATE MyGameModule)
set_target_properties(MyGameModulePlugin PROPERTIES FOLDER "Projects/MyGame")
if(COMMAND amber_enable_pch)
    amber_enable_pch(MyGameModulePlugin)
endif()

add_executable(MyGameLauncher
    Source/MyGameLauncher/main.cpp
)

if(NOT TARGET EngineRuntimePlayer)
    message(FATAL_ERROR "EngineRuntimePlayer target is required for MyGameLauncher. Configure with SDL2 and SDL2_image.")
endif()

target_compile_definitions(MyGameLauncher PRIVATE SDL_MAIN_HANDLED)
target_link_libraries(MyGameLauncher PRIVATE MyGameModule EngineRuntimePlayer)
set_target_properties(MyGameLauncher PROPERTIES FOLDER "Projects/MyGame")
if(COMMAND amber_enable_pch)
    amber_enable_pch(MyGameLauncher)
endif()
