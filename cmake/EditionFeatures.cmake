# Edition-Specific Features
# Plugin system, sharding, and GPU memory management for specific editions

# Plugin system for enterprise
list(APPEND THEMIS_CORE_SOURCES
    ../src/plugins/plugin_system_edition.cpp
)

# Enterprise sharding manager
if(THEMIS_EDITION STREQUAL "ENTERPRISE" OR THEMIS_EDITION STREQUAL "HYPERSCALER")
    list(APPEND THEMIS_CORE_SOURCES
        ../src/sharding/sharding_manager_edition.cpp
    )
endif()

# GPU memory management for enterprise (edition-specific VRAM limits)
if(THEMIS_ENABLE_GPU AND (THEMIS_EDITION STREQUAL "ENTERPRISE" OR THEMIS_EDITION STREQUAL "HYPERSCALER"))
    list(APPEND THEMIS_CORE_SOURCES
        ../src/gpu/gpu_memory_manager_edition.cpp
    )
endif()

# GPU module infrastructure (pure C++ bookkeeping, no hardware required)
# Provides: device discovery, safe-fail circuit breaker, audit log, policy gate,
# memory pool, metrics registry, config validation, kernel validator, alert manager,
# async launcher, multi-GPU load balancer, feature flags, admin API, integration
# facade, stream manager, query accelerator, tensor buffer, training loop.
if(THEMIS_ENABLE_GPU)
    list(APPEND THEMIS_CORE_SOURCES
        ../src/gpu/device_discovery.cpp
        ../src/gpu/safe_fail.cpp
        ../src/gpu/audit_log.cpp
        ../src/gpu/policy.cpp
        ../src/gpu/memory_pool.cpp
        ../src/gpu/metrics.cpp
        ../src/gpu/config.cpp
        ../src/gpu/kernel_validator.cpp
        ../src/gpu/alerts.cpp
        ../src/gpu/launcher.cpp
        ../src/gpu/load_balancer.cpp
        ../src/gpu/feature_flags.cpp
        ../src/gpu/admin_api.cpp
        ../src/gpu/gpu_module.cpp
        ../src/gpu/stream_manager.cpp
        ../src/gpu/query_accelerator.cpp
        ../src/gpu/tensor_buffer.cpp
        ../src/gpu/training_loop.cpp
    )
endif()
