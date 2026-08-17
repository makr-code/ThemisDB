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

# GPU module infrastructure (pure C++ bookkeeping, no hardware required)
# Provides: device discovery, safe-fail circuit breaker, audit log, policy gate,
# memory pool, metrics registry, config validation, kernel validator, alert manager,
# async launcher, multi-GPU load balancer, feature flags, admin API, integration
# facade, stream manager, query accelerator, tensor buffer, training loop.
# gpu_memory_manager_edition.cpp implements edition-aware VRAM allocation; it
# uses edition::GPU_MAX_VRAM_GB at runtime so it compiles correctly for every
# GPU-enabled edition (Community, Enterprise, Hyperscaler).
if(THEMIS_ENABLE_GPU)
    list(APPEND THEMIS_CORE_SOURCES
        ../src/gpu/device_discovery.cpp
        ../src/gpu/safe_fail.cpp
        ../src/gpu/audit_log.cpp
        ../src/gpu/policy.cpp
        ../src/gpu/memory_pool.cpp
        ../src/gpu/gpu_memory_manager_edition.cpp
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
        ../src/gpu/gpu_safe_raii.cpp
        ../src/gpu/kernel_timeout_enforcer.cpp
        ../src/gpu/query_accelerator.cpp
        ../src/gpu/graph_cache.cpp
        ../src/gpu/tensor_buffer.cpp
        ../src/gpu/training_loop.cpp
        ../src/gpu/rocm_backend.cpp
        ../src/gpu/cluster_topology.cpp
        ../src/gpu/cluster_coordinator.cpp
        ../src/gpu/profiler.cpp
        ../src/gpu/unified_memory.cpp
        ../src/gpu/time_slice_scheduler.cpp
        ../src/gpu/wasm_kernel_sandbox.cpp
        ../src/gpu/mig_manager.cpp
        ../src/gpu/vulkan_backend.cpp
        ../src/gpu/p2p_transfer.cpp
    )
else()
    # CPU-only builds still require these symbols because geo/gpu backend stubs
    # reuse the shared safe-fail and audit infrastructure.
    list(APPEND THEMIS_CORE_SOURCES
        ../src/gpu/device_discovery.cpp
        ../src/gpu/safe_fail.cpp
        ../src/gpu/audit_log.cpp
        ../src/gpu/metrics.cpp
    )
endif()
