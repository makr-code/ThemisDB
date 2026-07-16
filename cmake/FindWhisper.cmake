# FindWhisper.cmake
# Finds Whisper.cpp library
#
# This module defines:
#  WHISPER_FOUND - system has Whisper.cpp
#  WHISPER_INCLUDE_DIR - the Whisper.cpp include directory
#  WHISPER_LIBRARY - the Whisper.cpp library
#  Whisper::Whisper - imported target

find_path(WHISPER_INCLUDE_DIR
    NAMES whisper.h
    PATHS
        ${WHISPER_ROOT}/include
        ${CMAKE_SOURCE_DIR}/whisper.cpp/include
        ${CMAKE_SOURCE_DIR}/whisper.cpp
        ${CMAKE_SOURCE_DIR}/external/whisper.cpp
        ${CMAKE_SOURCE_DIR}/third_party/whisper.cpp
        /usr/local/include
        /usr/include
    PATH_SUFFIXES whisper
)

find_library(WHISPER_LIBRARY
    NAMES whisper libwhisper
    PATHS
        ${WHISPER_ROOT}/lib
    ${CMAKE_SOURCE_DIR}/whisper.cpp/build/src
    ${CMAKE_SOURCE_DIR}/whisper.cpp/build
        ${CMAKE_SOURCE_DIR}/external/whisper.cpp/build
        ${CMAKE_SOURCE_DIR}/third_party/whisper.cpp/build
        /usr/local/lib
        /usr/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Whisper
    REQUIRED_VARS WHISPER_LIBRARY WHISPER_INCLUDE_DIR
)

if(WHISPER_FOUND AND NOT TARGET Whisper::Whisper)
    add_library(Whisper::Whisper UNKNOWN IMPORTED)
    set_target_properties(Whisper::Whisper PROPERTIES
        IMPORTED_LOCATION "${WHISPER_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${WHISPER_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(WHISPER_INCLUDE_DIR WHISPER_LIBRARY)
