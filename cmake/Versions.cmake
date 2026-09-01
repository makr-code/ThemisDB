# ThemisDB Version Management
# Single source of truth: /VERSION file
# Edition logic has been moved to cmake/editions/

# Read VERSION from project root
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/VERSION")
    message(FATAL_ERROR "VERSION file not found at ${CMAKE_SOURCE_DIR}/VERSION")
endif()

file(READ "${CMAKE_SOURCE_DIR}/VERSION" THEMIS_VERSION_STRING)
string(STRIP "${THEMIS_VERSION_STRING}" THEMIS_VERSION_STRING)

# Parse semantic version: MAJOR.MINOR.PATCH[-prerelease][+build]
set(_themis_version_type_candidate "")
if(NOT THEMIS_VERSION_STRING MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(-([A-Za-z0-9.-]+))?(\\+.*)?$")
    message(FATAL_ERROR "Invalid VERSION format: '${THEMIS_VERSION_STRING}'\n"
                        "Expected semantic version like: 1.4.0-alpha or 1.4.0+build123")
endif()
if(DEFINED CMAKE_MATCH_5 AND NOT "${CMAKE_MATCH_5}" STREQUAL "")
    set(_themis_version_type_candidate "${CMAKE_MATCH_5}")
endif()

string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)" THEMIS_VERSION_NUMERIC "${THEMIS_VERSION_STRING}")
if(NOT THEMIS_VERSION_NUMERIC)
    message(FATAL_ERROR "Failed to parse numeric version from '${THEMIS_VERSION_STRING}'")
endif()

set(THEMIS_VERSION "${THEMIS_VERSION_NUMERIC}")
set(THEMIS_VERSION_MAJOR "${CMAKE_MATCH_1}")
set(THEMIS_VERSION_MINOR "${CMAKE_MATCH_2}")
set(THEMIS_VERSION_PATCH "${CMAKE_MATCH_3}")

set(THEMIS_VERSION_TYPE "stable")

if(NOT "${_themis_version_type_candidate}" STREQUAL "")
    string(TOLOWER "${_themis_version_type_candidate}" _themis_version_type_raw)
    string(REGEX REPLACE "[^a-z0-9]+" "" _themis_version_type_normalized "${_themis_version_type_raw}")
    if(NOT "${_themis_version_type_normalized}" STREQUAL "")
        set(THEMIS_VERSION_TYPE "${_themis_version_type_normalized}")
    endif()
endif()

# Make version available globally
add_compile_definitions(
    THEMIS_VERSION_STRING="${THEMIS_VERSION_STRING}"
    THEMIS_VERSION="${THEMIS_VERSION}"
    THEMIS_VERSION_TYPE="${THEMIS_VERSION_TYPE}"
)
set(THEMIS_VERSION_COMPILE_DEF_DEFINED ON)

set(THEMIS_VERSION_TYPE "${THEMIS_VERSION_TYPE}" CACHE STRING "ThemisDB version type" FORCE)
set(THEMIS_VERSION_NUMERIC "${THEMIS_VERSION_NUMERIC}" CACHE STRING "ThemisDB numeric version" FORCE)
message(STATUS "ThemisDB ${THEMIS_VERSION} (${THEMIS_VERSION_STRING}) [type=${THEMIS_VERSION_TYPE}]")
