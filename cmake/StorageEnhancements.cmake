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

    # Task Scheduler REST API and Web UI handler
    ../src/server/task_scheduler_api_handler.cpp
    
    # Distributed task coordination (Phase 2) and cron leader election (Phase 4)
    ../src/scheduler/distributed_task_coordinator.cpp

    # External scheduler adapters (Kubernetes CronJob, Airflow) – Phase 4
    ../src/scheduler/external_scheduler_adapter.cpp

    # Task audit events and anomaly detection
    ../src/scheduler/task_audit_event.cpp
    ../src/scheduler/task_anomaly_detector.cpp
    ../src/scheduler/task_audit_manager.cpp

    # Task execution result persistence (store outputs in ThemisDB)
    ../src/scheduler/task_result_store.cpp
    
    # Cron parser utility
    ../src/utils/cron_parser.cpp
    
    # HyperTable data structure
    ../src/timeseries/hypertable.cpp
    
    # Paged optimizer for query performance
    ../src/llm/lora_framework/paged_optimizer.cpp
    
    # Storage Engine with Dependency Injection (Phase 2)
    ../src/storage/storage_engine.cpp
    
    # WAL (Write-Ahead Log) for crash recovery and durability
    ../src/storage/wal_storage.cpp
    
    # Compaction and Garbage-Collection manager
    ../src/storage/compaction_manager.cpp

    # Storage Audit Logger (append-only, rotating audit trail)
    ../src/storage/storage_audit_logger.cpp

    # Index Maintenance Automation (Phase 4)
    ../src/storage/index_maintenance.cpp
    
    # Compression Strategy Manager and Metrics
    ../src/storage/compression_strategy.cpp
    ../src/storage/compressed_storage.cpp
    ../src/utils/compression_metrics.cpp

    # MVCC versioning and HLC timestamping (required for history/conflict layer)
    ../src/storage/hlc.cpp
    ../src/storage/mvcc_store.cpp

    # Atomic history and conflict layer
    ../src/storage/history_manager.cpp

    # Tiered storage (hot/warm/cold) with access-based and age-based migration
    ../src/storage/tiered_storage.cpp

    # Index Analyzer – per-index analyze with tier thresholds, cron scheduling, AI/ML hook – v1.9.0
    ../src/storage/index_analyzer.cpp

    # Distributed transactions (2PC across multiple shards) – v1.7.0
    ../src/storage/distributed_transaction_manager.cpp

    # NVMe optimizations (io_uring, multi-queue, ZNS, Direct I/O) – v1.6.0
    ../src/storage/nvme_manager.cpp

    # Online Schema Migration (zero-downtime DDL) – v1.7.0
    ../src/storage/online_schema_migration.cpp
)

# ============================================================================
# Tensor Network Module — TT/HT/QTT index and storage sources
# Added 2026-05-07: Phase 1–6 implementation files
# ============================================================================
list(APPEND THEMIS_CORE_SOURCES
    # Storage-layer tensor sources (TT decomposer, network engine, router, ...)
    ../src/storage/tensor_train_decomposer.cpp
    ../src/storage/tt_quantizer.cpp
    ../src/storage/tensor_network_storage_engine.cpp
    ../src/storage/tensor_router.cpp
    ../src/storage/tensor_compaction_filter.cpp
    ../src/storage/ggml_tensor_bridge.cpp
    ../src/storage/gguf_metadata.cpp

    # Core tensor index and manager
    ../src/tensor/tensor_index.cpp
    ../src/tensor/tensor_index_manager.cpp
    ../src/tensor/hnsw_tt_bridge.cpp

    # Phase 3 — zero-copy inference, query operators, RAG pipeline
    ../src/tensor/tensor_mmap_bridge.cpp
    ../src/tensor/tensor_core_bridge.cpp
    ../src/tensor/tensor_butterfly_operator.cpp
    ../src/tensor/tensor_ingestion_bridge.cpp
    ../src/query/tensor_contraction_engine.cpp
    ../src/query/tensor_aware_query_optimizer.cpp
    ../src/rag/flare_retrieval.cpp
    ../src/rag/targ_retrieval.cpp
    ../src/rag/tensor_rag_pipeline.cpp

    # Phase 3 — fingerprint graph, adapter repository
    ../src/tensor/tensor_fingerprint_graph.cpp
    ../src/tensor/adapter_repository.cpp

    # Phase 5 — HT index
    ../src/tensor/ht_index.cpp

    # Phase 6 — Hiss structural search, TNSR background task
    ../src/tensor/hiss_structural_search.cpp
    ../src/tensor/tnsr_task.cpp

    # Phase 7 — Unified Tensor Representation (UTR)
    ../src/tensor/hyper_index_builder.cpp
    ../src/tensor/utr_converter.cpp
)
