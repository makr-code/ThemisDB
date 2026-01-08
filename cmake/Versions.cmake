# ThemisDB Version Management
# Single source of truth: /VERSION file

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

# Edition selection: MINIMAL, COMMUNITY (default), ENTERPRISE, HYPERSCALER
if(NOT DEFINED THEMIS_EDITION)
    set(THEMIS_EDITION "COMMUNITY" CACHE STRING "ThemisDB Edition")
endif()

set_property(CACHE THEMIS_EDITION PROPERTY STRINGS 
    "MINIMAL" "COMMUNITY" "ENTERPRISE" "HYPERSCALER")

message(STATUS "ThemisDB ${THEMIS_VERSION} (${THEMIS_VERSION_STRING}) - Edition: ${THEMIS_EDITION}")

# Edition-based feature defaults
if(THEMIS_EDITION STREQUAL "MINIMAL")
    set(THEMIS_ENABLE_LLM OFF CACHE BOOL "LLM disabled for MINIMAL edition" FORCE)
    set(THEMIS_ENABLE_GRPC OFF CACHE BOOL "gRPC disabled for MINIMAL edition" FORCE)
    add_compile_definitions(THEMIS_MINIMAL_EDITION)
    
elseif(THEMIS_EDITION STREQUAL "COMMUNITY")
    # Community: core features, optional LLM/gRPC/protocols
    add_compile_definitions(THEMIS_COMMUNITY_EDITION)
    
elseif(THEMIS_EDITION STREQUAL "ENTERPRISE")
    # Enterprise: advanced features, HSM, multi-shard
    set(THEMIS_ENABLE_GRPC ON CACHE BOOL "gRPC enabled for ENTERPRISE edition" FORCE)
    add_compile_definitions(THEMIS_ENTERPRISE_EDITION)
    
elseif(THEMIS_EDITION STREQUAL "HYPERSCALER")
    # Hyperscaler: all features enabled, GPU, LLM, OpenTelemetry
    set(THEMIS_ENABLE_LLM ON CACHE BOOL "LLM enabled for HYPERSCALER edition" FORCE)
    set(THEMIS_ENABLE_GRPC ON CACHE BOOL "gRPC enabled for HYPERSCALER edition" FORCE)
    set(THEMIS_ENABLE_GPU ON CACHE BOOL "GPU enabled for HYPERSCALER edition" FORCE)
    set(THEMIS_ENABLE_TRACING ON CACHE BOOL "Tracing enabled for HYPERSCALER edition" FORCE)
    add_compile_definitions(THEMIS_HYPERSCALER_EDITION)
endif()
