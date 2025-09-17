# FindCURL.cmake - Find the CURL library

find_path(CURL_INCLUDE_DIR 
    NAMES curl/curl.h
    PATHS "${PROJECT_SOURCE_DIR}/third_party/curl/include"
    NO_DEFAULT_PATH
)

find_library(CURL_LIBRARY
    NAMES libcurl curl
    PATHS "${PROJECT_SOURCE_DIR}/third_party/curl/lib"
          "${PROJECT_SOURCE_DIR}/third_party/curl"
    NO_DEFAULT_PATH
)

if(CURL_INCLUDE_DIR AND CURL_LIBRARY)
    set(CURL_FOUND TRUE)
else()
    set(CURL_FOUND FALSE)
endif()

if(CURL_FOUND)
    if(NOT TARGET CURL::libcurl)
        add_library(CURL::libcurl UNKNOWN IMPORTED)
        set_target_properties(CURL::libcurl PROPERTIES
            IMPORTED_LOCATION "${CURL_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${CURL_INCLUDE_DIR}"
        )
    endif()
endif()

mark_as_advanced(CURL_INCLUDE_DIR CURL_LIBRARY)