# ThemisDB Edition Selection and Orchestration
# Validates platform detection and loads appropriate edition configuration

if(NOT THEMIS_PLATFORM_DETECTED)
    message(FATAL_ERROR "EditionDefaults.cmake requires PlatformDetection.cmake to be included first")
endif()

# Edition selection: MINIMAL, COMMUNITY (default), ENTERPRISE, HYPERSCALER, MILITARY
if(NOT DEFINED THEMIS_EDITION)
    set(THEMIS_EDITION "COMMUNITY" CACHE STRING "ThemisDB Edition")
endif()

set_property(CACHE THEMIS_EDITION PROPERTY STRINGS
    "MINIMAL" "COMMUNITY" "ENTERPRISE" "HYPERSCALER" "MILITARY")

# Validate edition
if(NOT THEMIS_EDITION MATCHES "^(MINIMAL|COMMUNITY|ENTERPRISE|HYPERSCALER|MILITARY)$")
    message(FATAL_ERROR "Invalid THEMIS_EDITION: ${THEMIS_EDITION}. Must be one of: MINIMAL, COMMUNITY, ENTERPRISE, HYPERSCALER, MILITARY")
endif()

message(STATUS "==========================================")
message(STATUS "Edition: ${THEMIS_EDITION}")
message(STATUS "==========================================")

# Load edition-specific configuration
include(${CMAKE_CURRENT_LIST_DIR}/${THEMIS_EDITION}.cmake)

# Set global compile definition for edition
add_compile_definitions(THEMIS_EDITION_${THEMIS_EDITION})

# Set global flag for edition selection completed
set(THEMIS_EDITION_SELECTED TRUE CACHE INTERNAL "Edition selection completed")
