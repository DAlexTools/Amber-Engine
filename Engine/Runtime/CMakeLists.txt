
get_filename_component(ENGINE_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/.. ABSOLUTE)

set(ENGINE_RUNTIME_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(ENGINE_THIRD_PARTY_DIR ${ENGINE_ROOT_DIR}/ThirdParty)
set(ENGINE_PHYSICS_DIR ${ENGINE_RUNTIME_DIR}/Physics)

add_library(EngineRuntimeCore STATIC
    ${ENGINE_RUNTIME_DIR}/Assets/AssetResolver.cpp
    ${ENGINE_RUNTIME_DIR}/Core/Math/MatrixMN.cpp
    ${ENGINE_RUNTIME_DIR}/Core/Math/AmberMath.cpp
    ${ENGINE_RUNTIME_DIR}/Core/Math/VectorN.cpp
    ${ENGINE_RUNTIME_DIR}/Core/Math/Vector2D.cpp
    ${ENGINE_RUNTIME_DIR}/Core/Threading/ThreadPool.cpp
    ${ENGINE_RUNTIME_DIR}/Project/ProjectDescriptor.cpp
    ${ENGINE_RUNTIME_DIR}/Scene/SceneAsset.cpp
)

target_include_directories(EngineRuntimeCore PUBLIC
    ${ENGINE_RUNTIME_DIR}
)

find_package(Threads REQUIRED)
target_link_libraries(EngineRuntimeCore PUBLIC
    AmberBuildConfig
    Threads::Threads
)
amber_target_folder(EngineRuntimeCore "Engine/Runtime/Core")
amber_add_header_tree(EngineRuntimeCore "${ENGINE_RUNTIME_DIR}/Assets")
amber_add_header_tree(EngineRuntimeCore "${ENGINE_RUNTIME_DIR}/Core")
amber_add_header_tree(EngineRuntimeCore "${ENGINE_RUNTIME_DIR}/Project")
amber_add_header_tree(EngineRuntimeCore "${ENGINE_RUNTIME_DIR}/Scene")
amber_group_sources(EngineRuntimeCore "${ENGINE_RUNTIME_DIR}")
amber_enable_pch(EngineRuntimeCore)

add_library(EngineRuntimeLogging STATIC
    ${ENGINE_RUNTIME_DIR}/Logging/LogBus.cpp
    ${ENGINE_RUNTIME_DIR}/Logging/Logger.cpp
)

target_include_directories(EngineRuntimeLogging PUBLIC
    ${ENGINE_RUNTIME_DIR}
)

target_link_libraries(EngineRuntimeLogging PUBLIC
    AmberBuildConfig
)
amber_target_folder(EngineRuntimeLogging "Engine/Runtime/Logging")
amber_add_header_tree(EngineRuntimeLogging "${ENGINE_RUNTIME_DIR}/Logging")
amber_group_sources(EngineRuntimeLogging "${ENGINE_RUNTIME_DIR}")
amber_enable_pch(EngineRuntimeLogging)

add_library(EngineRuntimeECS STATIC
    ${ENGINE_RUNTIME_DIR}/EntityComponentSystem/ECS.cpp
    ${ENGINE_RUNTIME_DIR}/Game/GameModuleInterface.cpp
    ${ENGINE_RUNTIME_DIR}/Scene/Object.cpp
    ${ENGINE_RUNTIME_DIR}/Scene/ObjectFactory.cpp
    ${ENGINE_RUNTIME_DIR}/Scene/PrimitiveObjects.cpp
    ${ENGINE_RUNTIME_DIR}/Scene/SceneObjectComponents.cpp
    ${ENGINE_RUNTIME_DIR}/Scene/SpriteObject.cpp
)

target_include_directories(EngineRuntimeECS PUBLIC
    ${ENGINE_RUNTIME_DIR}
    ${ENGINE_THIRD_PARTY_DIR}
)

target_link_libraries(EngineRuntimeECS PUBLIC
    AmberBuildConfig
    EngineRuntimeCore
    EngineRuntimeLogging
)
amber_target_folder(EngineRuntimeECS "Engine/Runtime/ECS")
amber_add_header_tree(EngineRuntimeECS "${ENGINE_RUNTIME_DIR}/EntityComponentSystem")
amber_add_header_tree(EngineRuntimeECS "${ENGINE_RUNTIME_DIR}/Game")
amber_add_header_tree(EngineRuntimeECS "${ENGINE_RUNTIME_DIR}/Scene")
amber_group_sources(EngineRuntimeECS "${ENGINE_RUNTIME_DIR}")
amber_enable_pch(EngineRuntimeECS)

add_library(Physics STATIC
    ${ENGINE_PHYSICS_DIR}/CollisionDetection.cpp
    ${ENGINE_PHYSICS_DIR}/Contact.cpp
    ${ENGINE_PHYSICS_DIR}/Constraint.cpp
    ${ENGINE_PHYSICS_DIR}/Force.cpp
    ${ENGINE_PHYSICS_DIR}/Mouse.cpp
    ${ENGINE_PHYSICS_DIR}/Particle.cpp
    ${ENGINE_PHYSICS_DIR}/Point.cpp
    ${ENGINE_RUNTIME_DIR}/Classes/World.cpp
    ${ENGINE_PHYSICS_DIR}/Body.cpp
    ${ENGINE_PHYSICS_DIR}/Cloth.cpp
    ${ENGINE_PHYSICS_DIR}/Shape.cpp
    ${ENGINE_PHYSICS_DIR}/Stick.cpp
    ${ENGINE_PHYSICS_DIR}/Utils.cpp
    ${ENGINE_PHYSICS_DIR}/base64.cpp
)

target_include_directories(Physics PUBLIC
    ${ENGINE_RUNTIME_DIR}
    ${ENGINE_PHYSICS_DIR}
)

target_link_libraries(Physics PUBLIC
    AmberBuildConfig
    EngineRuntimeCore
    EngineRuntimeLogging
)
target_sources(Physics PRIVATE
    ${ENGINE_RUNTIME_DIR}/Classes/World.h
)
amber_target_folder(Physics "Engine/Runtime/Physics")
amber_add_header_tree(Physics "${ENGINE_PHYSICS_DIR}")
amber_group_sources(Physics "${ENGINE_RUNTIME_DIR}")
amber_enable_pch(Physics)

add_executable(PhysicsCollisionFilteringCheck
    ${ENGINE_PHYSICS_DIR}/tests/PhysicsCollisionFilteringCheck.cpp
)

target_link_libraries(PhysicsCollisionFilteringCheck PRIVATE
    Physics
)
target_compile_definitions(PhysicsCollisionFilteringCheck PRIVATE SMOKE_TEST=1)
amber_target_folder(PhysicsCollisionFilteringCheck "Engine/Runtime/Physics/Tests")
amber_group_sources(PhysicsCollisionFilteringCheck "${ENGINE_RUNTIME_DIR}")
amber_enable_pch(PhysicsCollisionFilteringCheck)

if(BUILD_SDL_MODULES AND SDL2_TARGET_FOUND)
    add_library(ImGuiSDL STATIC
        ${ENGINE_THIRD_PARTY_DIR}/imgui/imgui.cpp
        ${ENGINE_THIRD_PARTY_DIR}/imgui/imgui_demo.cpp
        ${ENGINE_THIRD_PARTY_DIR}/imgui/imgui_draw.cpp
        ${ENGINE_THIRD_PARTY_DIR}/imgui/imgui_impl_sdl.cpp
        ${ENGINE_THIRD_PARTY_DIR}/imgui/imgui_sdl.cpp
        ${ENGINE_THIRD_PARTY_DIR}/imgui/imgui_tables.cpp
        ${ENGINE_THIRD_PARTY_DIR}/imgui/imgui_widgets.cpp
    )

    target_include_directories(ImGuiSDL PUBLIC
        ${ENGINE_THIRD_PARTY_DIR}
        ${ENGINE_THIRD_PARTY_DIR}/imgui
    )

    target_link_libraries(ImGuiSDL PUBLIC
        AmberBuildConfig
        ${SDL2_LINK_TARGETS}
    )
    amber_target_folder(ImGuiSDL "Engine/ThirdParty/imgui")
    amber_add_header_tree(ImGuiSDL "${ENGINE_THIRD_PARTY_DIR}/imgui")
    amber_group_sources(ImGuiSDL "${ENGINE_THIRD_PARTY_DIR}")
endif()

set(ENGINE_PHYSICS_BRIDGE_HEADERS
    ${ENGINE_RUNTIME_DIR}/EnginePhysicsBridge/EnginePhysicsBridge.h
    ${ENGINE_RUNTIME_DIR}/Components/PhysicsBodyComponent.h
    ${ENGINE_RUNTIME_DIR}/Systems/PhysicsBodyFactory.h
    ${ENGINE_RUNTIME_DIR}/Systems/PhysicsBodyLifecycleSystem.h
    ${ENGINE_RUNTIME_DIR}/Systems/PhysicsCollisionLayers.h
    ${ENGINE_RUNTIME_DIR}/Systems/PhysicsContactEventSystem.h
    ${ENGINE_RUNTIME_DIR}/Systems/PhysicsContactResponseSystem.h
    ${ENGINE_RUNTIME_DIR}/Systems/PhysicsSyncSystem.h
    ${ENGINE_RUNTIME_DIR}/Systems/PhysicsTransformConversions.h
    ${ENGINE_RUNTIME_DIR}/Systems/PhysicsVelocitySystem.h
    ${ENGINE_RUNTIME_DIR}/Systems/PhysicsWorldSystem.h
)

add_library(EnginePhysicsBridge INTERFACE)

target_include_directories(EnginePhysicsBridge INTERFACE
    ${ENGINE_RUNTIME_DIR}
    ${ENGINE_THIRD_PARTY_DIR}
)

target_link_libraries(EnginePhysicsBridge INTERFACE
    AmberBuildConfig
    Physics
)
amber_target_folder(EnginePhysicsBridge "Engine/Runtime/EnginePhysicsBridge")

add_library(EnginePhysicsBridgeCheck STATIC
    ${ENGINE_RUNTIME_DIR}/Systems/PhysicsBridgeCompileCheck.cpp
)
target_sources(EnginePhysicsBridgeCheck PRIVATE
    ${ENGINE_PHYSICS_BRIDGE_HEADERS}
)

target_include_directories(EnginePhysicsBridgeCheck PUBLIC
    ${ENGINE_RUNTIME_DIR}
    ${ENGINE_THIRD_PARTY_DIR}
)

target_link_libraries(EnginePhysicsBridgeCheck PUBLIC
    AmberBuildConfig
    EngineRuntimeLogging
    EngineRuntimeECS
    EnginePhysicsBridge
)
amber_target_folder(EnginePhysicsBridgeCheck "Engine/Runtime/EnginePhysicsBridge")
amber_group_sources(EnginePhysicsBridgeCheck "${ENGINE_RUNTIME_DIR}")
amber_enable_pch(EnginePhysicsBridgeCheck)

if(BUILD_SDL_MODULES AND SDL2_TARGET_FOUND AND SDL2_IMAGE_TARGET_FOUND)
    add_library(EngineRuntimePlayer STATIC
        ${ENGINE_RUNTIME_DIR}/Game/RuntimePlayer.cpp
        ${ENGINE_RUNTIME_DIR}/Game/RuntimeTextureCacheSDL.cpp
        ${ENGINE_RUNTIME_DIR}/Game/RuntimeViewerSDL.cpp
        ${ENGINE_RUNTIME_DIR}/Game/RuntimeWorld.cpp
    )

    target_include_directories(EngineRuntimePlayer PUBLIC
        ${ENGINE_RUNTIME_DIR}
    )

    target_compile_definitions(EngineRuntimePlayer PUBLIC SDL_MAIN_HANDLED)

    target_link_libraries(EngineRuntimePlayer PUBLIC
        AmberBuildConfig
        EngineRuntimeCore
        EngineRuntimeECS
        EngineRuntimeLogging
        ${SDL2_LINK_TARGETS}
        ${SDL2_IMAGE_TARGET}
    )
    amber_target_folder(EngineRuntimePlayer "Engine/Runtime/Game")
    amber_add_header_tree(EngineRuntimePlayer "${ENGINE_RUNTIME_DIR}/Game")
    amber_group_sources(EngineRuntimePlayer "${ENGINE_RUNTIME_DIR}")
    amber_enable_pch(EngineRuntimePlayer)
elseif(BUILD_SDL_MODULES)
    message(WARNING
        "EngineRuntimePlayer target is disabled because SDL2 or SDL2_image was not found."
    )
endif()

if(BUILD_SDL_MODULES)
    if(SDL2_TARGET_FOUND
        AND SDL2_IMAGE_TARGET_FOUND
        AND SDL2_MIXER_TARGET_FOUND
        AND SDL2_TTF_TARGET_FOUND
        AND LUA_PACKAGE_FOUND)
        add_library(Engine STATIC
            ${ENGINE_RUNTIME_DIR}/AssetManager/AssetManager.cpp
            ${ENGINE_RUNTIME_DIR}/Classes/Engine.cpp
            ${ENGINE_RUNTIME_DIR}/FileManager/FileManager.cpp
            ${ENGINE_RUNTIME_DIR}/Game/GameModule.cpp
            ${ENGINE_RUNTIME_DIR}/Game/LevelLoader.cpp
            ${ENGINE_RUNTIME_DIR}/Utilities/Utils.cpp
        )

        target_include_directories(Engine PUBLIC
            ${ENGINE_RUNTIME_DIR}
            ${ENGINE_THIRD_PARTY_DIR}
            ${LUA_INCLUDE_DIRS_EFFECTIVE}
        )

        target_compile_definitions(Engine PRIVATE SMOKE_TEST=1)

        target_link_libraries(Engine PUBLIC
            AmberBuildConfig
            EngineRuntimeCore
            EngineRuntimeECS
            EngineRuntimeLogging
            EnginePhysicsBridge
            ImGuiSDL
            ${SDL2_LINK_TARGETS}
            ${SDL2_IMAGE_TARGET}
            ${SDL2_MIXER_TARGET}
            ${SDL2_TTF_TARGET}
            ${LUA_LINK_TARGETS}
        )
        amber_target_folder(Engine "Engine/Runtime")
        amber_add_header_tree(Engine "${ENGINE_RUNTIME_DIR}/AssetManager")
        amber_add_header_tree(Engine "${ENGINE_RUNTIME_DIR}/Classes")
        amber_add_header_tree(Engine "${ENGINE_RUNTIME_DIR}/Components")
        amber_add_header_tree(Engine "${ENGINE_RUNTIME_DIR}/EntityComponentSystem")
        amber_add_header_tree(Engine "${ENGINE_RUNTIME_DIR}/EventBus")
        amber_add_header_tree(Engine "${ENGINE_RUNTIME_DIR}/Events")
        amber_add_header_tree(Engine "${ENGINE_RUNTIME_DIR}/FileManager")
        amber_add_header_tree(Engine "${ENGINE_RUNTIME_DIR}/Game")
        amber_add_header_tree(Engine "${ENGINE_RUNTIME_DIR}/Systems")
        amber_add_header_tree(Engine "${ENGINE_RUNTIME_DIR}/Utilities")
        amber_group_sources(Engine "${ENGINE_RUNTIME_DIR}")
        amber_enable_pch(Engine)
    else()
        message(WARNING
            "Engine target is disabled because SDL2/SDL2_image/SDL2_mixer/"
            "SDL2_ttf/Lua were not found. Configure with vcpkg or set "
            "BUILD_SDL_MODULES=OFF to build only Physics and EnginePhysicsBridge."
        )
    endif()
endif()
