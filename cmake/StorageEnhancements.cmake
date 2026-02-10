# Storage and Data Management Enhancement Sources
# Advanced data structures, optimization, and special handling

list(APPEND THEMIS_CORE_SOURCES
    # Differential update engine for efficient updates
    ../src/server/rpc/differential_update_engine.cpp
    
    # Hybrid retention policies
    ../src/scheduler/hybrid_retention_manager.cpp
    
    # Task scheduler with cron and CDC triggers
    ../src/scheduler/task_scheduler.cpp
    ../src/scheduler/event_trigger.cpp
    
    # Task audit events and anomaly detection
    ../src/scheduler/task_audit_event.cpp
    ../src/scheduler/task_anomaly_detector.cpp
    ../src/scheduler/task_audit_manager.cpp
    
    # Cron parser utility
    ../src/utils/cron_parser.cpp
    
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
    ../src/storage/compressed_storage.cpp
    ../src/utils/compression_metrics.cpp
)
