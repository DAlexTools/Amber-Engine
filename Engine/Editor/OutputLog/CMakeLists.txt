if(TARGET ImGuiSDL)
    add_library(EditorOutputLog STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/OutputLogWidget.cpp
    )

    target_include_directories(EditorOutputLog PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(EditorOutputLog PUBLIC
        AmberBuildConfig
        EngineRuntimeLogging
        ImGuiSDL
    )
    amber_target_folder(EditorOutputLog "Engine/Editor/OutputLog")
    amber_add_header_tree(EditorOutputLog "${CMAKE_CURRENT_SOURCE_DIR}/include")
    amber_group_sources(EditorOutputLog "${CMAKE_CURRENT_SOURCE_DIR}/..")
elseif(BUILD_SDL_MODULES)
    message(WARNING
        "EditorOutputLog target is disabled because ImGuiSDL target is not available."
    )
endif()
