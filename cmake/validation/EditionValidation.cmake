# ThemisDB Edition Validation
# Validates edition constraints and feature compatibility

if(NOT THEMIS_EDITION_SELECTED)
    message(FATAL_ERROR "EditionValidation.cmake requires EditionDefaults.cmake to be included first")
endif()

if(NOT THEMIS_FEATURES_CONFIGURED)
    message(FATAL_ERROR "EditionValidation.cmake requires FeatureDefaults.cmake to be included first")
endif()

message(STATUS "Validating edition constraints...")

# MINIMAL edition validation
if(THEMIS_EDITION STREQUAL "MINIMAL")
    # Ensure MINIMAL constraints are enforced
    if(THEMIS_ENABLE_LLM)
        message(FATAL_ERROR "MINIMAL edition does not support LLM. Use COMMUNITY or higher edition.")
    endif()
    
    if(THEMIS_ENABLE_GRPC)
        message(FATAL_ERROR "MINIMAL edition does not support gRPC. Use COMMUNITY or higher edition.")
    endif()
    
    if(THEMIS_ENABLE_GPU)
        message(FATAL_ERROR "MINIMAL edition does not support GPU acceleration. Use COMMUNITY or higher edition.")
    endif()
    
    if(THEMIS_ENABLE_TRACING)
        message(FATAL_ERROR "MINIMAL edition does not support OpenTelemetry tracing. Use HYPERSCALER edition.")
    endif()
    
    if(THEMIS_ENABLE_DISTRIBUTED_TRAINING)
        message(FATAL_ERROR "MINIMAL edition does not support distributed training. Use HYPERSCALER edition.")
    endif()
endif()

# COMMUNITY edition validation
if(THEMIS_EDITION STREQUAL "COMMUNITY")
    # Warn about advanced features
    if(THEMIS_ENABLE_TRACING)
        message(WARNING "OpenTelemetry tracing is not officially supported in COMMUNITY edition. Consider HYPERSCALER edition.")
    endif()
    
    if(THEMIS_ENABLE_HSM_REAL)
        message(WARNING "Real HSM is not supported in COMMUNITY edition. Use ENTERPRISE or HYPERSCALER edition.")
    endif()
    
    if(THEMIS_ENABLE_DISTRIBUTED_TRAINING)
        message(WARNING "Distributed training is not supported in COMMUNITY edition. Use HYPERSCALER edition.")
    endif()
endif()

# ENTERPRISE edition validation
if(THEMIS_EDITION STREQUAL "ENTERPRISE")
    # Ensure gRPC is enabled
    if(NOT THEMIS_ENABLE_GRPC)
        message(FATAL_ERROR "ENTERPRISE edition requires gRPC to be enabled.")
    endif()
    
    if(THEMIS_ENABLE_DISTRIBUTED_TRAINING)
        message(WARNING "Distributed training is not officially supported in ENTERPRISE edition. Consider HYPERSCALER edition.")
    endif()
endif()

# HYPERSCALER edition validation
if(THEMIS_EDITION STREQUAL "HYPERSCALER")
    # Ensure critical features are enabled
    if(NOT THEMIS_ENABLE_LLM)
        message(WARNING "LLM is recommended for HYPERSCALER edition but currently disabled.")
    endif()
    
    if(NOT THEMIS_ENABLE_GPU)
        message(WARNING "GPU acceleration is recommended for HYPERSCALER edition but currently disabled.")
    endif()
endif()

message(STATUS "Edition validation: OK")
