cmake_minimum_required(VERSION 3.21)
project(cp2077-coop)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

find_package(Juice REQUIRED)
find_package(Opus REQUIRED)
find_package(AL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

find_library(ZSTD_LIBRARY NAMES zstd PATHS "${PROJECT_SOURCE_DIR}/third_party/zstd" REQUIRED)
add_library(zstd STATIC IMPORTED)
set_target_properties(zstd PROPERTIES IMPORTED_LOCATION "${ZSTD_LIBRARY}")
target_include_directories(zstd INTERFACE third_party/zstd)

find_library(LIBSODIUM_LIBRARY NAMES sodium libsodium PATHS "${PROJECT_SOURCE_DIR}/third_party/libsodium" REQUIRED)
add_library(libsodium STATIC IMPORTED)
set_target_properties(libsodium PROPERTIES IMPORTED_LOCATION "${LIBSODIUM_LIBRARY}")
target_include_directories(libsodium INTERFACE third_party/libsodium)

file(GLOB ENET_SOURCES third_party/enet/*.c)
add_library(enet STATIC ${ENET_SOURCES})
target_include_directories(enet PUBLIC third_party/enet/include)

file(GLOB_RECURSE COOP_SOURCES
    src/core/*.cpp
    src/net/*.cpp
    src/server/*.cpp
    src/physics/*.cpp
    src/voice/*.cpp
)

add_library(cp2077-coop SHARED ${COOP_SOURCES})

target_include_directories(cp2077-coop PRIVATE
    third_party/enet/include
    third_party
    third_party/zstd
    third_party/libsodium)

target_link_libraries(cp2077-coop PRIVATE enet zstd libsodium juice opus AL::AL OpenSSL::SSL CURL::libcurl)

add_executable(coop_dedicated src/server/DedicatedMain.cpp)
target_link_libraries(coop_dedicated PRIVATE cp2077-coop enet zstd libsodium juice)


set(CPYTHON_DIR "${PROJECT_SOURCE_DIR}/third_party/cpython-3.11")
if(EXISTS "${CPYTHON_DIR}/Include")
    target_include_directories(cp2077-coop PRIVATE "${CPYTHON_DIR}/Include")
    set(PY_LIB_PATH "${CPYTHON_DIR}/libpython3.11.a")
    if(EXISTS ${PY_LIB_PATH})
        target_link_libraries(cp2077-coop PRIVATE ${PY_LIB_PATH})
    endif()
else()
    find_package(Python3 COMPONENTS Development)
    if(Python3_FOUND)
        target_include_directories(cp2077-coop PRIVATE ${Python3_INCLUDE_DIRS})
        target_link_libraries(cp2077-coop PRIVATE ${Python3_LIBRARIES})
    endif()
endif()
