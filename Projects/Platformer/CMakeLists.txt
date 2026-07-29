cmake_minimum_required(VERSION 3.16)

project(Platformer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT DEFINED AMBER_ENGINE_ROOT)
    get_filename_component(AMBER_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()

set(AMBER_ENGINE_ROOT "${AMBER_ENGINE_ROOT}" CACHE PATH "Path to AmberEngine root")
set(ENGINE_THIRD_PARTY_DIR "${AMBER_ENGINE_ROOT}/Engine/ThirdParty")
set(PLATFORMER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Source/Platformer)
set(PLATFORMER_LAUNCHER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Source/PlatformerLauncher)

if(NOT TARGET AmberBuildConfig)
    set(BUILD_SDL_MODULES ON CACHE BOOL "" FORCE)
    set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
    set(AMBER_BUILD_PROJECTS OFF CACHE BOOL "" FORCE)
    add_subdirectory("${AMBER_ENGINE_ROOT}" "${CMAKE_BINARY_DIR}/AmberEngine")
endif()

if(NOT SDL2_TARGET_FOUND)
    function(platformer_resolve_optional_target output)
        foreach(candidate ${ARGN})
            if(TARGET ${candidate})
                set(${output} ${candidate} PARENT_SCOPE)
                set(${output}_FOUND TRUE PARENT_SCOPE)
                return()
            endif()
        endforeach()

        set(${output}_FOUND FALSE PARENT_SCOPE)
    endfunction()

    find_package(SDL2 CONFIG QUIET)
    find_package(SDL2_image CONFIG QUIET)
    find_package(Lua QUIET)

    platformer_resolve_optional_target(SDL2_TARGET SDL2::SDL2 SDL2::SDL2-static)
    platformer_resolve_optional_target(SDL2_IMAGE_TARGET SDL2_image::SDL2_image SDL2_image::SDL2_image-static)
    set(SDL2_LINK_TARGETS ${SDL2_TARGET})

    if(TARGET Lua::Lua)
        set(LUA_LINK_TARGETS Lua::Lua)
    elseif(Lua_FOUND OR LUA_FOUND)
        set(LUA_LINK_TARGETS ${LUA_LIBRARIES})
    endif()

    set(LUA_INCLUDE_DIRS_EFFECTIVE ${LUA_INCLUDE_DIR})
    if(LUA_INCLUDE_DIRS)
        list(APPEND LUA_INCLUDE_DIRS_EFFECTIVE ${LUA_INCLUDE_DIRS})
    endif()
endif()

if(SDL2_TARGET_FOUND)
    add_library(PlatformerGameModule STATIC
        ${PLATFORMER_SOURCE_DIR}/PlatformerGameModule.cpp
        ${PLATFORMER_SOURCE_DIR}/PlatformerSceneObjects.cpp
    )

    target_compile_definitions(PlatformerGameModule PUBLIC SMOKE_TEST=1)
    set_target_properties(PlatformerGameModule PROPERTIES POSITION_INDEPENDENT_CODE ON)

    target_include_directories(PlatformerGameModule PUBLIC
        ${PLATFORMER_SOURCE_DIR}
        ${ENGINE_THIRD_PARTY_DIR}
        ${LUA_INCLUDE_DIRS_EFFECTIVE}
    )

    target_link_libraries(PlatformerGameModule PUBLIC
        AmberBuildConfig
        EngineRuntimeCore
        EngineRuntimeECS
        Physics
        ${LUA_LINK_TARGETS}
        ${SDL2_LINK_TARGETS}
    )

    if(SDL2_IMAGE_TARGET_FOUND)
        target_compile_definitions(PlatformerGameModule PUBLIC AMBER_ENABLE_PLATFORMER_EDITOR_SCENE=1)
        target_link_libraries(PlatformerGameModule PUBLIC ${SDL2_IMAGE_TARGET})
    endif()

    if(TARGET EditorDiagnostics)
        target_compile_definitions(PlatformerGameModule PUBLIC AMBER_ENABLE_SAMPLE_DIAGNOSTICS)
        target_link_libraries(PlatformerGameModule PUBLIC EditorDiagnostics)
    endif()

    amber_target_folder(PlatformerGameModule "Projects/Platformer")
    amber_add_header_tree(PlatformerGameModule "${PLATFORMER_SOURCE_DIR}")
    amber_group_sources(PlatformerGameModule "${CMAKE_CURRENT_SOURCE_DIR}")
    amber_enable_pch(PlatformerGameModule)

    add_library(PlatformerGameModulePlugin SHARED
        ${PLATFORMER_SOURCE_DIR}/PlatformerGameModulePlugin.cpp
    )

    target_link_libraries(PlatformerGameModulePlugin PRIVATE PlatformerGameModule)
    amber_target_folder(PlatformerGameModulePlugin "Projects/Platformer")
    amber_group_sources(PlatformerGameModulePlugin "${CMAKE_CURRENT_SOURCE_DIR}")
    amber_enable_pch(PlatformerGameModulePlugin)

    add_executable(PlatformerApp
        ${PLATFORMER_LAUNCHER_DIR}/main.cpp
    )

    if(NOT TARGET EngineRuntimePlayer)
        message(FATAL_ERROR "EngineRuntimePlayer target is required for PlatformerApp. Configure with SDL2 and SDL2_image.")
    endif()

    target_compile_definitions(PlatformerApp PRIVATE SDL_MAIN_HANDLED)
    target_link_libraries(PlatformerApp PRIVATE PlatformerGameModule EngineRuntimePlayer)
    amber_target_folder(PlatformerApp "Projects/Platformer")
    amber_group_sources(PlatformerApp "${CMAKE_CURRENT_SOURCE_DIR}")
    amber_enable_pch(PlatformerApp)

    if(TARGET AmberEditor)
        add_dependencies(AmberEditor PlatformerApp PlatformerGameModulePlugin)
    endif()
else()
    message(WARNING
        "PlatformerApp target is disabled because SDL2 was not found."
    )
endif()
