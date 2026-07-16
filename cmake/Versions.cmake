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
if(NOT THEMIS_VERSION_STRING MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(.*)$")
    message(FATAL_ERROR "Invalid VERSION format: '${THEMIS_VERSION_STRING}'\n"
                        "Expected semantic version like: 1.4.0-alpha or 1.4.0+build123")
endif()

string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)" THEMIS_VERSION_NUMERIC "${THEMIS_VERSION_STRING}")
set(THEMIS_VERSION "${THEMIS_VERSION_NUMERIC}")

# Make version available globally
add_compile_definitions(
    THEMIS_VERSION_STRING="${THEMIS_VERSION_STRING}"
    THEMIS_VERSION="${THEMIS_VERSION}"
)
set(THEMIS_VERSION_COMPILE_DEF_DEFINED ON)

message(STATUS "ThemisDB ${THEMIS_VERSION} (${THEMIS_VERSION_STRING})")
