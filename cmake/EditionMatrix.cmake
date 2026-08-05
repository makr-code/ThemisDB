# ThemisDB Edition-Feature Compatibility Matrix
# Defines which features are available in each edition

message(STATUS "==========================================")
message(STATUS "Edition-Feature Matrix Validation")
message(STATUS "==========================================")

# Centralized GPU VRAM limit compile definition (applied once per edition)
# This is defined once here AFTER all edition files are loaded to avoid redefinition
if(NOT THEMIS_GPU_LIMIT_COMPILE_DEF_DEFINED)
    add_compile_definitions(THEMIS_GPU_MAX_VRAM_GB=${THEMIS_GPU_MAX_VRAM_GB})
    set(THEMIS_GPU_LIMIT_COMPILE_DEF_DEFINED ON)
endif()

if(NOT THEMIS_EDITION_SELECTED)
    message(FATAL_ERROR "EditionMatrix.cmake requires EditionDefaults.cmake to be included first")
endif()

# ============================================================================
# EDITION-FEATURE COMPATIBILITY MATRIX
# ============================================================================
# This matrix defines which features are:
# - REQUIRED: Must be ON for this edition
# - ALLOWED: Can be toggled ON/OFF by user
# - FORBIDDEN: Must be OFF for this edition (will be forced off with warning)

# Define the matrix
# Format: EDITION_FEATURE_<EDITION>_<FEATURE> = REQUIRED | ALLOWED | FORBIDDEN

# MINIMAL Edition - Smallest footprint, basic functionality only
set(EDITION_FEATURE_MINIMAL_LLM "FORBIDDEN")
set(EDITION_FEATURE_MINIMAL_GPU "FORBIDDEN")
set(EDITION_FEATURE_MINIMAL_CUDA "FORBIDDEN")
set(EDITION_FEATURE_MINIMAL_GRPC "FORBIDDEN")
set(EDITION_FEATURE_MINIMAL_TRACING "FORBIDDEN")
set(EDITION_FEATURE_MINIMAL_HTTP2 "ALLOWED")
set(EDITION_FEATURE_MINIMAL_HTTP3 "FORBIDDEN")
set(EDITION_FEATURE_MINIMAL_WEBSOCKET "ALLOWED")
set(EDITION_FEATURE_MINIMAL_MQTT "ALLOWED")
set(EDITION_FEATURE_MINIMAL_POSTGRES_WIRE "ALLOWED")
set(EDITION_FEATURE_MINIMAL_MCP "FORBIDDEN")
set(EDITION_FEATURE_MINIMAL_SSE "ALLOWED")
set(EDITION_FEATURE_MINIMAL_DISKANN "FORBIDDEN")
set(EDITION_FEATURE_MINIMAL_DISTRIBUTED_TRAINING "FORBIDDEN")

# COMMUNITY Edition - Standard features for small to medium deployments
set(EDITION_FEATURE_COMMUNITY_LLM "ALLOWED")
set(EDITION_FEATURE_COMMUNITY_GPU "ALLOWED")
set(EDITION_FEATURE_COMMUNITY_CUDA "ALLOWED")
set(EDITION_FEATURE_COMMUNITY_GRPC "ALLOWED")
set(EDITION_FEATURE_COMMUNITY_TRACING "ALLOWED")
set(EDITION_FEATURE_COMMUNITY_HTTP2 "ALLOWED")
set(EDITION_FEATURE_COMMUNITY_HTTP3 "ALLOWED")
set(EDITION_FEATURE_COMMUNITY_WEBSOCKET "ALLOWED")
set(EDITION_FEATURE_COMMUNITY_MQTT "ALLOWED")
set(EDITION_FEATURE_COMMUNITY_POSTGRES_WIRE "ALLOWED")
set(EDITION_FEATURE_COMMUNITY_MCP "ALLOWED")
set(EDITION_FEATURE_COMMUNITY_SSE "ALLOWED")
set(EDITION_FEATURE_COMMUNITY_DISKANN "ALLOWED")
set(EDITION_FEATURE_COMMUNITY_DISTRIBUTED_TRAINING "ALLOWED")

# ENTERPRISE Edition - Advanced features for production deployments
set(EDITION_FEATURE_ENTERPRISE_LLM "ALLOWED")
set(EDITION_FEATURE_ENTERPRISE_GPU "ALLOWED")
set(EDITION_FEATURE_ENTERPRISE_CUDA "ALLOWED")
set(EDITION_FEATURE_ENTERPRISE_GRPC "REQUIRED")
set(EDITION_FEATURE_ENTERPRISE_TRACING "REQUIRED")
set(EDITION_FEATURE_ENTERPRISE_HTTP2 "ALLOWED")
set(EDITION_FEATURE_ENTERPRISE_HTTP3 "ALLOWED")
set(EDITION_FEATURE_ENTERPRISE_WEBSOCKET "ALLOWED")
set(EDITION_FEATURE_ENTERPRISE_MQTT "ALLOWED")
set(EDITION_FEATURE_ENTERPRISE_POSTGRES_WIRE "ALLOWED")
set(EDITION_FEATURE_ENTERPRISE_MCP "ALLOWED")
set(EDITION_FEATURE_ENTERPRISE_SSE "ALLOWED")
set(EDITION_FEATURE_ENTERPRISE_DISKANN "ALLOWED")
set(EDITION_FEATURE_ENTERPRISE_DISTRIBUTED_TRAINING "ALLOWED")

# HYPERSCALER Edition - All features for cloud-scale deployments
set(EDITION_FEATURE_HYPERSCALER_LLM "REQUIRED")
set(EDITION_FEATURE_HYPERSCALER_GPU "REQUIRED")
set(EDITION_FEATURE_HYPERSCALER_CUDA "ALLOWED")
set(EDITION_FEATURE_HYPERSCALER_GRPC "REQUIRED")
set(EDITION_FEATURE_HYPERSCALER_TRACING "REQUIRED")
set(EDITION_FEATURE_HYPERSCALER_HTTP2 "ALLOWED")
set(EDITION_FEATURE_HYPERSCALER_HTTP3 "ALLOWED")
set(EDITION_FEATURE_HYPERSCALER_WEBSOCKET "ALLOWED")
set(EDITION_FEATURE_HYPERSCALER_MQTT "ALLOWED")
set(EDITION_FEATURE_HYPERSCALER_POSTGRES_WIRE "ALLOWED")
set(EDITION_FEATURE_HYPERSCALER_MCP "ALLOWED")
set(EDITION_FEATURE_HYPERSCALER_SSE "ALLOWED")
set(EDITION_FEATURE_HYPERSCALER_DISKANN "ALLOWED")
set(EDITION_FEATURE_HYPERSCALER_DISTRIBUTED_TRAINING "ALLOWED")

# MILITARY Edition - Hardened, air-gapped capable, government/classified deployments
set(EDITION_FEATURE_MILITARY_LLM "ALLOWED")
set(EDITION_FEATURE_MILITARY_GPU "ALLOWED")
set(EDITION_FEATURE_MILITARY_CUDA "ALLOWED")
set(EDITION_FEATURE_MILITARY_GRPC "REQUIRED")
set(EDITION_FEATURE_MILITARY_TRACING "ALLOWED")
set(EDITION_FEATURE_MILITARY_HTTP2 "ALLOWED")
set(EDITION_FEATURE_MILITARY_HTTP3 "FORBIDDEN")
set(EDITION_FEATURE_MILITARY_WEBSOCKET "ALLOWED")
set(EDITION_FEATURE_MILITARY_MQTT "ALLOWED")
set(EDITION_FEATURE_MILITARY_POSTGRES_WIRE "ALLOWED")
set(EDITION_FEATURE_MILITARY_MCP "FORBIDDEN")
set(EDITION_FEATURE_MILITARY_SSE "ALLOWED")
set(EDITION_FEATURE_MILITARY_DISKANN "ALLOWED")
set(EDITION_FEATURE_MILITARY_DISTRIBUTED_TRAINING "FORBIDDEN")

# ============================================================================
# CI MODE
# ============================================================================
# When THEMIS_CI_MODE=ON, REQUIRED features are treated as default-ON but are
# NOT force-overridden. This allows CI builds to disable heavyweight external
# dependencies (gRPC, GPU, LLM) that are unavailable in standard CI runners
# while still validating the edition configuration.
option(THEMIS_CI_MODE "Relaxed feature enforcement for CI builds" OFF)

# ============================================================================
# VALIDATION LOGIC
# ============================================================================

# Helper function to validate a single feature
function(validate_feature FEATURE_NAME FEATURE_VAR)
    # Get the matrix value for this edition + feature
    set(matrix_key "EDITION_FEATURE_${THEMIS_EDITION}_${FEATURE_NAME}")

    if(DEFINED ${matrix_key})
        set(policy ${${matrix_key}})

        if(policy STREQUAL "REQUIRED")
            if(THEMIS_CI_MODE)
                # In CI mode: warn but do NOT force the feature ON so that
                # builds without the dependency (e.g. gRPC) can still succeed.
                if(NOT ${FEATURE_VAR})
                    message(STATUS "  ${FEATURE_VAR}: REQUIRED by ${THEMIS_EDITION} edition (CI mode: not forced ON)")
                else()
                    message(STATUS "  ${FEATURE_VAR}: REQUIRED (ON)")
                endif()
            else()
                if(NOT ${FEATURE_VAR})
                    message(WARNING "${FEATURE_VAR} is REQUIRED for ${THEMIS_EDITION} edition, forcing ON")
                    set(${FEATURE_VAR} ON CACHE BOOL "Required by ${THEMIS_EDITION} edition" FORCE)
                endif()
                message(STATUS "  ${FEATURE_VAR}: REQUIRED (ON)")
            endif()
            
        elseif(policy STREQUAL "FORBIDDEN")
            if(${FEATURE_VAR})
                message(WARNING "${FEATURE_VAR} is FORBIDDEN for ${THEMIS_EDITION} edition, forcing OFF")
                set(${FEATURE_VAR} OFF CACHE BOOL "Forbidden by ${THEMIS_EDITION} edition" FORCE)
            endif()
            message(STATUS "  ${FEATURE_VAR}: FORBIDDEN (OFF)")
            
        elseif(policy STREQUAL "ALLOWED")
            if(${FEATURE_VAR})
                message(STATUS "  ${FEATURE_VAR}: ALLOWED (ON)")
            else()
                message(STATUS "  ${FEATURE_VAR}: ALLOWED (OFF)")
            endif()
        endif()
    else()
        # Feature not in matrix, allow it
        if(${FEATURE_VAR})
            message(STATUS "  ${FEATURE_VAR}: Not in matrix (ON)")
        else()
            message(STATUS "  ${FEATURE_VAR}: Not in matrix (OFF)")
        endif()
    endif()
endfunction()

# Validate all features
message(STATUS "Validating features against ${THEMIS_EDITION} edition matrix:")

validate_feature(LLM THEMIS_ENABLE_LLM)
validate_feature(GPU THEMIS_ENABLE_GPU)
validate_feature(CUDA THEMIS_ENABLE_CUDA)
validate_feature(GRPC THEMIS_ENABLE_GRPC)
validate_feature(TRACING THEMIS_ENABLE_TRACING)
validate_feature(HTTP2 THEMIS_ENABLE_HTTP2)
validate_feature(HTTP3 THEMIS_ENABLE_HTTP3)
validate_feature(WEBSOCKET THEMIS_ENABLE_WEBSOCKET)
validate_feature(MQTT THEMIS_ENABLE_MQTT)
validate_feature(POSTGRES_WIRE THEMIS_ENABLE_POSTGRES_WIRE)
validate_feature(MCP THEMIS_ENABLE_MCP)
validate_feature(SSE THEMIS_ENABLE_SSE)
validate_feature(DISKANN THEMIS_ENABLE_DISKANN)
validate_feature(DISTRIBUTED_TRAINING THEMIS_ENABLE_DISTRIBUTED_TRAINING)

message(STATUS "==========================================")
message(STATUS "Edition-Feature Matrix Validation: Complete")
message(STATUS "==========================================")
