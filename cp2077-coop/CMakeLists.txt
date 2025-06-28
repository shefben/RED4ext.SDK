cmake_minimum_required(VERSION 3.21) project(cp2077 - coop)

    list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

    find_package(Juice REQUIRED)
    find_package(Opus REQUIRED)
    find_package(AL REQUIRED)
    find_package(OpenSSL REQUIRED)
    find_package(Sodium REQUIRED)

    add_library(enet STATIC third_party/enet/enet.c)
    target_include_directories(enet PUBLIC "${PROJECT_SOURCE_DIR}/third_party/enet/include")

#See README for build and packaging instructions.
#This CMake script builds the cp2077 - coop mod and the coop_dedicated server.

#Build the mod's native library.
                add_library(cp2077 -
                            coop SHARED src / net / Net.cpp src / net / Connection.cpp src / net / StatBatch.cpp src /
                                net / NatClient.cpp src / net / WorldMarkers.cpp src / net / PhaseBundle.cpp src /
                                core / GameClock.cpp src / core / SpatialGrid.cpp src / core / SaveFork.cpp src / core /
                                SaveMigration.cpp src / core / Settings.cpp src / core / SessionState.cpp src / core /
                                CrashHandler.cpp src / core / TaskGraph.cpp src / physics / LagComp.cpp src / physics / CarPhysics.cpp src /
                                server / InventoryController.cpp src / server / VendorController.cpp src / server /
                                ApartmentController.cpp src / server / DealerController.cpp src / server /
                                NpcController.cpp src / server / VehicleController.cpp src / server /
                                BreachController.cpp src / server / ShardController.cpp src / server /
                                ElevatorController.cpp src / server / GlobalEventController.cpp src / server /
                                AdminController.cpp src / server / CyberController.cpp src / server /
                                PerkController.cpp src / server / SkillController.cpp src / server /
                                QuestWatchdog.cpp src / server / PhaseGC.cpp src / server /
                                PhaseTriggerController.cpp src / server / TradeController.cpp src / server /
                                SnapshotHeap.cpp src / server / TextureGuard.cpp src / server / Journal.cpp src /
                                server / SectorLODController.cpp src / server / BillboardController.cpp src / server /
                                DoorBreachController.cpp src / server / QuestGadgetController.cpp src / server /
                                TransitController.cpp src / server / CameraController.cpp src / server /
                                CarryController.cpp src / server / GrenadeController.cpp src / server /
                                SmartCamController.cpp src / server / ArcadeController.cpp src / server / PoliceDispatch.cpp src / server /
                                LedgerService.cpp src / server / WebDash.cpp src / voice / VoiceEncoder.cpp src /
                                voice / VoiceDecoder.cpp)

                    target_include_directories(cp2077 - coop PRIVATE "${PROJECT_SOURCE_DIR}/third_party/enet/include"
                                                                     "${PROJECT_SOURCE_DIR}/third_party"
                                                                     "${PROJECT_SOURCE_DIR}/third_party/zstd")

                        target_link_libraries(cp2077 - coop PRIVATE enet juice opus AL::AL OpenSSL::SSL sodium)

                            set(BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build") file(MAKE_DIRECTORY "${BUILD_DIR}")

                                set(ARCHIVE "${BUILD_DIR}/cp2077-coop.archive")
                                    add_custom_command(OUTPUT "${ARCHIVE}" COMMAND ${CMAKE_COMMAND} - E touch
                                                       "${ARCHIVE}" COMMENT "Creating empty archive")

                                        add_executable(coop_dedicated src/server/DedicatedMain.cpp
                                                       src/server/DamageValidator.cpp
                                                       src/server/ServerConfig.cpp
                                                       src/server/Heartbeat.cpp
                                                       src/server/AdminController.cpp
                                                       src/plugin/PythonVM.cpp
                                                       src/plugin/PluginManager.cpp)

                                            target_include_directories(coop_dedicated PRIVATE
                                                                       "${PROJECT_SOURCE_DIR}/third_party"
                                                                       "${PROJECT_SOURCE_DIR}/third_party/cpython-3.11/Include")

                                                target_link_libraries(coop_dedicated PRIVATE cp2077 - coop enet juice
                                                                                ${PROJECT_SOURCE_DIR}/third_party/cpython-3.11/libpython3.11.a)
                                                target_compile_definitions(coop_dedicated PRIVATE Py_ENABLE_SHARED=0)

                                                    add_executable(coop_merge tools / coop_merge.cpp)

                                                        target_link_libraries(coop_merge PRIVATE cp2077 - coop)

                                                            add_custom_target(cp2077 - coop -
                                                                              archive ALL DEPENDS "${ARCHIVE}" cp2077 -
                                                                              coop)
