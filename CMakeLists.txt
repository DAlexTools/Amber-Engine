cmake_minimum_required(VERSION 3.16)

project(AmberEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "_CMake")

option(BUILD_SDL_MODULES "Build SDL renderer, samples and game engine targets" ON)
option(BUILD_UNIT_TESTS "Build GoogleTest unit test targets" OFF)
option(AMBER_WITH_EDITOR "Build Amber editor modules and tools" ON)
set(AMBER_WITH_EDITOR_ONLY_DATA_DEFAULT ${AMBER_WITH_EDITOR})
option(AMBER_WITH_EDITOR_ONLY_DATA "Keep editor-only runtime metadata in compiled types" ${AMBER_WITH_EDITOR_ONLY_DATA_DEFAULT})

add_library(AmberBuildConfig INTERFACE)
set_target_properties(AmberBuildConfig PROPERTIES FOLDER "Engine/Build")
target_compile_definitions(AmberBuildConfig INTERFACE
    AMBER_WITH_EDITOR=$<BOOL:${AMBER_WITH_EDITOR}>
    AMBER_WITH_EDITOR_ONLY_DATA=$<BOOL:${AMBER_WITH_EDITOR_ONLY_DATA}>
    WITH_EDITOR=$<BOOL:${AMBER_WITH_EDITOR}>
    WITH_EDITOR_ONLY_DATA=$<BOOL:${AMBER_WITH_EDITOR_ONLY_DATA}>
)

function(amber_target_folder target folder)
    if(TARGET ${target})
        set_target_properties(${target} PROPERTIES FOLDER "${folder}")
    endif()
endfunction()

function(amber_add_header_tree target root)
    if(NOT TARGET ${target})
        return()
    endif()

    file(GLOB_RECURSE header_files CONFIGURE_DEPENDS
        "${root}/*.h"
        "${root}/*.hh"
        "${root}/*.hpp"
        "${root}/*.hxx"
        "${root}/*.inl"
        "${root}/*.ipp"
    )

    if(header_files)
        target_sources(${target} PRIVATE ${header_files})
    endif()
endfunction()

function(amber_group_sources target root)
    if(NOT TARGET ${target})
        return()
    endif()

    get_target_property(target_sources ${target} SOURCES)
    if(NOT target_sources)
        return()
    endif()

    set(group_files)
    foreach(source IN LISTS target_sources)
        if(IS_ABSOLUTE "${source}")
            set(abs_source "${source}")
        else()
            get_filename_component(abs_source "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
        endif()

        if(EXISTS "${abs_source}")
            file(RELATIVE_PATH relative_source "${root}" "${abs_source}")
            if(NOT relative_source MATCHES "^\\.\\.")
                list(APPEND group_files "${abs_source}")
            endif()
        endif()
    endforeach()

    if(group_files)
        list(REMOVE_DUPLICATES group_files)
        source_group(TREE "${root}" FILES ${group_files})
    endif()
endfunction()

function(resolve_optional_target output)
    foreach(candidate ${ARGN})
        if(TARGET ${candidate})
            set(${output} ${candidate} PARENT_SCOPE)
            set(${output}_FOUND TRUE PARENT_SCOPE)
            return()
        endif()
    endforeach()

    set(${output}_FOUND FALSE PARENT_SCOPE)
endfunction()

if(BUILD_SDL_MODULES)
    find_package(SDL2 CONFIG QUIET)
    find_package(SDL2_image CONFIG QUIET)
    find_package(SDL2_mixer CONFIG QUIET)
    find_package(SDL2_ttf CONFIG QUIET)
    find_package(sdl2-gfx CONFIG QUIET)
    find_package(unofficial-sdl2-gfx CONFIG QUIET)
    find_package(SDL2_gfx CONFIG QUIET)
    find_package(Lua QUIET)

    resolve_optional_target(SDL2_TARGET SDL2::SDL2 SDL2::SDL2-static)
    resolve_optional_target(SDL2_IMAGE_TARGET SDL2_image::SDL2_image SDL2_image::SDL2_image-static)
    resolve_optional_target(SDL2_MIXER_TARGET SDL2_mixer::SDL2_mixer SDL2_mixer::SDL2_mixer-static)
    resolve_optional_target(SDL2_TTF_TARGET SDL2_ttf::SDL2_ttf SDL2_ttf::SDL2_ttf-static)
    resolve_optional_target(SDL2_GFX_TARGET SDL2::SDL2_gfx unofficial::sdl2-gfx::sdl2-gfx SDL2_gfx::SDL2_gfx SDL2_gfx::SDL2_gfx-static)

    set(SDL2_LINK_TARGETS ${SDL2_TARGET})

    if(TARGET Lua::Lua)
        set(LUA_LINK_TARGETS Lua::Lua)
        set(LUA_PACKAGE_FOUND TRUE)
    elseif(Lua_FOUND OR LUA_FOUND)
        set(LUA_LINK_TARGETS ${LUA_LIBRARIES})
        set(LUA_PACKAGE_FOUND TRUE)
    else()
        set(LUA_PACKAGE_FOUND FALSE)
    endif()

    set(LUA_INCLUDE_DIRS_EFFECTIVE ${LUA_INCLUDE_DIR})
    if(LUA_INCLUDE_DIRS)
        list(APPEND LUA_INCLUDE_DIRS_EFFECTIVE ${LUA_INCLUDE_DIRS})
    endif()
endif()

add_subdirectory(Engine)

if(BUILD_SDL_MODULES)
    add_subdirectory(Samples)
endif()

if(BUILD_UNIT_TESTS)
    find_package(GTest CONFIG QUIET)

    if(GTest_FOUND)
        enable_testing()
        add_subdirectory(Tests)
    else()
        message(WARNING
            "BUILD_UNIT_TESTS=ON, but GoogleTest was not found. "
            "Configure with vcpkg or install GTest to build unit tests."
        )
    endif()
endif()
