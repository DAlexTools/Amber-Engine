set(PHYSICS_DEMOS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/PhysicsDemos)
set(GAMES_DEMOS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/GamesDemos)
set(PLATFORMER2_DIR ${GAMES_DEMOS_DIR}/Platformer2)
set(CONTAINER_SANDBOX_DIR ${PHYSICS_DEMOS_DIR}/ContainerSandbox)
set(PHYSICS_LAB_DIR ${PHYSICS_DEMOS_DIR}/PhysicsLab)

if(SDL2_TARGET_FOUND)
    if(SDL2_IMAGE_TARGET_FOUND)
        add_executable(Platformer2App
            ${PLATFORMER2_DIR}/main.cpp
            ${PLATFORMER2_DIR}/Platformer2App.cpp
        )

        target_compile_definitions(Platformer2App PRIVATE SDL_MAIN_HANDLED SMOKE_TEST=1)

        target_include_directories(Platformer2App PRIVATE
            ${PLATFORMER2_DIR}
        )

        target_link_libraries(Platformer2App PRIVATE
            AmberBuildConfig
            ${SDL2_LINK_TARGETS}
            ${SDL2_IMAGE_TARGET}
        )
        if(TARGET ImGuiSDL)
            target_compile_definitions(Platformer2App PRIVATE AMBER_ENABLE_PLATFORMER2_EDITOR)
            target_link_libraries(Platformer2App PRIVATE ImGuiSDL)
        endif()
        amber_target_folder(Platformer2App "Samples/GamesDemos/Platformer2")
        amber_add_header_tree(Platformer2App "${PLATFORMER2_DIR}")
        amber_group_sources(Platformer2App "${CMAKE_CURRENT_SOURCE_DIR}")
    else()
        message(WARNING
            "Platformer2App target is disabled because SDL2_image was not found."
        )
    endif()

    add_executable(ContainerSandboxApp
        ${CONTAINER_SANDBOX_DIR}/main.cpp
        ${CONTAINER_SANDBOX_DIR}/ContainerSandboxApp.cpp
    )

    target_compile_definitions(ContainerSandboxApp PRIVATE SDL_MAIN_HANDLED SMOKE_TEST=1)

    target_include_directories(ContainerSandboxApp PRIVATE
        ${CONTAINER_SANDBOX_DIR}
    )

    target_link_libraries(ContainerSandboxApp PRIVATE
        AmberBuildConfig
        Physics
        ${SDL2_LINK_TARGETS}
    )
    if(TARGET EditorDiagnostics)
        target_compile_definitions(ContainerSandboxApp PRIVATE AMBER_ENABLE_SAMPLE_DIAGNOSTICS)
        target_link_libraries(ContainerSandboxApp PRIVATE EditorDiagnostics)
    endif()
    amber_target_folder(ContainerSandboxApp "Samples/PhysicsDemos/ContainerSandbox")
    amber_add_header_tree(ContainerSandboxApp "${CONTAINER_SANDBOX_DIR}")
    amber_group_sources(ContainerSandboxApp "${CMAKE_CURRENT_SOURCE_DIR}")

    if(TARGET ImGuiSDL)
        add_executable(PhysicsLabApp
            ${PHYSICS_LAB_DIR}/main.cpp
            ${PHYSICS_LAB_DIR}/PhysicsLabApp.cpp
        )

        target_compile_definitions(PhysicsLabApp PRIVATE SDL_MAIN_HANDLED SMOKE_TEST=1)

        target_include_directories(PhysicsLabApp PRIVATE
            ${PHYSICS_LAB_DIR}
        )

        target_link_libraries(PhysicsLabApp PRIVATE
            AmberBuildConfig
            Physics
            ImGuiSDL
            ${SDL2_LINK_TARGETS}
        )
        if(TARGET EditorOutputLog)
            target_compile_definitions(PhysicsLabApp PRIVATE AMBER_ENABLE_PHYSICS_LAB_OUTPUT_LOG)
            target_link_libraries(PhysicsLabApp PRIVATE EditorOutputLog)
        endif()
        amber_target_folder(PhysicsLabApp "Samples/PhysicsDemos/PhysicsLab")
        amber_add_header_tree(PhysicsLabApp "${PHYSICS_LAB_DIR}")
        amber_group_sources(PhysicsLabApp "${CMAKE_CURRENT_SOURCE_DIR}")
    else()
        message(WARNING
            "PhysicsLabApp target is disabled because ImGuiSDL target is not available."
        )
    endif()
else()
    message(WARNING
        "Platformer2App, ContainerSandboxApp and PhysicsLabApp targets are disabled because SDL2 was not found."
    )
endif()

if(SDL2_TARGET_FOUND AND SDL2_GFX_TARGET_FOUND)
    add_library(RendererSDL STATIC
        ${PHYSICS_DEMOS_DIR}/Common/LegacyDiagnosticsOverlay.cpp
        ${PHYSICS_DEMOS_DIR}/Renderer/SDL/Graphics.cpp
    )

    target_include_directories(RendererSDL PUBLIC
        ${PHYSICS_DEMOS_DIR}
        ${PHYSICS_DEMOS_DIR}/Renderer/SDL
    )

    target_link_libraries(RendererSDL PUBLIC
        AmberBuildConfig
        Physics
        ${SDL2_LINK_TARGETS}
        ${SDL2_GFX_TARGET}
    )
    amber_target_folder(RendererSDL "Samples/PhysicsDemos/Renderer/SDL")
    amber_add_header_tree(RendererSDL "${PHYSICS_DEMOS_DIR}/Common")
    amber_add_header_tree(RendererSDL "${PHYSICS_DEMOS_DIR}/Renderer/SDL")
    amber_group_sources(RendererSDL "${PHYSICS_DEMOS_DIR}")

    if(SDL2_IMAGE_TARGET_FOUND)
        function(add_physics_sample target_name sample_folder)
            add_executable(${target_name} ${ARGN})
            target_compile_definitions(${target_name} PRIVATE SDL_MAIN_HANDLED)
            target_include_directories(${target_name} PRIVATE
                ${PHYSICS_DEMOS_DIR}
            )
            target_link_libraries(${target_name} PRIVATE
                AmberBuildConfig
                Physics
                RendererSDL
                ${SDL2_IMAGE_TARGET}
            )
            amber_target_folder(${target_name} "Samples/PhysicsDemos/${sample_folder}")
            amber_add_header_tree(${target_name} "${PHYSICS_DEMOS_DIR}/${sample_folder}")
            amber_group_sources(${target_name} "${PHYSICS_DEMOS_DIR}")
        endfunction()

        add_physics_sample(AngryApp AngryApp
            ${PHYSICS_DEMOS_DIR}/AngryApp/Main.cpp
            ${PHYSICS_DEMOS_DIR}/AngryApp/Application.cpp
        )

        add_physics_sample(RagdollApp Ragdoll
            ${PHYSICS_DEMOS_DIR}/Ragdoll/Main.cpp
            ${PHYSICS_DEMOS_DIR}/Ragdoll/Application.cpp
        )

        add_physics_sample(ChainApp Chain
            ${PHYSICS_DEMOS_DIR}/Chain/Main.cpp
            ${PHYSICS_DEMOS_DIR}/Chain/Application.cpp
        )

        add_physics_sample(SoftBodyApp SoftBody
            ${PHYSICS_DEMOS_DIR}/SoftBody/Main.cpp
            ${PHYSICS_DEMOS_DIR}/SoftBody/Application.cpp
        )

        add_physics_sample(FabricSimulationApp FabricSimulation
            ${PHYSICS_DEMOS_DIR}/FabricSimulation/main.cpp
            ${PHYSICS_DEMOS_DIR}/FabricSimulation/ClothApplication.cpp
        )
    else()
        message(WARNING
            "Physics sample targets are disabled because SDL2_image was not found."
        )
    endif()
else()
    message(WARNING
        "RendererSDL and physics sample targets are disabled because SDL2 or SDL2_gfx was not found."
    )
endif()

if(TARGET Engine)
    add_executable(GameEngineApp
        ${GAMES_DEMOS_DIR}/Game/main.cpp
    )

    target_link_libraries(GameEngineApp PRIVATE
        AmberBuildConfig
        Engine
    )
    target_compile_definitions(GameEngineApp PRIVATE SMOKE_TEST=1)
    amber_target_folder(GameEngineApp "Samples/GamesDemos/Game")
    amber_add_header_tree(GameEngineApp "${GAMES_DEMOS_DIR}/Game")
    amber_group_sources(GameEngineApp "${CMAKE_CURRENT_SOURCE_DIR}")
else()
    message(WARNING
        "GameEngineApp target is disabled because Engine target is not available."
    )
endif()
