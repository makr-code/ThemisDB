# FindPiper.cmake
# Finds Piper TTS library
#
# This module defines:
#  PIPER_FOUND - system has Piper TTS
#  PIPER_INCLUDE_DIR - the Piper include directory
#  PIPER_LIBRARY - the Piper library
#  Piper::Piper - imported target

find_path(PIPER_INCLUDE_DIR
    NAMES piper.hpp
    PATHS
        ${PIPER_ROOT}/include
        ${CMAKE_SOURCE_DIR}/external/piper
        ${CMAKE_SOURCE_DIR}/third_party/piper/src/cpp
        /usr/local/include
        /usr/include
    PATH_SUFFIXES piper
)

find_library(PIPER_LIBRARY
    NAMES piper_phonemize libpiper_phonemize piper
    PATHS
        ${PIPER_ROOT}/lib
        ${CMAKE_SOURCE_DIR}/external/piper/build
        ${CMAKE_SOURCE_DIR}/third_party/piper/build
        /usr/local/lib
        /usr/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Piper
    REQUIRED_VARS PIPER_LIBRARY PIPER_INCLUDE_DIR
)

if(PIPER_FOUND AND NOT TARGET Piper::Piper)
    add_library(Piper::Piper UNKNOWN IMPORTED)
    set_target_properties(Piper::Piper PROPERTIES
        IMPORTED_LOCATION "${PIPER_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${PIPER_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(PIPER_INCLUDE_DIR PIPER_LIBRARY)
