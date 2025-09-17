# FindOpus.cmake - Find the Opus library

find_path(OPUS_INCLUDE_DIR 
    NAMES opus.h
    PATHS "${PROJECT_SOURCE_DIR}/third_party/opus/include"
          "${PROJECT_SOURCE_DIR}/third_party/opus/include/opus"
    NO_DEFAULT_PATH
)

find_library(OPUS_LIBRARY
    NAMES opus libopus
    PATHS "${PROJECT_SOURCE_DIR}/third_party/opus/lib"
          "${PROJECT_SOURCE_DIR}/third_party/opus"
    NO_DEFAULT_PATH
)

if(OPUS_INCLUDE_DIR AND OPUS_LIBRARY)
    set(OPUS_FOUND TRUE)
else()
    # Check if we can build from source
    if(EXISTS "${PROJECT_SOURCE_DIR}/third_party/opus/CMakeLists.txt")
        add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/opus opus EXCLUDE_FROM_ALL)
        set(OPUS_FOUND TRUE)
        set(OPUS_LIBRARY opus)
        set(OPUS_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/third_party/opus/include")
    else()
        set(OPUS_FOUND FALSE)
    endif()
endif()

if(OPUS_FOUND)
    if(NOT TARGET Opus::opus)
        add_library(Opus::opus UNKNOWN IMPORTED)
        if(TARGET opus)
            add_library(Opus::opus ALIAS opus)
        else()
            set_target_properties(Opus::opus PROPERTIES
                IMPORTED_LOCATION "${OPUS_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${OPUS_INCLUDE_DIR}"
            )
        endif()
    endif()
endif()

mark_as_advanced(OPUS_INCLUDE_DIR OPUS_LIBRARY)