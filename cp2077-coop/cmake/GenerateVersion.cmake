# Generate version information from Git and build environment
function(generate_version_info)
    # Get Git hash if available
    find_package(Git QUIET)
    if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        
        # Get commit count for build number
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_COMMIT_COUNT
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
    else()
        set(GIT_HASH "unknown")
        set(GIT_COMMIT_COUNT "0")
    endif()
    
    # Generate timestamp
    string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S UTC" UTC)
    
    # Create version header with generated values
    set(VERSION_HEADER_CONTENT "
// Auto-generated version information - DO NOT EDIT
#pragma once
#include <cstdint>

namespace CoopNet
{
    // Version constants
    constexpr uint32_t VERSION_MAJOR = 1;
    constexpr uint32_t VERSION_MINOR = 0;
    constexpr uint32_t VERSION_PATCH = 0;
    constexpr uint32_t VERSION_BUILD = ${GIT_COMMIT_COUNT};
    
    // Build information
    constexpr const char* GIT_HASH = \"${GIT_HASH}\";
    constexpr const char* BUILD_DATE = \"${BUILD_TIMESTAMP}\";
    
    // Generate CRC at compile time
    constexpr uint32_t FNV1A_OFFSET = 2166136261u;
    constexpr uint32_t FNV1A_PRIME = 16777619u;
    
    constexpr uint32_t fnv1a_hash(const char* str, uint32_t hash = FNV1A_OFFSET)
    {
        return *str ? fnv1a_hash(str + 1, (hash ^ *str) * FNV1A_PRIME) : hash;
    }
    
    constexpr uint32_t VERSION_CRC = fnv1a_hash(\"1.0.0-${GIT_HASH}\");
}
")
    
    # Write the generated header
    file(WRITE "${PROJECT_BINARY_DIR}/generated/VersionGenerated.hpp" "${VERSION_HEADER_CONTENT}")
    
    # Set variables for parent scope
    set(COOP_VERSION_MAJOR 1 PARENT_SCOPE)
    set(COOP_VERSION_MINOR 0 PARENT_SCOPE)
    set(COOP_VERSION_PATCH 0 PARENT_SCOPE)
    set(COOP_VERSION_BUILD ${GIT_COMMIT_COUNT} PARENT_SCOPE)
    set(COOP_GIT_HASH ${GIT_HASH} PARENT_SCOPE)
    set(COOP_BUILD_DATE "${BUILD_TIMESTAMP}" PARENT_SCOPE)
    
    message(STATUS "Generated version: 1.0.0.${GIT_COMMIT_COUNT} (${GIT_HASH})")
endfunction()