if(TARGET ImGuiSDL AND TARGET EditorOutputLog AND SDL2_TARGET_FOUND AND SDL2_IMAGE_TARGET_FOUND)
    add_library(EditorShell STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/AssetRegistry.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/EditorViewport.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/EditorApplication.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/EditorPlaySession.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ProjectDescriptor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ProjectGenerator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/GameModuleResolver.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/SceneDocument.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/SelectionService.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/TextureCache.cpp
    )

    target_include_directories(EditorShell PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(EditorShell PUBLIC
        AmberBuildConfig
        EngineRuntimeCore
        EngineRuntimeECS
        EditorOutputLog
        ImGuiSDL
        ${SDL2_LINK_TARGETS}
        ${SDL2_IMAGE_TARGET}
    )
    if(TARGET Engine)
        target_link_libraries(EditorShell PUBLIC Engine)
    endif()

    amber_target_folder(EditorShell "Engine/Editor/Shell")
    amber_add_header_tree(EditorShell "${CMAKE_CURRENT_SOURCE_DIR}/include")
    amber_group_sources(EditorShell "${CMAKE_CURRENT_SOURCE_DIR}")

    add_executable(AmberEditor
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    )

    target_compile_definitions(AmberEditor PRIVATE SDL_MAIN_HANDLED SMOKE_TEST=1)
    target_link_libraries(AmberEditor PRIVATE EditorShell)

    amber_target_folder(AmberEditor "Engine/Editor/Shell")
    amber_group_sources(AmberEditor "${CMAKE_CURRENT_SOURCE_DIR}")
elseif(BUILD_SDL_MODULES)
    message(WARNING
        "AmberEditor target is disabled because ImGuiSDL, EditorOutputLog, SDL2 or SDL2_image is not available."
    )
endif()
