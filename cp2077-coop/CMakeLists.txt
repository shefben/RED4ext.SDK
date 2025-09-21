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

# Build sqlite3 from source
add_library(sqlite3 STATIC third_party/sqlite/sqlite3.c)
target_include_directories(sqlite3 PUBLIC third_party/sqlite)

file(GLOB ENET_SOURCES third_party/enet/*.c)
add_library(enet STATIC ${ENET_SOURCES})
target_include_directories(enet PUBLIC third_party/enet/include)

# Add lz4 support
find_library(LZ4_LIBRARY NAMES liblz4_static lz4 PATHS
    "${PROJECT_SOURCE_DIR}/third_party/lz4/static"
    "${PROJECT_SOURCE_DIR}/third_party/lz4"
    REQUIRED)
add_library(lz4 STATIC IMPORTED)
set_target_properties(lz4 PROPERTIES IMPORTED_LOCATION "${LZ4_LIBRARY}")
target_include_directories(lz4 INTERFACE third_party/lz4/include)

# Add spdlog support
add_subdirectory(third_party/spdlog)

# Add Codeware support (header-only library)
add_library(codeware INTERFACE)
target_include_directories(codeware INTERFACE
    ../Codeware/lib
    third_party/wil/include)
target_compile_definitions(codeware INTERFACE CODEWARE_HEADER_ONLY)

file(GLOB_RECURSE COOP_SOURCES
    src/core/*.cpp
    src/net/*.cpp
    src/server/*.cpp
    src/runtime/*.cpp
    src/physics/*.cpp
    src/voice/*.cpp
    src/plugin/*.cpp
    src/content/*.cpp
    src/config/*.cpp
    src/performance/*.cpp
    src/database/*.cpp
    src/events/*.cpp
    src/quest/*.cpp
    src/save/*.cpp
    src/scripting/*.cpp
    src/sync/*.cpp
    src/test/*.cpp
    src/ui/*.cpp
)

add_library(cp2077-coop SHARED ${COOP_SOURCES})

target_include_directories(cp2077-coop PRIVATE
    "${PROJECT_BINARY_DIR}/generated"
    "${PROJECT_SOURCE_DIR}/../include"
    third_party/enet/include
    third_party
    third_party/zstd
    third_party/libsodium
    third_party/sqlite
    third_party/curl/include
    third_party/openal/include
    third_party/opus/include
    third_party/openssl/include
    third_party/rapidjson/include
    third_party/nlohmann
    third_party/lz4/lib
    third_party/spdlog/include
    ../Codeware/lib
    ../cp2077-nativedb-master
    ../cp2077-red-data-master
    ../cp2077-red-filesystem-master
    ../cp2077-red-socket-main
    ../cp2077_db_json_dump)

# Define SODIUM_STATIC and CURL_STATICLIB for proper static linking
# Also define NOMINMAX to prevent Windows.h min/max macro conflicts
target_compile_definitions(cp2077-coop PRIVATE SODIUM_STATIC CURL_STATICLIB NOMINMAX WIN32_LEAN_AND_MEAN)

# Include HTTP/3 QUIC libraries to resolve linking errors
target_link_libraries(cp2077-coop PRIVATE enet zstd "${LIBSODIUM_LIBRARY}" sqlite3 juice opus AL::AL lz4 spdlog::spdlog codeware
    "${PROJECT_SOURCE_DIR}/third_party/curl/lib/libssl.a"
    "${PROJECT_SOURCE_DIR}/third_party/curl/lib/libcrypto.a"
    "${PROJECT_SOURCE_DIR}/third_party/curl/lib/libcurl.a"
    "${PROJECT_SOURCE_DIR}/third_party/curl/lib/libz.a"
    "${PROJECT_SOURCE_DIR}/third_party/curl/lib/libbrotlicommon.a"
    "${PROJECT_SOURCE_DIR}/third_party/curl/lib/libbrotlidec.a"
    "${PROJECT_SOURCE_DIR}/third_party/curl/lib/libnghttp2.a"
    "${PROJECT_SOURCE_DIR}/third_party/curl/lib/libssh2.a"
    "${PROJECT_SOURCE_DIR}/third_party/curl/lib/libngtcp2.a"
    "${PROJECT_SOURCE_DIR}/third_party/curl/lib/libnghttp3.a"
    "${PROJECT_SOURCE_DIR}/third_party/curl/lib/libngtcp2_crypto_libressl.a")

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
    target_link_libraries(cp2077-coop PRIVATE crypt32 wldap32 normaliz advapi32 winhttp bcrypt secur32 legacy_stdio_definitions)
    target_link_libraries(coop_dedicated PRIVATE crypt32 wldap32 normaliz advapi32 winhttp bcrypt secur32)
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
