# FindKerberos.cmake
# Find Kerberos 5 (MIT Kerberos, Heimdal, or Active Directory)
#
# This module defines:
#   Kerberos_FOUND        - System has Kerberos
#   Kerberos_INCLUDE_DIRS - The Kerberos include directories
#   Kerberos_LIBRARIES    - The libraries needed to use Kerberos
#   Kerberos::Kerberos    - Imported target for Kerberos
#
# Environment variables:
#   KRB5_ROOT      - Root directory for Kerberos installation
#   KRB5_INCLUDEDIR - Include directory
#   KRB5_LIBDIR    - Library directory

# Try to find krb5-config
find_program(KRB5_CONFIG NAMES krb5-config)

if(KRB5_CONFIG)
    # Use krb5-config to get compile and link flags
    execute_process(
        COMMAND ${KRB5_CONFIG} --cflags gssapi
        OUTPUT_VARIABLE KRB5_CFLAGS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    execute_process(
        COMMAND ${KRB5_CONFIG} --libs gssapi
        OUTPUT_VARIABLE KRB5_LDFLAGS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    # Parse include directories from CFLAGS
    string(REGEX MATCHALL "-I([^ ]+)" KRB5_INCLUDE_FLAGS ${KRB5_CFLAGS})
    foreach(FLAG ${KRB5_INCLUDE_FLAGS})
        string(REGEX REPLACE "^-I" "" DIR ${FLAG})
        list(APPEND Kerberos_INCLUDE_DIRS ${DIR})
    endforeach()
    
    # Parse libraries from LDFLAGS
    string(REGEX MATCHALL "-l([^ ]+)" KRB5_LIB_FLAGS ${KRB5_LDFLAGS})
    foreach(FLAG ${KRB5_LIB_FLAGS})
        string(REGEX REPLACE "^-l" "" LIB ${FLAG})
        list(APPEND Kerberos_LIBRARIES ${LIB})
    endforeach()
    
    # Also extract library directories
    string(REGEX MATCHALL "-L([^ ]+)" KRB5_LIBDIR_FLAGS ${KRB5_LDFLAGS})
    foreach(FLAG ${KRB5_LIBDIR_FLAGS})
        string(REGEX REPLACE "^-L" "" DIR ${FLAG})
        list(APPEND Kerberos_LIBRARY_DIRS ${DIR})
    endforeach()
endif()

# If krb5-config not found or failed, try to find headers and libraries manually
if(NOT Kerberos_INCLUDE_DIRS)
    find_path(Kerberos_INCLUDE_DIR
        NAMES gssapi/gssapi.h gssapi.h krb5.h
        PATHS
            /usr/include
            /usr/local/include
            /opt/local/include
            /opt/homebrew/include
            $ENV{KRB5_ROOT}/include
            $ENV{KRB5_INCLUDEDIR}
        PATH_SUFFIXES
            gssapi
            krb5
    )
    
    if(Kerberos_INCLUDE_DIR)
        list(APPEND Kerberos_INCLUDE_DIRS ${Kerberos_INCLUDE_DIR})
        
        # Check for parent directory if we found a subdirectory
        get_filename_component(PARENT_DIR ${Kerberos_INCLUDE_DIR} DIRECTORY)
        if(EXISTS "${PARENT_DIR}/gssapi/gssapi.h" OR EXISTS "${PARENT_DIR}/krb5.h")
            list(APPEND Kerberos_INCLUDE_DIRS ${PARENT_DIR})
        endif()
    endif()
endif()

if(NOT Kerberos_LIBRARIES)
    # Find required libraries
    find_library(KRB5_LIBRARY
        NAMES krb5
        PATHS
            /usr/lib
            /usr/local/lib
            /opt/local/lib
            /opt/homebrew/lib
            $ENV{KRB5_ROOT}/lib
            $ENV{KRB5_LIBDIR}
    )
    
    find_library(GSSAPI_LIBRARY
        NAMES gssapi_krb5 gssapi
        PATHS
            /usr/lib
            /usr/local/lib
            /opt/local/lib
            /opt/homebrew/lib
            $ENV{KRB5_ROOT}/lib
            $ENV{KRB5_LIBDIR}
    )
    
    if(KRB5_LIBRARY)
        list(APPEND Kerberos_LIBRARIES ${KRB5_LIBRARY})
    endif()
    
    if(GSSAPI_LIBRARY)
        list(APPEND Kerberos_LIBRARIES ${GSSAPI_LIBRARY})
    endif()
    
    # Additional support libraries (optional)
    find_library(K5CRYPTO_LIBRARY NAMES k5crypto)
    if(K5CRYPTO_LIBRARY)
        list(APPEND Kerberos_LIBRARIES ${K5CRYPTO_LIBRARY})
    endif()
    
    find_library(COM_ERR_LIBRARY NAMES com_err)
    if(COM_ERR_LIBRARY)
        list(APPEND Kerberos_LIBRARIES ${COM_ERR_LIBRARY})
    endif()
endif()

# Remove duplicates
if(Kerberos_INCLUDE_DIRS)
    list(REMOVE_DUPLICATES Kerberos_INCLUDE_DIRS)
endif()

if(Kerberos_LIBRARIES)
    list(REMOVE_DUPLICATES Kerberos_LIBRARIES)
endif()

# Handle standard arguments
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Kerberos
    REQUIRED_VARS Kerberos_LIBRARIES Kerberos_INCLUDE_DIRS
    VERSION_VAR Kerberos_VERSION
)

# Create imported target
if(Kerberos_FOUND AND NOT TARGET Kerberos::Kerberos)
    add_library(Kerberos::Kerberos INTERFACE IMPORTED)
    set_target_properties(Kerberos::Kerberos PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${Kerberos_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES "${Kerberos_LIBRARIES}"
    )
endif()

mark_as_advanced(
    Kerberos_INCLUDE_DIR
    KRB5_LIBRARY
    GSSAPI_LIBRARY
    K5CRYPTO_LIBRARY
    COM_ERR_LIBRARY
    KRB5_CONFIG
)
