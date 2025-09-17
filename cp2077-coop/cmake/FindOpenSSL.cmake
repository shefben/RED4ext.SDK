# FindOpenSSL.cmake - Find the OpenSSL library

find_path(OPENSSL_INCLUDE_DIR 
    NAMES openssl/ssl.h
    PATHS "${PROJECT_SOURCE_DIR}/third_party/openssl/include"
    NO_DEFAULT_PATH
)

find_library(OPENSSL_SSL_LIBRARY
    NAMES libssl_static libssl ssl
    PATHS "${PROJECT_SOURCE_DIR}/third_party/openssl/lib/VC/x64/MD"
          "${PROJECT_SOURCE_DIR}/third_party/openssl/lib/VC/x64/MT" 
          "${PROJECT_SOURCE_DIR}/third_party/openssl/lib"
          "${PROJECT_SOURCE_DIR}/third_party/openssl"
    NO_DEFAULT_PATH
)

find_library(OPENSSL_CRYPTO_LIBRARY
    NAMES libcrypto_static libcrypto crypto
    PATHS "${PROJECT_SOURCE_DIR}/third_party/openssl/lib/VC/x64/MD"
          "${PROJECT_SOURCE_DIR}/third_party/openssl/lib/VC/x64/MT"
          "${PROJECT_SOURCE_DIR}/third_party/openssl/lib"
          "${PROJECT_SOURCE_DIR}/third_party/openssl"
    NO_DEFAULT_PATH
)

if(OPENSSL_INCLUDE_DIR AND OPENSSL_SSL_LIBRARY AND OPENSSL_CRYPTO_LIBRARY)
    set(OPENSSL_FOUND TRUE)
    set(OPENSSL_LIBRARIES ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})
else()
    set(OPENSSL_FOUND FALSE)
endif()

if(OPENSSL_FOUND)
    if(NOT TARGET OpenSSL::SSL)
        add_library(OpenSSL::SSL UNKNOWN IMPORTED)
        set_target_properties(OpenSSL::SSL PROPERTIES
            IMPORTED_LOCATION "${OPENSSL_SSL_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}"
        )
    endif()
    
    if(NOT TARGET OpenSSL::Crypto)
        add_library(OpenSSL::Crypto UNKNOWN IMPORTED)
        set_target_properties(OpenSSL::Crypto PROPERTIES
            IMPORTED_LOCATION "${OPENSSL_CRYPTO_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}"
        )
    endif()
endif()

mark_as_advanced(OPENSSL_INCLUDE_DIR OPENSSL_SSL_LIBRARY OPENSSL_CRYPTO_LIBRARY)