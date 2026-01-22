# Storage and Data Management Enhancement Sources
# Advanced data structures, optimization, and special handling

list(APPEND THEMIS_CORE_SOURCES
    # Differential update engine for efficient updates
    ../src/server/rpc/differential_update_engine.cpp
    
    # Hybrid retention policies
    ../src/scheduler/hybrid_retention_manager.cpp
    
    # HyperTable data structure
    ../src/timeseries/hypertable.cpp
    
    # Paged optimizer for query performance
    ../src/llm/lora_framework/paged_optimizer.cpp
    
    # Storage Engine with Dependency Injection (Phase 2)
    ../src/storage/storage_engine.cpp
    
    # Index Maintenance Automation (Phase 4)
    ../src/storage/index_maintenance.cpp
    
    # Compression Strategy Manager and Metrics
    ../src/storage/compression_strategy.cpp
    ../src/utils/compression_metrics.cpp
)
