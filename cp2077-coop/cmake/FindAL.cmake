find_path(AL_INCLUDE_DIR 
    NAMES al.h
    PATHS "${PROJECT_SOURCE_DIR}/third_party/openal/include"
    PATH_SUFFIXES AL include/AL
    NO_DEFAULT_PATH
)

find_library(AL_LIBRARY 
    NAMES OpenAL32 openal libopenal
    PATHS "${PROJECT_SOURCE_DIR}/third_party/openal/libs/Win64"
          "${PROJECT_SOURCE_DIR}/third_party/openal/libs/Win32"
          "${PROJECT_SOURCE_DIR}/third_party/openal/lib"
          "${PROJECT_SOURCE_DIR}/third_party/openal"
    NO_DEFAULT_PATH
)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AL DEFAULT_MSG AL_LIBRARY AL_INCLUDE_DIR)
if(AL_FOUND)
    if(NOT TARGET AL::AL)
        add_library(AL::AL UNKNOWN IMPORTED)
        set_target_properties(AL::AL PROPERTIES
            IMPORTED_LOCATION "${AL_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${AL_INCLUDE_DIR}")
    endif()
endif()
