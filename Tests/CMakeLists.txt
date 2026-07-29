set(PHYSICS_UNIT_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Physics/Vector2DTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Physics/WorldCollisionTests.cpp
)

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

add_executable(EngineUnitTests
    ${CMAKE_CURRENT_SOURCE_DIR}/Engine/AssetResolverTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Engine/LogBusTests.cpp
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
target_compile_definitions(EngineUnitTests PRIVATE C_UNIT_TEST=1)

gtest_discover_tests(EngineUnitTests
    DISCOVERY_MODE PRE_TEST
    PROPERTIES
        LABELS "unit;engine"
)
