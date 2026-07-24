add_library(Physics STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CollisionDetection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Contact.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Constraint.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Force.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Mouse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Particle.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Point.cpp
    ${ENGINE_RUNTIME_DIR}/Classes/World.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/Body.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/Cloth.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/Shape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/Stick.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Utils/Utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Utils/base64.cpp
)

target_include_directories(Physics PUBLIC
    ${ENGINE_RUNTIME_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Physics
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
amber_add_header_tree(Physics "${CMAKE_CURRENT_SOURCE_DIR}/include")
amber_group_sources(Physics "${ENGINE_RUNTIME_DIR}")

add_executable(PhysicsCollisionFilteringCheck
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/PhysicsCollisionFilteringCheck.cpp
)

target_link_libraries(PhysicsCollisionFilteringCheck PRIVATE
    Physics
)
target_compile_definitions(PhysicsCollisionFilteringCheck PRIVATE SMOKE_TEST=1)
amber_target_folder(PhysicsCollisionFilteringCheck "Engine/Runtime/Physics/Tests")
amber_group_sources(PhysicsCollisionFilteringCheck "${ENGINE_RUNTIME_DIR}")
