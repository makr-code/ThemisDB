# Workflow-Dokumentation

Dieses Verzeichnis enthält eine Markdown-Dokumentation für jeden GitHub Actions Workflow im Repository.
Für jeden Workflow wird Aufgabe, Auslöser, Eingaben und Funktionsweise beschrieben.

## Legende

| Symbol | Bedeutung |
|--------|-----------|
| 🔄 CI/CD | Automatisch bei Push/PR ausgelöst |
| 🖱️ Manuell | Nur über die GitHub Actions UI ausführbar |
| ♻️ Reusable | Wiederverwendbarer Workflow, der von anderen aufgerufen wird |
| ⏰ Geplant | Zeitgesteuert (Cron) |

**Gesamt: 217 Workflows**

## 01 · Core

| Workflow | Typ | Dokumentation |
|----------|-----|---------------|
| CI Scope Classifier | ♻️ Reusable | [01-core/ci-scope-classifier.md](01-core/ci-scope-classifier.md) |
| Themis Core Framework CI | 🔄 CI/CD | [01-core/themis-core-ci.md](01-core/themis-core-ci.md) |

## 02 · Feature Modules

| Workflow | Typ | Dokumentation |
|----------|-----|---------------|
| CapabilityAutoGenerator Persist State CI | 🔄 CI/CD | [02-feature-modules/capabilityautogenerator-persist-state-ci.md](02-feature-modules/capabilityautogenerator-persist-state-ci.md) |
| CLI Migration Scanner CI | 🔄 CI/CD | [02-feature-modules/cli-migration-scanner-ci.md](02-feature-modules/cli-migration-scanner-ci.md) |
| Dependency Resolution Engine CI | 🔄 CI/CD | [02-feature-modules/dependency-resolution-engine-ci.md](02-feature-modules/dependency-resolution-engine-ci.md) |
| ExporterFactory Stub Replacement CI | 🔄 CI/CD | [02-feature-modules/exporterfactory-stub-replacement-ci.md](02-feature-modules/exporterfactory-stub-replacement-ci.md) |
| Multi-Tenant Update Scheduling CI | 🔄 CI/CD | [02-feature-modules/multi-tenant-update-scheduling-ci.md](02-feature-modules/multi-tenant-update-scheduling-ci.md) |
| Parquet Exporter Tests | 🔄 CI/CD | [02-feature-modules/parquet-exporter-tests.md](02-feature-modules/parquet-exporter-tests.md) |
| Plugin Registry Shared Mutex CI | 🔄 CI/CD | [02-feature-modules/plugin-registry-shared-mutex-ci.md](02-feature-modules/plugin-registry-shared-mutex-ci.md) |
| Process Graph Visit Timestamp CI | 🔄 CI/CD | [02-feature-modules/process-graph-visit-timestamp-ci.md](02-feature-modules/process-graph-visit-timestamp-ci.md) |
| TaskScheduler Auth Context CI | 🔄 CI/CD | [02-feature-modules/taskscheduler-auth-context-ci.md](02-feature-modules/taskscheduler-auth-context-ci.md) |
| Acceleration Benchmark CI | 🔄 CI/CD | [02-feature-modules/acceleration/acceleration-benchmark-ci.md](02-feature-modules/acceleration/acceleration-benchmark-ci.md) |
| CUDA HNSW Large-k CI | 🔄 CI/CD | [02-feature-modules/acceleration/cuda-hnsw-large-k-ci.md](02-feature-modules/acceleration/cuda-hnsw-large-k-ci.md) |
| Runtime Device Capability Negotiation CI | 🔄 CI/CD | [02-feature-modules/acceleration/runtime-device-capability-negotiation-ci.md](02-feature-modules/acceleration/runtime-device-capability-negotiation-ci.md) |
| VLLMResourceManager Multi-GPU NVML Monitoring CI | 🔄 CI/CD | [02-feature-modules/acceleration/vllm-multi-gpu-nvml-monitoring-ci.md](02-feature-modules/acceleration/vllm-multi-gpu-nvml-monitoring-ci.md) |
| Adaptive Join Strategies CI | 🔄 CI/CD | [02-feature-modules/adaptive-query/adaptive-join-strategies-ci.md](02-feature-modules/adaptive-query/adaptive-join-strategies-ci.md) |
| Adaptive Query Compilation CI | 🔄 CI/CD | [02-feature-modules/adaptive-query/adaptive-query-compilation-ci.md](02-feature-modules/adaptive-query/adaptive-query-compilation-ci.md) |
| Geo Module CI | 🔄 CI/CD | [02-feature-modules/adaptive-query/geo-module-ci.md](02-feature-modules/adaptive-query/geo-module-ci.md) |
| Geo Spatial JOIN CI | 🔄 CI/CD | [02-feature-modules/adaptive-query/geo-spatial-join-ci.md](02-feature-modules/adaptive-query/geo-spatial-join-ci.md) |
| Intelligent Prefetching CI | 🔄 CI/CD | [02-feature-modules/adaptive-query/intelligent-prefetching-ci.md](02-feature-modules/adaptive-query/intelligent-prefetching-ci.md) |
| Materialized Views Incremental Maintenance CI | 🔄 CI/CD | [02-feature-modules/adaptive-query/materialized-views-incremental-maintenance-ci.md](02-feature-modules/adaptive-query/materialized-views-incremental-maintenance-ci.md) |
| Parallel Query Execution (Intra-Query) CI | 🔄 CI/CD | [02-feature-modules/adaptive-query/parallel-query-execution-intra-query-ci.md](02-feature-modules/adaptive-query/parallel-query-execution-intra-query-ci.md) |
| Query JIT Compilation CI | 🔄 CI/CD | [02-feature-modules/adaptive-query/query-jit-compilation-ci.md](02-feature-modules/adaptive-query/query-jit-compilation-ci.md) |
| Query Plan Caching CI | 🔄 CI/CD | [02-feature-modules/adaptive-query/query-plan-caching-ci.md](02-feature-modules/adaptive-query/query-plan-caching-ci.md) |
| Query Vectorized Execution CI | 🔄 CI/CD | [02-feature-modules/adaptive-query/query-vectorized-execution-ci.md](02-feature-modules/adaptive-query/query-vectorized-execution-ci.md) |
| Temporal-Spatial Queries CI | 🔄 CI/CD | [02-feature-modules/adaptive-query/temporal-spatial-queries-ci.md](02-feature-modules/adaptive-query/temporal-spatial-queries-ci.md) |
| Chimera Benchmark CI | 🔄 CI/CD | [02-feature-modules/chimera/chimera-benchmark-ci.md](02-feature-modules/chimera/chimera-benchmark-ci.md) |
| Chimera Error Recovery & Retry CI | 🔄 CI/CD | [02-feature-modules/chimera/chimera-error-recovery-retry-ci.md](02-feature-modules/chimera/chimera-error-recovery-retry-ci.md) |
| Chimera Integration CI | 🔄 CI/CD | [02-feature-modules/chimera/chimera-integration-ci.md](02-feature-modules/chimera/chimera-integration-ci.md) |
| Config Audit Trail CI | 🔄 CI/CD | [02-feature-modules/config/config-audit-trail-ci.md](02-feature-modules/config/config-audit-trail-ci.md) |
| Config Deprecation Aggregation CI | 🔄 CI/CD | [02-feature-modules/config/config-deprecation-aggregation-ci.md](02-feature-modules/config/config-deprecation-aggregation-ci.md) |
| Config File Watcher CI | 🔄 CI/CD | [02-feature-modules/config/config-file-watcher-ci.md](02-feature-modules/config/config-file-watcher-ci.md) |
| LLM CPU Fallback CI | 🔄 CI/CD | [02-feature-modules/llm/llm-cpu-fallback-ci.md](02-feature-modules/llm/llm-cpu-fallback-ci.md) |
| LLM CUDA GPU CI | 🔄 CI/CD | [02-feature-modules/llm/llm-cuda-gpu-ci.md](02-feature-modules/llm/llm-cuda-gpu-ci.md) |
| LLM Deployment Plugin RocksDB CI | 🔄 CI/CD | [02-feature-modules/llm/llm-deployment-rocksdb-ci.md](02-feature-modules/llm/llm-deployment-rocksdb-ci.md) |
| LLM OpenAI-Compat Adapter CI | 🔄 CI/CD | [02-feature-modules/llm/llm-openai-compat-ci.md](02-feature-modules/llm/llm-openai-compat-ci.md) |
| RAG Pipeline CI | 🔄 CI/CD | [02-feature-modules/rag/rag-pipeline-ci.md](02-feature-modules/rag/rag-pipeline-ci.md) |
| Adaptive Shard Rebalancer CI | 🔄 CI/CD | [02-feature-modules/replication/adaptive-shard-rebalancer-ci.md](02-feature-modules/replication/adaptive-shard-rebalancer-ci.md) |
| Bidirectional Replication CI | 🔄 CI/CD | [02-feature-modules/replication/bidirectional-replication-ci.md](02-feature-modules/replication/bidirectional-replication-ci.md) |
| Compressed Replication CI | 🔄 CI/CD | [02-feature-modules/replication/compressed-replication-ci.md](02-feature-modules/replication/compressed-replication-ci.md) |
| Geo-Replication Consistency Levels CI | 🔄 CI/CD | [02-feature-modules/replication/geo-replication-consistency-ci.md](02-feature-modules/replication/geo-replication-consistency-ci.md) |
| Logical Replication – Parallel Decoding CI | 🔄 CI/CD | [02-feature-modules/replication/logical-replication-parallel-decoding-ci.md](02-feature-modules/replication/logical-replication-parallel-decoding-ci.md) |
| Multi-Tier Replication CI | 🔄 CI/CD | [02-feature-modules/replication/multi-tier-replication-ci.md](02-feature-modules/replication/multi-tier-replication-ci.md) |
| Async Retry Back-Off CI | 🔄 CI/CD | [02-feature-modules/resilience/async-retry-backoff-ci.md](02-feature-modules/resilience/async-retry-backoff-ci.md) |
| Async Retry RemoteRegistryClient CI | 🔄 CI/CD | [02-feature-modules/resilience/async-retry-remote-registry-ci.md](02-feature-modules/resilience/async-retry-remote-registry-ci.md) |
| RedisCacheCoordinator Async Pub/Sub CI | 🔄 CI/CD | [02-feature-modules/resilience/redis-cache-coordinator-async-loop-ci.md](02-feature-modules/resilience/redis-cache-coordinator-async-loop-ci.md) |
| Remote Registry Client – Async Retry CI | 🔄 CI/CD | [02-feature-modules/resilience/remote-registry-client-async-retry-ci.md](02-feature-modules/resilience/remote-registry-client-async-retry-ci.md) |
| Arrow User Registration Plugin CI | 🔄 CI/CD | [02-feature-modules/security/arrow-user-registration-plugin-ci.md](02-feature-modules/security/arrow-user-registration-plugin-ci.md) |
| Auth Middleware JWT Scope Enforcement CI | 🔄 CI/CD | [02-feature-modules/security/auth-middleware-jwt-scope-ci.md](02-feature-modules/security/auth-middleware-jwt-scope-ci.md) |
| Adaptive Compaction CI | 🔄 CI/CD | [02-feature-modules/storage/adaptive-compaction-ci.md](02-feature-modules/storage/adaptive-compaction-ci.md) |
| Automatic Schema Migration CI | 🔄 CI/CD | [02-feature-modules/storage/automatic-schema-migration-ci.md](02-feature-modules/storage/automatic-schema-migration-ci.md) |
| Binary Delta Patches CI | 🔄 CI/CD | [02-feature-modules/storage/binary-delta-patches-ci.md](02-feature-modules/storage/binary-delta-patches-ci.md) |
| BlobRedundancyManager – RocksDB Event Listener CI | 🔄 CI/CD | [02-feature-modules/storage/blob-redundancy-event-listener-ci.md](02-feature-modules/storage/blob-redundancy-event-listener-ci.md) |
| Cache Warmup Parallel Bulk Load CI | 🔄 CI/CD | [02-feature-modules/storage/cache-warmup-parallel-bulk-load-ci.md](02-feature-modules/storage/cache-warmup-parallel-bulk-load-ci.md) |
| Erasure Coding for Blob Storage CI | 🔄 CI/CD | [02-feature-modules/storage/erasure-coding-blob-storage-ci.md](02-feature-modules/storage/erasure-coding-blob-storage-ci.md) |
| Gorilla Buffer CI | 🔄 CI/CD | [02-feature-modules/storage/gorilla-buffer-ci.md](02-feature-modules/storage/gorilla-buffer-ci.md) |
| Index Compression CI | 🔄 CI/CD | [02-feature-modules/storage/index-compression-ci.md](02-feature-modules/storage/index-compression-ci.md) |
| ManifestDatabase File Deletion CI | 🔄 CI/CD | [02-feature-modules/storage/manifest-database-file-deletion-ci.md](02-feature-modules/storage/manifest-database-file-deletion-ci.md) |
| NVMe Manager CI | 🔄 CI/CD | [02-feature-modules/storage/nvme-manager-ci.md](02-feature-modules/storage/nvme-manager-ci.md) |
| Online Schema Migration CI | 🔄 CI/CD | [02-feature-modules/storage/online-schema-migration-ci.md](02-feature-modules/storage/online-schema-migration-ci.md) |
| OrphanDetector DistributedCoordinator Wiring CI | 🔄 CI/CD | [02-feature-modules/storage/orphan-detector-wired-ci.md](02-feature-modules/storage/orphan-detector-wired-ci.md) |
| Parallel File Downloads CI | 🔄 CI/CD | [02-feature-modules/storage/parallel-file-downloads-ci.md](02-feature-modules/storage/parallel-file-downloads-ci.md) |
| RocksDBWrapper Size Calculation CI | 🔄 CI/CD | [02-feature-modules/storage/rocksdb-size-calculation-ci.md](02-feature-modules/storage/rocksdb-size-calculation-ci.md) |
| RS Repair Engine Parallelisation CI | 🔄 CI/CD | [02-feature-modules/storage/rs-repair-engine-parallelisation-ci.md](02-feature-modules/storage/rs-repair-engine-parallelisation-ci.md) |
| TSStore Out-of-Order Write CI | 🔄 CI/CD | [02-feature-modules/storage/tsstore-out-of-order-ci.md](02-feature-modules/storage/tsstore-out-of-order-ci.md) |
| Write-Optimized Merge (WOM) Tree CI | 🔄 CI/CD | [02-feature-modules/storage/wom-tree-ci.md](02-feature-modules/storage/wom-tree-ci.md) |
| Zero-Copy Blob Transfers CI | 🔄 CI/CD | [02-feature-modules/storage/zero-copy-blob-transfers-ci.md](02-feature-modules/storage/zero-copy-blob-transfers-ci.md) |
| Adaptive Deadlock Prevention CI | 🔄 CI/CD | [02-feature-modules/transactions/adaptive-deadlock-prevention-ci.md](02-feature-modules/transactions/adaptive-deadlock-prevention-ci.md) |
| Percolator Distributed Transaction Coordinator CI | 🔄 CI/CD | [02-feature-modules/transactions/percolator-distributed-transaction-coordinator-ci.md](02-feature-modules/transactions/percolator-distributed-transaction-coordinator-ci.md) |
| Distributed Transaction Coordinator (2PC) CI | 🔄 CI/CD | [02-feature-modules/transactions/transaction-distributed-2pc-ci.md](02-feature-modules/transactions/transaction-distributed-2pc-ci.md) |
| Transaction OCC CI | 🔄 CI/CD | [02-feature-modules/transactions/transaction-occ-ci.md](02-feature-modules/transactions/transaction-occ-ci.md) |
| SAGA Orchestration Engine CI | 🔄 CI/CD | [02-feature-modules/transactions/transaction-saga-orchestration-ci.md](02-feature-modules/transactions/transaction-saga-orchestration-ci.md) |
| Transaction Savepoints CI | 🔄 CI/CD | [02-feature-modules/transactions/transaction-savepoints-ci.md](02-feature-modules/transactions/transaction-savepoints-ci.md) |
| Transaction SSI CI | 🔄 CI/CD | [02-feature-modules/transactions/transaction-ssi-ci.md](02-feature-modules/transactions/transaction-ssi-ci.md) |

## 03 · Editions

| Workflow | Typ | Dokumentation |
|----------|-----|---------------|
| Edition Build CI (reusable) | ♻️ Reusable | [03-editions/edition-build-ci.md](03-editions/edition-build-ci.md) |
| Edition · COMMUNITY · CI | 🔄 CI/CD | [03-editions/edition-community-ci.md](03-editions/edition-community-ci.md) |
| Edition · ENTERPRISE · CI | 🔄 CI/CD | [03-editions/edition-enterprise-ci.md](03-editions/edition-enterprise-ci.md) |
| Edition · HYPERSCALER · CI | 🔄 CI/CD | [03-editions/edition-hyperscaler-ci.md](03-editions/edition-hyperscaler-ci.md) |
| Edition · MILITARY · CI | 🔄 CI/CD | [03-editions/edition-military-ci.md](03-editions/edition-military-ci.md) |
| Edition · MINIMAL · CI | 🔄 CI/CD | [03-editions/edition-minimal-ci.md](03-editions/edition-minimal-ci.md) |

## 04 · Release

| Workflow | Typ | Dokumentation |
|----------|-----|---------------|
| [Manual] Bootstrap Release Branches | 🖱️ Manuell | [04-release/bootstrap-release-branches.md](04-release/bootstrap-release-branches.md) |
| Binary Package Layout & Installer Policy | 📐 Standard | [04-release/binary-package-layout.md](04-release/binary-package-layout.md) |
| Build Binary Release · Linux | 🔄 CI/CD | [04-release/build-binary-linux.md](04-release/build-binary-linux.md) |
| Build Binary Release · Windows | 🔄 CI/CD | [04-release/build-binary-windows.md](04-release/build-binary-windows.md) |
| Canary Deployments CI | 🔄 CI/CD | [04-release/canary-deployments-ci.md](04-release/canary-deployments-ci.md) |
| [Manual] Create Release Archive | 🖱️ Manuell | [04-release/create-release-archive.md](04-release/create-release-archive.md) |
| Docker Image CI | 🔄 CI/CD | [04-release/docker-image.md](04-release/docker-image.md) |
| Publish Docker image to Docker Hub (on GitHub Release) | 🔄 CI/CD | [04-release/dockerhub-publish-on-release.md](04-release/dockerhub-publish-on-release.md) |
| Publish · Enterprise Edition | 🔄 CI/CD | [04-release/publish-enterprise.md](04-release/publish-enterprise.md) |
| Publish · Hyperscaler Edition | 🔄 CI/CD | [04-release/publish-hyperscaler.md](04-release/publish-hyperscaler.md) |

## 05 · Quality

| Workflow | Typ | Dokumentation |
|----------|-----|---------------|
| Build Reproducibility CI | 🔄 CI/CD | [05-quality/build/build-reproducibility-ci.md](05-quality/build/build-reproducibility-ci.md) |
| Cross-Module Performance Regression CI | ⏰ Geplant | [05-quality/build/cross-module-performance-regression-ci.md](05-quality/build/cross-module-performance-regression-ci.md) |
| Error-Handling Audit | ⏰ Geplant | [05-quality/build/error-handling-audit.md](05-quality/build/error-handling-audit.md) |
| Chunk-Level Encryption at Rest CI | 🔄 CI/CD | [05-quality/security/chunk-level-encryption-ci.md](05-quality/security/chunk-level-encryption-ci.md) |
| LoRA Certificate Store CI | 🔄 CI/CD | [05-quality/security/lora-certificate-store-ci.md](05-quality/security/lora-certificate-store-ci.md) |
| PII Redaction Policy Check | 🔄 CI/CD | [05-quality/security/pii-redaction-check.md](05-quality/security/pii-redaction-check.md) |
| PKI Stub Verification CI | 🔄 CI/CD | [05-quality/security/pki-stub-verification-ci.md](05-quality/security/pki-stub-verification-ci.md) |
| Secret Scanning CI | 🔄 CI/CD | [05-quality/security/secret-scanning-ci.md](05-quality/security/secret-scanning-ci.md) |
| Security Hardening CI | 🔄 CI/CD | [05-quality/security/security-hardening-ci.md](05-quality/security/security-hardening-ci.md) |
| Security Signature RocksDB Iteration CI | 🔄 CI/CD | [05-quality/security/security-signature-rocksdb-iteration-ci.md](05-quality/security/security-signature-rocksdb-iteration-ci.md) |
| Documentation Validation | ♻️ Reusable | [05-quality/validation/documentation-validation.md](05-quality/validation/documentation-validation.md) |
| Research Documentation Validation | ⏰ Geplant | [05-quality/validation/research-validation.md](05-quality/validation/research-validation.md) |
| Validate AI-Guardrails | 🔄 CI/CD | [05-quality/validation/validate-ai-guardrails.md](05-quality/validation/validate-ai-guardrails.md) |
| Validate Config Mapping | 🔄 CI/CD | [05-quality/validation/validate-config-mapping.md](05-quality/validation/validate-config-mapping.md) |
| Validate Grafana Dashboards | 🔄 CI/CD | [05-quality/validation/validate-grafana-dashboards.md](05-quality/validation/validate-grafana-dashboards.md) |
| Validate Roadmap | 🔄 CI/CD | [05-quality/validation/validate-roadmap.md](05-quality/validation/validate-roadmap.md) |

## 06 · Infrastructure

| Workflow | Typ | Dokumentation |
|----------|-----|---------------|
| CrossShard Coordinator ID + Compensation RPC CI | 🔄 CI/CD | [06-infrastructure/distributed/cross-shard-coordinator-id-compensation-rpc-ci.md](06-infrastructure/distributed/cross-shard-coordinator-id-compensation-rpc-ci.md) |
| Distributed Cache Integration CI | 🔄 CI/CD | [06-infrastructure/distributed/distributed-cache-integration-ci.md](06-infrastructure/distributed/distributed-cache-integration-ci.md) |
| Distributed Cluster Updates CI | 🔄 CI/CD | [06-infrastructure/distributed/distributed-cluster-updates-ci.md](06-infrastructure/distributed/distributed-cluster-updates-ci.md) |
| Epoch Fencing CI (Phase 4.1) | 🔄 CI/CD | [06-infrastructure/distributed/epoch-fencing-ci.md](06-infrastructure/distributed/epoch-fencing-ci.md) |
| HttpServer ShardingManager CI | 🔄 CI/CD | [06-infrastructure/distributed/httpserver-shardingmanager-ci.md](06-infrastructure/distributed/httpserver-shardingmanager-ci.md) |
| Load Balancing with Raft Coordination CI | 🔄 CI/CD | [06-infrastructure/distributed/load-balancing-raft-coordination-ci.md](06-infrastructure/distributed/load-balancing-raft-coordination-ci.md) |
| Shard RPC Integration CI | 🔄 CI/CD | [06-infrastructure/distributed/shard-rpc-integration-ci.md](06-infrastructure/distributed/shard-rpc-integration-ci.md) |
| Sharding Focused Tests CI | 🔄 CI/CD | [06-infrastructure/distributed/sharding-focused-tests-ci.md](06-infrastructure/distributed/sharding-focused-tests-ci.md) |
| GPU Module CI Gate | 🔄 CI/CD | [06-infrastructure/gpu/gpu-ci.md](06-infrastructure/gpu/gpu-ci.md) |
| GPU Memory Oversubscription CI | 🔄 CI/CD | [06-infrastructure/gpu/gpu-memory-oversubscription-ci.md](06-infrastructure/gpu/gpu-memory-oversubscription-ci.md) |
| OpenCL ErasureCoder Parity CI | 🔄 CI/CD | [06-infrastructure/gpu/opencl-erasure-coder-parity-ci.md](06-infrastructure/gpu/opencl-erasure-coder-parity-ci.md) |
| Vulkan Compute Shader Pipeline CI | 🔄 CI/CD | [06-infrastructure/gpu/vulkan-compute-shader-pipeline-ci.md](06-infrastructure/gpu/vulkan-compute-shader-pipeline-ci.md) |
| API Gateway Enhancements CI | 🔄 CI/CD | [06-infrastructure/networking/api-gateway-enhancements-ci.md](06-infrastructure/networking/api-gateway-enhancements-ci.md) |
| Bandwidth Management and QoS CI | 🔄 CI/CD | [06-infrastructure/networking/bandwidth-management-qos-ci.md](06-infrastructure/networking/bandwidth-management-qos-ci.md) |
| UDP Server (Ingestion) CI | 🔄 CI/CD | [06-infrastructure/networking/udp-server-ci.md](06-infrastructure/networking/udp-server-ci.md) |
| Wire Protocol Optimizations CI | 🔄 CI/CD | [06-infrastructure/networking/wire-protocol-optimizations-ci.md](06-infrastructure/networking/wire-protocol-optimizations-ci.md) |
| Wire Protocol V2 CI | 🔄 CI/CD | [06-infrastructure/networking/wire-protocol-v2-ci.md](06-infrastructure/networking/wire-protocol-v2-ci.md) |
| MetricsCollector Shared Mutex CI | 🔄 CI/CD | [06-infrastructure/observability/metrics-collector-shared-mutex-ci.md](06-infrastructure/observability/metrics-collector-shared-mutex-ci.md) |
| ML Anomaly Detector CI | 🔄 CI/CD | [06-infrastructure/observability/ml-anomaly-detector-ci.md](06-infrastructure/observability/ml-anomaly-detector-ci.md) |
| Request Tracing and Correlation IDs CI | 🔄 CI/CD | [06-infrastructure/observability/request-tracing-correlation-ids-ci.md](06-infrastructure/observability/request-tracing-correlation-ids-ci.md) |
| Root Cause Analyzer CI | 🔄 CI/CD | [06-infrastructure/observability/root-cause-analyzer-ci.md](06-infrastructure/observability/root-cause-analyzer-ci.md) |
| Statistics Collector CI | 🔄 CI/CD | [06-infrastructure/observability/statistics-collector-ci.md](06-infrastructure/observability/statistics-collector-ci.md) |

## 07 · Data Pipelines

| Workflow | Typ | Dokumentation |
|----------|-----|---------------|
| CDC WebSocket Streaming CI | 🔄 CI/CD | [07-data-pipelines/cdc-websocket-streaming-ci.md](07-data-pipelines/cdc-websocket-streaming-ci.md) |
| CDC Consumer Group Semantics CI | 🔄 CI/CD | [07-data-pipelines/consumer-group-semantics-ci.md](07-data-pipelines/consumer-group-semantics-ci.md) |
| Content Embedding Pipeline CI | 🔄 CI/CD | [07-data-pipelines/content-embedding-pipeline-ci.md](07-data-pipelines/content-embedding-pipeline-ci.md) |
| Importer Module Tests | 🔄 CI/CD | [07-data-pipelines/importer-tests.md](07-data-pipelines/importer-tests.md) |
| Ingestion Module Tests | 🔄 CI/CD | [07-data-pipelines/ingestion-tests.md](07-data-pipelines/ingestion-tests.md) |
| Kafka Consumer Source Connector CI | 🔄 CI/CD | [07-data-pipelines/kafka-consumer-source-connector-ci.md](07-data-pipelines/kafka-consumer-source-connector-ci.md) |
| Kafka Importer CI | 🔄 CI/CD | [07-data-pipelines/kafka-importer-ci.md](07-data-pipelines/kafka-importer-ci.md) |
| S3-Compatible Object Storage Source Connector CI | 🔄 CI/CD | [07-data-pipelines/s3-compatible-object-storage-connector-ci.md](07-data-pipelines/s3-compatible-object-storage-connector-ci.md) |
| WAL Archival to Object Storage CI | 🔄 CI/CD | [07-data-pipelines/wal-archival-object-storage-ci.md](07-data-pipelines/wal-archival-object-storage-ci.md) |

## 08 · Maintenance

| Workflow | Typ | Dokumentation |
|----------|-----|---------------|
| Acceleration ROADMAP Audit | 🔄 CI/CD | [08-maintenance/acceleration-roadmap-audit.md](08-maintenance/acceleration-roadmap-audit.md) |
| [Manual] Add Documentation Metadata | 🖱️ Manuell | [08-maintenance/add-doc-metadata.md](08-maintenance/add-doc-metadata.md) |
| Auto Label | 🔄 CI/CD | [08-maintenance/auto-label.md](08-maintenance/auto-label.md) |
| Classify Bridge CI | 🔄 CI/CD | [08-maintenance/classify-bridge-ci.md](08-maintenance/classify-bridge-ci.md) |
| Code Maturity Analysis & Auto-Versioning | 🔄 CI/CD | [08-maintenance/code-maturity-analysis.md](08-maintenance/code-maturity-analysis.md) |
| Data Augmentation Pipeline Tests | 🔄 CI/CD | [08-maintenance/data-augmentation-tests.md](08-maintenance/data-augmentation-tests.md) |
| Docs Orphan Check | 🔄 CI/CD | [08-maintenance/docs-orphan-check.md](08-maintenance/docs-orphan-check.md) |
| [Manual] Label Governance - Setup & Audit | 🖱️ Manuell | [08-maintenance/github_workflows_label-governance.md](08-maintenance/github_workflows_label-governance.md) |
| Module Docs Sync | 🔄 CI/CD | [08-maintenance/module-docs-sync.md](08-maintenance/module-docs-sync.md) |
| OpenAPI SDK Generation | 🔄 CI/CD | [08-maintenance/sdk-generation.md](08-maintenance/sdk-generation.md) |
| Root Docs Hygiene | 🔄 CI/CD | [08-maintenance/root-docs-hygiene.md](08-maintenance/root-docs-hygiene.md) |

## 09 · PR Gates

| Workflow | Typ | Dokumentation |
|----------|-----|---------------|
| PR Path Gate · enterprise | 🔄 CI/CD | [09-pr-gates/pr-path-gate-enterprise.md](09-pr-gates/pr-path-gate-enterprise.md) |
| PR Path Gate · hyperscaler | 🔄 CI/CD | [09-pr-gates/pr-path-gate-hyperscaler.md](09-pr-gates/pr-path-gate-hyperscaler.md) |
| PR Path Gate · main (Community) | 🔄 CI/CD | [09-pr-gates/pr-path-gate-main.md](09-pr-gates/pr-path-gate-main.md) |
| PR Quick Checks | 🔄 CI/CD | [09-pr-gates/pr-quick-checks.md](09-pr-gates/pr-quick-checks.md) |

## Docs

| Workflow | Typ | Dokumentation |
|----------|-----|---------------|
| Doc Metadata Check | 🔄 CI/CD | [docs/doc-metadata-check.md](docs/doc-metadata-check.md) |
| Documentation Pipeline | 🔄 CI/CD | [docs/docs-pipeline.md](docs/docs-pipeline.md) |
| Primary-Docs Index Generator | 🔄 CI/CD | [docs/primary-docs-index.md](docs/primary-docs-index.md) |

## Root-Level Workflows

| Workflow | Typ | Dokumentation |
|----------|-----|---------------|
| IndexRecommender Access-Pattern Persistence CI | 🔄 CI/CD | [access-pattern-persistence-ci.md](access-pattern-persistence-ci.md) |
| AQL v1.8.0 Interfaces CI | 🔄 CI/CD | [aql-v1.8-interfaces-ci.md](aql-v1.8-interfaces-ci.md) |
| AsyncIngestionWorker YAML Config CI | 🔄 CI/CD | [async-ingestion-worker-yaml-config-ci.md](async-ingestion-worker-yaml-config-ci.md) |
| BackendRegistry Thread-Safety CI | 🔄 CI/CD | [backend-registry-thread-safety-ci.md](backend-registry-thread-safety-ci.md) |
| CDC Changefeed Sequence Counter CI | 🔄 CI/CD | [cdc-changefeed-sequence-counter-ci.md](cdc-changefeed-sequence-counter-ci.md) |
| CDC Interfaces CI | 🔄 CI/CD | [cdc-interfaces-ci.md](cdc-interfaces-ci.md) |
| CEP Lock-Held-Across-Callbacks CI | 🔄 CI/CD | [cep-lock-held-callbacks-ci.md](cep-lock-held-callbacks-ci.md) |
| Chimera Retry Policy CI | 🔄 CI/CD | [chimera-retry-policy-ci.md](chimera-retry-policy-ci.md) |
| CI Scope Classifier | ♻️ Reusable | [ci-scope-classifier.md](ci-scope-classifier.md) |
| ConfigEncryptedStore Lock Upgrade CI | 🔄 CI/CD | [config-encrypted-store-lock-upgrade-ci.md](config-encrypted-store-lock-upgrade-ci.md) |
| Content Abuse Detection CI | 🔄 CI/CD | [content-abuse-detection-ci.md](content-abuse-detection-ci.md) |
| Content Deduplication via Perceptual Hashing CI | 🔄 CI/CD | [content-dedup-perceptual-hashing-ci.md](content-dedup-perceptual-hashing-ci.md) |
| Prompt Engineering – CoT Step Tracer CI | 🔄 CI/CD | [cot-tracer-ci.md](cot-tracer-ci.md) |
| CSV Export for Compliance Reports CI | 🔄 CI/CD | [csv-export-compliance-reporting-ci.md](csv-export-compliance-reporting-ci.md) |
| Cypher/Gremlin Parser CI | 🔄 CI/CD | [cypher-gremlin-parser-ci.md](cypher-gremlin-parser-ci.md) |
| DiffEngine Cache Stampede CI | 🔄 CI/CD | [diff-engine-cache-stampede-ci.md](diff-engine-cache-stampede-ci.md) |
| DistributedAnalytics Healthy Shard Count CI | 🔄 CI/CD | [distributed-analytics-healthy-shard-count-ci.md](distributed-analytics-healthy-shard-count-ci.md) |
| DistributedGraphManager Shared Mutex CI | 🔄 CI/CD | [distributed-graph-shared-mutex-ci.md](distributed-graph-shared-mutex-ci.md) |
| Distributed Ingestion Coordinator CI | 🔄 CI/CD | [distributed-ingestion-coordinator-ci.md](distributed-ingestion-coordinator-ci.md) |
| Prompt Engineering – DSPy Module CI | 🔄 CI/CD | [dspy-module-ci.md](dspy-module-ci.md) |
| eID Authenticator CI | 🔄 CI/CD | [eid-authenticator-ci.md](eid-authenticator-ci.md) |
| Analytics – Forecasting Batch/Streaming CI | 🔄 CI/CD | [forecasting-batch-streaming-ci.md](forecasting-batch-streaming-ci.md) |
| Geo Point Clustering (DBSCAN / k-means) CI | 🔄 CI/CD | [geo-point-clustering-dbscan-kmeans-ci.md](geo-point-clustering-dbscan-kmeans-ci.md) |
| Governance Module CI | 🔄 CI/CD | [governance-module-ci.md](governance-module-ci.md) |
| GraphQL WebSocket Handler — CDC Callback Safety CI | 🔄 CI/CD | [graphql-ws-handler-cdc-callback-safety-ci.md](graphql-ws-handler-cdc-callback-safety-ci.md) |
| Hardware-Accelerated Query Execution CI | 🔄 CI/CD | [hardware-accelerated-query-execution-ci.md](hardware-accelerated-query-execution-ci.md) |
| IncrementalView Micro-Batch Lock CI | 🔄 CI/CD | [incremental-view-micro-batch-lock-ci.md](incremental-view-micro-batch-lock-ci.md) |
| Ingestion LLM Adapter CI | 🔄 CI/CD | [ingestion-llm-adapter-ci.md](ingestion-llm-adapter-ci.md) |
| Join Exporter CI | 🔄 CI/CD | [join-exporter-ci.md](join-exporter-ci.md) |
| LLM Deployment Plugin RocksDB Storage CI | 🔄 CI/CD | [llm-deployment-plugin-rocksdb-storage-ci.md](llm-deployment-plugin-rocksdb-storage-ci.md) |
| LLMProcessAnalyzer LRU Cache CI | 🔄 CI/CD | [llm-process-analyzer-lru-cache-ci.md](llm-process-analyzer-lru-cache-ci.md) |
| LoRA Adapter Hot-Loading CI | 🔄 CI/CD | [lora-adapter-hot-loading-ci.md](lora-adapter-hot-loading-ci.md) |
| Matryoshka Truncation CI | 🔄 CI/CD | [matryoshka-truncation-ci.md](matryoshka-truncation-ci.md) |
| Metadata Interfaces CI | 🔄 CI/CD | [metadata-interfaces-ci.md](metadata-interfaces-ci.md) |
| ML Serving TOCTOU + Lock Fix CI | 🔄 CI/CD | [ml-serving-toctou-lock-ci.md](ml-serving-toctou-lock-ci.md) |
| ModelServingEngine Inference Under Registry Lock CI | 🔄 CI/CD | [model-serving-inference-lock-ci.md](model-serving-inference-lock-ci.md) |
| MQTT Client Service CI | 🔄 CI/CD | [mqtt-client-service-ci.md](mqtt-client-service-ci.md) |
| OLAP LRU Cache CI | 🔄 CI/CD | [olap-lru-cache-ci.md](olap-lru-cache-ci.md) |
| OZG Service Registry CI | 🔄 CI/CD | [ozg-service-registry-ci.md](ozg-service-registry-ci.md) |
| Plugin Manager & Lifecycle CI | 🔄 CI/CD | [plugin-manager-lifecycle-ci.md](plugin-manager-lifecycle-ci.md) |
| Plugin Security CRL/OCSP CI | 🔄 CI/CD | [plugin-security-crl-ocsp-ci.md](plugin-security-crl-ocsp-ci.md) |
| Plugin Security PE Certificate Extraction CI | 🔄 CI/CD | [plugin-security-pe-cert-extraction-ci.md](plugin-security-pe-cert-extraction-ci.md) |
| Predictive Prefetcher ML CI | 🔄 CI/CD | [predictive-prefetcher-ml-ci.md](predictive-prefetcher-ml-ci.md) |
| Process Discovery & Conformance Checking CI | 🔄 CI/CD | [process-discovery-conformance-ci.md](process-discovery-conformance-ci.md) |
| Prompt Engineering – A/B Experiment Framework CI | 🔄 CI/CD | [prompt-ab-experiment-ci.md](prompt-ab-experiment-ci.md) |
| Prompt Engineering – Context Window Budget Manager CI | 🔄 CI/CD | [prompt-engineering-context-window-ci.md](prompt-engineering-context-window-ci.md) |
| Prompt Engineering – Library Import/Export CI | 🔄 CI/CD | [prompt-library-io-ci.md](prompt-library-io-ci.md) |
| Prompt Engineering – Prompt Regression Runner CI | 🔄 CI/CD | [prompt-regression-runner-ci.md](prompt-regression-runner-ci.md) |
| Prompt Engineering – ProTeGi Optimizer CI | 🔄 CI/CD | [protegi-optimizer-ci.md](protegi-optimizer-ci.md) |
| Prompt Engineering – Reflection Integration CI | 🔄 CI/CD | [reflection-integration-ci.md](reflection-integration-ci.md) |
| Prompt Engineering – Reflection Tuning CI | 🔄 CI/CD | [reflection-tuner-ci.md](reflection-tuner-ci.md) |
| SAGA Orchestration Engine CI | 🔄 CI/CD | [saga-orchestration-engine-ci.md](saga-orchestration-engine-ci.md) |
| Schema Version Manager CI | 🔄 CI/CD | [schema-version-manager-ci.md](schema-version-manager-ci.md) |
| SIMD Vectorization — AVX-512 and ARM NEON CI | 🔄 CI/CD | [simd-vectorization-avx512-arm-neon-ci.md](simd-vectorization-avx512-arm-neon-ci.md) |
| Temporal Phase 4 CI | 🔄 CI/CD | [temporal-phase4-ci.md](temporal-phase4-ci.md) |
| Transaction Audit Trail CI | 🔄 CI/CD | [transaction-audit-trail-ci.md](transaction-audit-trail-ci.md) |
| Transaction Write Batching CI | 🔄 CI/CD | [transaction-write-batching-ci.md](transaction-write-batching-ci.md) |
| Prompt Engineering – Tree-of-Thoughts Reasoner CI | 🔄 CI/CD | [tree-of-thoughts-ci.md](tree-of-thoughts-ci.md) |
| User Storage Encrypted CI | 🔄 CI/CD | [user-storage-encrypted-ci.md](user-storage-encrypted-ci.md) |
| Versioned API Routing CI | 🔄 CI/CD | [versioned-api-routing-ci.md](versioned-api-routing-ci.md) |
| Voice Module CI | 🔄 CI/CD | [voice-module-ci.md](voice-module-ci.md) |
| XDOMEA Connector CI | 🔄 CI/CD | [xdomea-connector-ci.md](xdomea-connector-ci.md) |
| XÖV Importer CI | 🔄 CI/CD | [xoev-importer-ci.md](xoev-importer-ci.md) |
