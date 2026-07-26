if(TARGET ImGuiSDL AND TARGET EditorOutputLog AND SDL2_TARGET_FOUND)
    add_library(EditorShell STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/EditorViewport.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/EditorApplication.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/SceneDocument.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/SelectionService.cpp
    )

    target_include_directories(EditorShell PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(EditorShell PUBLIC
        AmberBuildConfig
        EditorOutputLog
        ImGuiSDL
        ${SDL2_LINK_TARGETS}
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
        "AmberEditor target is disabled because ImGuiSDL, EditorOutputLog or SDL2 is not available."
    )
endif()
