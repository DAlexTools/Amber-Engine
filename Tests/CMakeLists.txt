set(PHYSICS_UNIT_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Physics/Vector2DTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Physics/WorldCollisionTests.cpp
)

file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" AMBER_TEST_SOURCE_ROOT)

add_executable(PhysicsUnitTests
    ${PHYSICS_UNIT_TEST_SOURCES}
)
amber_target_folder(PhysicsUnitTests "Tests/Physics")
amber_group_sources(PhysicsUnitTests "${CMAKE_CURRENT_SOURCE_DIR}")
amber_enable_pch(PhysicsUnitTests)

target_link_libraries(PhysicsUnitTests PRIVATE
    AmberBuildConfig
    Physics
    GTest::gtest
    GTest::gtest_main
)
target_compile_definitions(PhysicsUnitTests PRIVATE C_UNIT_TEST=1)

include(GoogleTest)
gtest_discover_tests(PhysicsUnitTests
    DISCOVERY_MODE PRE_TEST
    PROPERTIES
        LABELS "unit;physics"
)

set(ENGINE_UNIT_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Engine/AssetResolverTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Engine/LogBusTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Engine/SceneAssetTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Engine/ThreadPoolTests.cpp
)
if(TARGET EngineRuntimePlayer)
    list(APPEND ENGINE_UNIT_TEST_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/RuntimeRendererConfigTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/RuntimeWorldTests.cpp
    )
endif()

add_executable(EngineUnitTests
    ${ENGINE_UNIT_TEST_SOURCES}
)
amber_target_folder(EngineUnitTests "Tests/Engine")
amber_group_sources(EngineUnitTests "${CMAKE_CURRENT_SOURCE_DIR}")
amber_enable_pch(EngineUnitTests)

target_link_libraries(EngineUnitTests PRIVATE
    AmberBuildConfig
    EngineRuntimeCore
    EngineRuntimeLogging
    Physics
    GTest::gtest
    GTest::gtest_main
)
if(TARGET EngineRuntimePlayer)
    target_link_libraries(EngineUnitTests PRIVATE EngineRuntimePlayer)
endif()
target_compile_definitions(EngineUnitTests PRIVATE C_UNIT_TEST=1)

gtest_discover_tests(EngineUnitTests
    DISCOVERY_MODE PRE_TEST
    PROPERTIES
        LABELS "unit;engine"
)

if(TARGET AmberEditorCore)
    set(EDITOR_UNIT_TEST_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/Editor/ActorTypeRegistryTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Editor/RuntimeParityTests.cpp
    )
    if(TARGET PlatformerGameModule)
        list(APPEND EDITOR_UNIT_TEST_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/Editor/PlatformerPIETests.cpp
        )
    endif()

    add_executable(EditorUnitTests
        ${EDITOR_UNIT_TEST_SOURCES}
    )
    amber_target_folder(EditorUnitTests "Tests/Editor")
    amber_group_sources(EditorUnitTests "${CMAKE_CURRENT_SOURCE_DIR}")
    amber_enable_pch(EditorUnitTests)

    target_link_libraries(EditorUnitTests PRIVATE
        AmberBuildConfig
        AmberEditorCore
        GTest::gtest
        GTest::gtest_main
    )
    if(TARGET PlatformerGameModule)
        target_link_libraries(EditorUnitTests PRIVATE PlatformerGameModule)
    endif()
    target_compile_definitions(EditorUnitTests PRIVATE
        C_UNIT_TEST=1
        AMBER_TEST_SOURCE_ROOT="${AMBER_TEST_SOURCE_ROOT}"
    )

    gtest_discover_tests(EditorUnitTests
        DISCOVERY_MODE PRE_TEST
        PROPERTIES
            LABELS "unit;editor;parity"
    )
endif()
