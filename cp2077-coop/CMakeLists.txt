cmake_minimum_required(VERSION 3.21)
project(cp2077-coop)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Generate version information
include(GenerateVersion)
generate_version_info()

# Create generated directory
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/generated")

add_subdirectory(third_party/juice)
find_package(Opus REQUIRED)
find_package(AL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

find_library(ZSTD_LIBRARY NAMES libzstd_static zstd libzstd PATHS 
    "${PROJECT_SOURCE_DIR}/third_party/zstd/static"
    "${PROJECT_SOURCE_DIR}/third_party/zstd/dll"
    "${PROJECT_SOURCE_DIR}/third_party/zstd" 
    REQUIRED)
add_library(zstd STATIC IMPORTED)
set_target_properties(zstd PROPERTIES IMPORTED_LOCATION "${ZSTD_LIBRARY}")
target_include_directories(zstd INTERFACE third_party/zstd)

find_library(LIBSODIUM_LIBRARY NAMES libsodium sodium PATHS 
    "${PROJECT_SOURCE_DIR}/third_party/libsodium/x64/Release/v143/static"
    "${PROJECT_SOURCE_DIR}/third_party/libsodium/x64/Release/v142/static"
    "${PROJECT_SOURCE_DIR}/third_party/libsodium/x64/Debug/v143/static"
    "${PROJECT_SOURCE_DIR}/third_party/libsodium/x64/Debug/v142/static"
    "${PROJECT_SOURCE_DIR}/third_party/libsodium" 
    REQUIRED)
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
    src/runtime/*.cpp
    src/physics/*.cpp
    src/voice/*.cpp
    src/plugin/*.cpp
)

add_library(cp2077-coop SHARED ${COOP_SOURCES})

target_include_directories(cp2077-coop PRIVATE
    "${PROJECT_BINARY_DIR}/generated"
    "${PROJECT_SOURCE_DIR}/../include"
    third_party/enet/include
    third_party
    third_party/zstd
    third_party/libsodium
    third_party/curl/include
    third_party/openal/include
    third_party/opus/include
    third_party/openssl/include
    third_party/rapidjson/include)

# Define SODIUM_STATIC for proper static linking
target_compile_definitions(cp2077-coop PRIVATE SODIUM_STATIC)

target_link_libraries(cp2077-coop PRIVATE enet zstd "${LIBSODIUM_LIBRARY}" juice opus AL::AL OpenSSL::SSL OpenSSL::Crypto 
    "${PROJECT_SOURCE_DIR}/third_party/curl/lib/libcurl.a"
    "${PROJECT_SOURCE_DIR}/third_party/curl/lib/libz.a"
    "${PROJECT_SOURCE_DIR}/third_party/curl/lib/libbrotlicommon.a"
    "${PROJECT_SOURCE_DIR}/third_party/curl/lib/libbrotlidec.a"
    "${PROJECT_SOURCE_DIR}/third_party/curl/lib/libnghttp2.a"
    "${PROJECT_SOURCE_DIR}/third_party/curl/lib/libssh2.a")

add_executable(coop_dedicated src/server/DedicatedMain.cpp)
target_link_libraries(coop_dedicated PRIVATE cp2077-coop enet zstd libsodium juice)

if (WIN32)
    target_link_libraries(cp2077-coop PRIVATE Ws2_32)
    target_link_libraries(coop_dedicated PRIVATE Ws2_32)
    target_link_libraries(cp2077-coop PRIVATE Psapi)
    target_link_libraries(coop_dedicated PRIVATE Psapi)
    target_link_libraries(cp2077-coop PRIVATE winmm)  # For timeGetTime, timeBeginPeriod, timeEndPeriod
    target_link_libraries(coop_dedicated PRIVATE winmm)
    # Additional Windows libraries for curl, crypto, and certificates
    target_link_libraries(cp2077-coop PRIVATE crypt32 wldap32 normaliz advapi32 winhttp)
    target_link_libraries(coop_dedicated PRIVATE crypt32 wldap32 normaliz advapi32 winhttp)
    # ___chkstk_ms symbol is now provided by WindowsCompat.cpp
endif()


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
