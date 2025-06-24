cmake_minimum_required(VERSION 3.21)
project(cp2077-coop)

# See README for build and packaging instructions.
# This CMake script builds the cp2077-coop mod and the coop_dedicated server.

# Build the mod's native library.
add_library(cp2077-coop SHARED
    src/net/Net.cpp
    src/net/Connection.cpp
    src/net/StatBatch.cpp
    src/core/GameClock.cpp
    src/core/SaveFork.cpp
    src/core/CrashHandler.cpp
    src/physics/LagComp.cpp
    src/physics/CarPhysics.cpp
    src/server/NpcController.cpp
)

target_include_directories(cp2077-coop
    PRIVATE
        "${PROJECT_SOURCE_DIR}/third_party/enet/include"
)

target_link_libraries(cp2077-coop PRIVATE enet)

set(BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build")
file(MAKE_DIRECTORY "${BUILD_DIR}")

set(ARCHIVE "${BUILD_DIR}/cp2077-coop.archive")
add_custom_command(
    OUTPUT "${ARCHIVE}"
    COMMAND ${CMAKE_COMMAND} -E touch "${ARCHIVE}"
    COMMENT "Creating empty archive"
)

add_executable(coop_dedicated
    src/server/DedicatedMain.cpp
    src/server/DamageValidator.cpp
    src/server/Heartbeat.cpp
)

target_link_libraries(coop_dedicated PRIVATE cp2077-coop enet)

add_custom_target(cp2077-coop-archive ALL DEPENDS "${ARCHIVE}" cp2077-coop)

