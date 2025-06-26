cmake_minimum_required(VERSION 3.21)
project(cp2077-coop)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

find_package(Juice REQUIRED)
find_package(Opus REQUIRED)
find_package(AL REQUIRED)
find_package(OpenSSL REQUIRED)

# See README for build and packaging instructions.
# This CMake script builds the cp2077-coop mod and the coop_dedicated server.

# Build the mod's native library.
add_library(cp2077-coop SHARED
    src/net/Net.cpp
    src/net/Connection.cpp
    src/net/StatBatch.cpp
    src/net/NatClient.cpp
    src/core/GameClock.cpp
    src/core/SpatialGrid.cpp
    src/core/SaveFork.cpp
    src/core/SaveMigration.cpp
    src/core/Settings.cpp
    src/core/SessionState.cpp
    src/core/CrashHandler.cpp
    src/physics/LagComp.cpp
    src/physics/CarPhysics.cpp
    src/server/InventoryController.cpp
    src/server/VendorController.cpp
    src/server/NpcController.cpp
    src/server/VehicleController.cpp
    src/server/BreachController.cpp
    src/server/ElevatorController.cpp
    src/server/GlobalEventController.cpp
    src/server/AdminController.cpp
    src/server/LedgerService.cpp
    src/server/WebDash.cpp
    src/voice/VoiceEncoder.cpp
    src/voice/VoiceDecoder.cpp
)

target_include_directories(cp2077-coop
    PRIVATE
        "${PROJECT_SOURCE_DIR}/third_party/enet/include"
        "${PROJECT_SOURCE_DIR}/third_party"
)

target_link_libraries(cp2077-coop PRIVATE enet juice opus AL::AL OpenSSL::SSL)

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
    src/server/AdminController.cpp
)

target_include_directories(coop_dedicated PRIVATE "${PROJECT_SOURCE_DIR}/third_party")

target_link_libraries(coop_dedicated PRIVATE cp2077-coop enet juice)

add_executable(coop_merge
    tools/coop_merge.cpp
)

target_link_libraries(coop_merge PRIVATE cp2077-coop)

add_custom_target(cp2077-coop-archive ALL DEPENDS "${ARCHIVE}" cp2077-coop)
