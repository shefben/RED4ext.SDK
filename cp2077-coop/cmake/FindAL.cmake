find_path(AL_INCLUDE_DIR al.h PATH_SUFFIXES AL include/AL)
find_library(AL_LIBRARY NAMES openal OpenAL32)
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
