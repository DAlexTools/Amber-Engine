set(ENGINE_EDITOR_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(ENGINE_EDITOR_OUTPUT_LOG_DIR ${ENGINE_EDITOR_DIR}/OutputLog)
set(ENGINE_EDITOR_DIAGNOSTICS_DIR ${ENGINE_EDITOR_DIR}/Diagnostics)
set(ENGINE_EDITOR_SHELL_DIR ${ENGINE_EDITOR_DIR}/Shell)

if(TARGET ImGuiSDL)
    add_library(EditorOutputLog STATIC
        ${ENGINE_EDITOR_OUTPUT_LOG_DIR}/OutputLogWidget.cpp
    )

    target_include_directories(EditorOutputLog PUBLIC
        ${ENGINE_EDITOR_OUTPUT_LOG_DIR}
    )

    target_link_libraries(EditorOutputLog PUBLIC
        AmberBuildConfig
        EngineRuntimeLogging
        ImGuiSDL
    )
    amber_target_folder(EditorOutputLog "Engine/Editor/OutputLog")
    amber_add_header_tree(EditorOutputLog "${ENGINE_EDITOR_OUTPUT_LOG_DIR}")
    amber_group_sources(EditorOutputLog "${ENGINE_EDITOR_DIR}")
    amber_enable_pch(EditorOutputLog)
elseif(BUILD_SDL_MODULES)
    message(WARNING
        "EditorOutputLog target is disabled because ImGuiSDL target is not available."
    )
endif()

if(TARGET EditorOutputLog AND TARGET ImGuiSDL)
    add_library(EditorDiagnostics STATIC
        ${ENGINE_EDITOR_DIAGNOSTICS_DIR}/SampleDiagnosticsOverlay.cpp
    )

    target_include_directories(EditorDiagnostics PUBLIC
        ${ENGINE_EDITOR_DIAGNOSTICS_DIR}
    )

    target_link_libraries(EditorDiagnostics PUBLIC
        AmberBuildConfig
        EditorOutputLog
        ImGuiSDL
    )
    amber_target_folder(EditorDiagnostics "Engine/Editor/Diagnostics")
    amber_add_header_tree(EditorDiagnostics "${ENGINE_EDITOR_DIAGNOSTICS_DIR}")
    amber_group_sources(EditorDiagnostics "${ENGINE_EDITOR_DIR}")
    amber_enable_pch(EditorDiagnostics)
elseif(BUILD_SDL_MODULES)
    message(WARNING
        "EditorDiagnostics target is disabled because EditorOutputLog or ImGuiSDL target is not available."
    )
endif()

if(TARGET ImGuiSDL AND TARGET EditorOutputLog AND SDL2_TARGET_FOUND AND SDL2_IMAGE_TARGET_FOUND)
    add_library(EditorShell STATIC
        ${ENGINE_EDITOR_SHELL_DIR}/AssetRegistry.cpp
        ${ENGINE_EDITOR_SHELL_DIR}/EditorViewport.cpp
        ${ENGINE_EDITOR_SHELL_DIR}/EditorApplication.cpp
        ${ENGINE_EDITOR_SHELL_DIR}/EditorPlaySession.cpp
        ${ENGINE_EDITOR_SHELL_DIR}/ProjectGenerator.cpp
        ${ENGINE_EDITOR_SHELL_DIR}/GameModuleResolver.cpp
        ${ENGINE_EDITOR_SHELL_DIR}/SceneDocument.cpp
        ${ENGINE_EDITOR_SHELL_DIR}/SelectionService.cpp
        ${ENGINE_EDITOR_SHELL_DIR}/TextureCache.cpp
    )

    target_include_directories(EditorShell PUBLIC
        ${ENGINE_EDITOR_SHELL_DIR}
    )

        target_link_libraries(EditorShell PUBLIC
            AmberBuildConfig
            EngineRuntimeCore
            EngineRuntimeECS
            EngineRuntimePlayer
            EditorOutputLog
            ImGuiSDL
            ${SDL2_LINK_TARGETS}
        ${SDL2_IMAGE_TARGET}
    )
    if(TARGET Engine)
        target_link_libraries(EditorShell PUBLIC Engine)
    endif()

    amber_target_folder(EditorShell "Engine/Editor/Shell")
    amber_add_header_tree(EditorShell "${ENGINE_EDITOR_SHELL_DIR}")
    amber_group_sources(EditorShell "${ENGINE_EDITOR_SHELL_DIR}")
    amber_enable_pch(EditorShell)

    add_executable(AmberEditor
        ${ENGINE_EDITOR_SHELL_DIR}/main.cpp
    )

    target_compile_definitions(AmberEditor PRIVATE SDL_MAIN_HANDLED SMOKE_TEST=1)
    target_link_libraries(AmberEditor PRIVATE EditorShell)

    amber_target_folder(AmberEditor "Engine/Editor/Shell")
    amber_group_sources(AmberEditor "${ENGINE_EDITOR_SHELL_DIR}")
    amber_enable_pch(AmberEditor)
elseif(BUILD_SDL_MODULES)
    message(WARNING
        "AmberEditor target is disabled because ImGuiSDL, EditorOutputLog, SDL2 or SDL2_image is not available."
    )
endif()
