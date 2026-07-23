if(TARGET EditorOutputLog AND TARGET ImGuiSDL)
    add_library(EditorDiagnostics STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/SampleDiagnosticsOverlay.cpp
    )

    target_include_directories(EditorDiagnostics PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(EditorDiagnostics PUBLIC
        AmberBuildConfig
        EditorOutputLog
        ImGuiSDL
    )
    amber_target_folder(EditorDiagnostics "Engine/Editor/Diagnostics")
    amber_add_header_tree(EditorDiagnostics "${CMAKE_CURRENT_SOURCE_DIR}/include")
    amber_group_sources(EditorDiagnostics "${CMAKE_CURRENT_SOURCE_DIR}/..")
elseif(BUILD_SDL_MODULES)
    message(WARNING
        "EditorDiagnostics target is disabled because EditorOutputLog or ImGuiSDL target is not available."
    )
endif()
