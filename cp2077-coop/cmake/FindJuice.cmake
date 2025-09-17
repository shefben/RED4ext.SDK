# FindJuice.cmake - Find the Juice library

find_path(JUICE_INCLUDE_DIR 
    NAMES juice.h
    PATHS "${PROJECT_SOURCE_DIR}/third_party/juice/include"
    NO_DEFAULT_PATH
)

find_library(JUICE_LIBRARY
    NAMES juice libjuice
    PATHS "${PROJECT_SOURCE_DIR}/third_party/juice/lib"
    NO_DEFAULT_PATH
)

if(JUICE_INCLUDE_DIR AND JUICE_LIBRARY)
    set(JUICE_FOUND TRUE)
else()
    # If not found in lib/, build from source
    if(EXISTS "${PROJECT_SOURCE_DIR}/third_party/juice/CMakeLists.txt")
        add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/juice juice EXCLUDE_FROM_ALL)
        set(JUICE_FOUND TRUE)
        set(JUICE_LIBRARY juice)
        set(JUICE_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/third_party/juice/include")
    else()
        set(JUICE_FOUND FALSE)
    endif()
endif()

if(JUICE_FOUND)
    if(NOT TARGET Juice::Juice)
        if(TARGET juice)
            # Use the built target
            add_library(Juice::Juice ALIAS juice)
        else()
            add_library(Juice::Juice UNKNOWN IMPORTED)
            set_target_properties(Juice::Juice PROPERTIES
                IMPORTED_LOCATION "${JUICE_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${JUICE_INCLUDE_DIR}"
            )
        endif()
    endif()
endif()

mark_as_advanced(JUICE_INCLUDE_DIR JUICE_LIBRARY)