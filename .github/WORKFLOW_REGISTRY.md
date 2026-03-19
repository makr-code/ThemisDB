# ThemisDB Workflow Registry

Complete mapping of all GitHub Actions workflows organized by functional category.

## Directory Structure

```
.github/workflows/
├── _templates/                          # Reusable workflow templates
├── 01-core/                             # Core CI (2 workflows)
├── 02-feature-modules/                  # Feature-specific CI (62 workflows)
│   ├── acceleration/
│   ├── adaptive-query/
│   ├── chimera/
│   ├── config/
│   ├── llm/
│   ├── replication/
│   ├── resilience/
│   ├── security/
│   ├── storage/
│   └── transactions/
├── 03-editions/                         # Product editions (6 workflows)
├── 04-release/                          # Release & deployment (6 workflows)
├── 05-quality/                          # Quality gates (16 workflows)
│   ├── build/
│   ├── security/
│   └── validation/
├── 06-infrastructure/                   # System infrastructure (19 workflows)
│   ├── distributed/
│   ├── gpu/
│   ├── networking/
│   └── observability/
├── 07-data-pipelines/                   # Data processing (9 workflows)
├── 08-maintenance/                      # Repository maintenance (12 workflows)
├── 09-pr-gates/                         # PR quality gates (4 workflows)
└── docs/                                # Documentation workflows (2 workflows)
```

---

## 01-core (2 workflows)

| Workflow | Path | Description |
|----------|------|-------------|
| themis-core-ci.yml | `01-core/themis-core-ci.yml` | Core ThemisDB CI pipeline |
| ci-scope-classifier.yml | `01-core/ci-scope-classifier.yml` | Reusable scope classifier for downstream workflows |

---

## 02-feature-modules (62 workflows)

### acceleration (2)

| Workflow | Path |
|----------|------|
| acceleration-benchmark-ci.yml | `02-feature-modules/acceleration/acceleration-benchmark-ci.yml` |
| runtime-device-capability-negotiation-ci.yml | `02-feature-modules/acceleration/runtime-device-capability-negotiation-ci.yml` |

### adaptive-query (9)

| Workflow | Path |
|----------|------|
| adaptive-join-strategies-ci.yml | `02-feature-modules/adaptive-query/adaptive-join-strategies-ci.yml` |
| adaptive-query-compilation-ci.yml | `02-feature-modules/adaptive-query/adaptive-query-compilation-ci.yml` |
| geo-spatial-join-ci.yml | `02-feature-modules/adaptive-query/geo-spatial-join-ci.yml` |
| intelligent-prefetching-ci.yml | `02-feature-modules/adaptive-query/intelligent-prefetching-ci.yml` |
| materialized-views-incremental-maintenance-ci.yml | `02-feature-modules/adaptive-query/materialized-views-incremental-maintenance-ci.yml` |
| parallel-query-execution-intra-query-ci.yml | `02-feature-modules/adaptive-query/parallel-query-execution-intra-query-ci.yml` |
| query-jit-compilation-ci.yml | `02-feature-modules/adaptive-query/query-jit-compilation-ci.yml` |
| query-plan-caching-ci.yml | `02-feature-modules/adaptive-query/query-plan-caching-ci.yml` |
| temporal-spatial-queries-ci.yml | `02-feature-modules/adaptive-query/temporal-spatial-queries-ci.yml` |

### chimera (3)

| Workflow | Path |
|----------|------|
| chimera-benchmark-ci.yml | `02-feature-modules/chimera/chimera-benchmark-ci.yml` |
| chimera-error-recovery-retry-ci.yml | `02-feature-modules/chimera/chimera-error-recovery-retry-ci.yml` |
| chimera-integration-ci.yml | `02-feature-modules/chimera/chimera-integration-ci.yml` |

### config (3)

| Workflow | Path |
|----------|------|
| config-audit-trail-ci.yml | `02-feature-modules/config/config-audit-trail-ci.yml` |
| config-deprecation-aggregation-ci.yml | `02-feature-modules/config/config-deprecation-aggregation-ci.yml` |
| config-file-watcher-ci.yml | `02-feature-modules/config/config-file-watcher-ci.yml` |

### llm (3)

| Workflow | Path |
|----------|------|
| llm-cpu-fallback-ci.yml | `02-feature-modules/llm/llm-cpu-fallback-ci.yml` |
| llm-cuda-gpu-ci.yml | `02-feature-modules/llm/llm-cuda-gpu-ci.yml` |
| llm-openai-compat-ci.yml | `02-feature-modules/llm/llm-openai-compat-ci.yml` |

### replication (5)

| Workflow | Path |
|----------|------|
| adaptive-shard-rebalancer-ci.yml | `02-feature-modules/replication/adaptive-shard-rebalancer-ci.yml` |
| bidirectional-replication-ci.yml | `02-feature-modules/replication/bidirectional-replication-ci.yml` |
| compressed-replication-ci.yml | `02-feature-modules/replication/compressed-replication-ci.yml` |
| geo-replication-consistency-ci.yml | `02-feature-modules/replication/geo-replication-consistency-ci.yml` |
| multi-tier-replication-ci.yml | `02-feature-modules/replication/multi-tier-replication-ci.yml` |

### resilience (4)

| Workflow | Path |
|----------|------|
| async-retry-backoff-ci.yml | `02-feature-modules/resilience/async-retry-backoff-ci.yml` |
| async-retry-remote-registry-ci.yml | `02-feature-modules/resilience/async-retry-remote-registry-ci.yml` |
| redis-cache-coordinator-async-loop-ci.yml | `02-feature-modules/resilience/redis-cache-coordinator-async-loop-ci.yml` |
| remote-registry-client-async-retry-ci.yml | `02-feature-modules/resilience/remote-registry-client-async-retry-ci.yml` |

### security (2)

| Workflow | Path |
|----------|------|
| arrow-user-registration-plugin-ci.yml | `02-feature-modules/security/arrow-user-registration-plugin-ci.yml` |
| auth-middleware-jwt-scope-ci.yml | `02-feature-modules/security/auth-middleware-jwt-scope-ci.yml` |

### storage (16)

| Workflow | Path |
|----------|------|
| adaptive-compaction-ci.yml | `02-feature-modules/storage/adaptive-compaction-ci.yml` |
| automatic-schema-migration-ci.yml | `02-feature-modules/storage/automatic-schema-migration-ci.yml` |
| binary-delta-patches-ci.yml | `02-feature-modules/storage/binary-delta-patches-ci.yml` |
| blob-redundancy-event-listener-ci.yml | `02-feature-modules/storage/blob-redundancy-event-listener-ci.yml` |
| cache-warmup-parallel-bulk-load-ci.yml | `02-feature-modules/storage/cache-warmup-parallel-bulk-load-ci.yml` |
| erasure-coding-blob-storage-ci.yml | `02-feature-modules/storage/erasure-coding-blob-storage-ci.yml` |
| gorilla-buffer-ci.yml | `02-feature-modules/storage/gorilla-buffer-ci.yml` |
| index-compression-ci.yml | `02-feature-modules/storage/index-compression-ci.yml` |
| manifest-database-file-deletion-ci.yml | `02-feature-modules/storage/manifest-database-file-deletion-ci.yml` |
| online-schema-migration-ci.yml | `02-feature-modules/storage/online-schema-migration-ci.yml` |
| orphan-detector-wired-ci.yml | `02-feature-modules/storage/orphan-detector-wired-ci.yml` |
| parallel-file-downloads-ci.yml | `02-feature-modules/storage/parallel-file-downloads-ci.yml` |
| rocksdb-size-calculation-ci.yml | `02-feature-modules/storage/rocksdb-size-calculation-ci.yml` |
| rs-repair-engine-parallelisation-ci.yml | `02-feature-modules/storage/rs-repair-engine-parallelisation-ci.yml` |
| wom-tree-ci.yml | `02-feature-modules/storage/wom-tree-ci.yml` |
| zero-copy-blob-transfers-ci.yml | `02-feature-modules/storage/zero-copy-blob-transfers-ci.yml` |

### transactions (6)

| Workflow | Path |
|----------|------|
| adaptive-deadlock-prevention-ci.yml | `02-feature-modules/transactions/adaptive-deadlock-prevention-ci.yml` |
| percolator-distributed-transaction-coordinator-ci.yml | `02-feature-modules/transactions/percolator-distributed-transaction-coordinator-ci.yml` |
| transaction-distributed-2pc-ci.yml | `02-feature-modules/transactions/transaction-distributed-2pc-ci.yml` |
| transaction-occ-ci.yml | `02-feature-modules/transactions/transaction-occ-ci.yml` |
| transaction-savepoints-ci.yml | `02-feature-modules/transactions/transaction-savepoints-ci.yml` |
| transaction-ssi-ci.yml | `02-feature-modules/transactions/transaction-ssi-ci.yml` |

### root (9)

| Workflow | Path |
|----------|------|
| capabilityautogenerator-persist-state-ci.yml | `02-feature-modules/capabilityautogenerator-persist-state-ci.yml` |
| cli-migration-scanner-ci.yml | `02-feature-modules/cli-migration-scanner-ci.yml` |
| dependency-resolution-engine-ci.yml | `02-feature-modules/dependency-resolution-engine-ci.yml` |
| exporterfactory-stub-replacement-ci.yml | `02-feature-modules/exporterfactory-stub-replacement-ci.yml` |
| multi-tenant-update-scheduling-ci.yml | `02-feature-modules/multi-tenant-update-scheduling-ci.yml` |
| parquet-exporter-tests.yml | `02-feature-modules/parquet-exporter-tests.yml` |
| plugin-registry-shared-mutex-ci.yml | `02-feature-modules/plugin-registry-shared-mutex-ci.yml` |
| process-graph-visit-timestamp-ci.yml | `02-feature-modules/process-graph-visit-timestamp-ci.yml` |
| taskscheduler-auth-context-ci.yml | `02-feature-modules/taskscheduler-auth-context-ci.yml` |

---

## 03-editions (6 workflows)

| Workflow | Path |
|----------|------|
| edition-build-ci.yml | `03-editions/edition-build-ci.yml` |
| edition-community-ci.yml | `03-editions/edition-community-ci.yml` |
| edition-enterprise-ci.yml | `03-editions/edition-enterprise-ci.yml` |
| edition-hyperscaler-ci.yml | `03-editions/edition-hyperscaler-ci.yml` |
| edition-military-ci.yml | `03-editions/edition-military-ci.yml` |
| edition-minimal-ci.yml | `03-editions/edition-minimal-ci.yml` |

---

## 04-release (6 workflows)

| Workflow | Path |
|----------|------|
| bootstrap-release-branches.yml | `04-release/bootstrap-release-branches.yml` |
| canary-deployments-ci.yml | `04-release/canary-deployments-ci.yml` |
| create-release-archive.yml | `04-release/create-release-archive.yml` |
| dockerhub-publish-on-release.yml | `04-release/dockerhub-publish-on-release.yml` |
| publish-enterprise.yml | `04-release/publish-enterprise.yml` |
| publish-hyperscaler.yml | `04-release/publish-hyperscaler.yml` |

---

## 05-quality (16 workflows)

### security (7)

| Workflow | Path |
|----------|------|
| chunk-level-encryption-ci.yml | `05-quality/security/chunk-level-encryption-ci.yml` |
| lora-certificate-store-ci.yml | `05-quality/security/lora-certificate-store-ci.yml` |
| pii-redaction-check.yml | `05-quality/security/pii-redaction-check.yml` |
| pki-stub-verification-ci.yml | `05-quality/security/pki-stub-verification-ci.yml` |
| secret-scanning-ci.yml | `05-quality/security/secret-scanning-ci.yml` |
| security-hardening-ci.yml | `05-quality/security/security-hardening-ci.yml` |
| security-signature-rocksdb-iteration-ci.yml | `05-quality/security/security-signature-rocksdb-iteration-ci.yml` |

### build (3)

| Workflow | Path |
|----------|------|
| build-reproducibility-ci.yml | `05-quality/build/build-reproducibility-ci.yml` |
| cross-module-performance-regression-ci.yml | `05-quality/build/cross-module-performance-regression-ci.yml` |
| error-handling-audit.yml | `05-quality/build/error-handling-audit.yml` |

### validation (6)

| Workflow | Path |
|----------|------|
| documentation-validation.yml | `05-quality/validation/documentation-validation.yml` |
| research-validation.yml | `05-quality/validation/research-validation.yml` |
| validate-ai-guardrails.yml | `05-quality/validation/validate-ai-guardrails.yml` |
| validate-config-mapping.yml | `05-quality/validation/validate-config-mapping.yml` |
| validate-grafana-dashboards.yml | `05-quality/validation/validate-grafana-dashboards.yml` |
| validate-roadmap.yml | `05-quality/validation/validate-roadmap.yml` |

---

## 06-infrastructure (19 workflows)

### gpu (4)

| Workflow | Path |
|----------|------|
| gpu-ci.yml | `06-infrastructure/gpu/gpu-ci.yml` |
| gpu-memory-oversubscription-ci.yml | `06-infrastructure/gpu/gpu-memory-oversubscription-ci.yml` |
| opencl-erasure-coder-parity-ci.yml | `06-infrastructure/gpu/opencl-erasure-coder-parity-ci.yml` |
| vulkan-compute-shader-pipeline-ci.yml | `06-infrastructure/gpu/vulkan-compute-shader-pipeline-ci.yml` |

### distributed (6)

| Workflow | Path |
|----------|------|
| cross-shard-coordinator-id-compensation-rpc-ci.yml | `06-infrastructure/distributed/cross-shard-coordinator-id-compensation-rpc-ci.yml` |
| distributed-cache-integration-ci.yml | `06-infrastructure/distributed/distributed-cache-integration-ci.yml` |
| distributed-cluster-updates-ci.yml | `06-infrastructure/distributed/distributed-cluster-updates-ci.yml` |
| httpserver-shardingmanager-ci.yml | `06-infrastructure/distributed/httpserver-shardingmanager-ci.yml` |
| load-balancing-raft-coordination-ci.yml | `06-infrastructure/distributed/load-balancing-raft-coordination-ci.yml` |
| shard-rpc-integration-ci.yml | `06-infrastructure/distributed/shard-rpc-integration-ci.yml` |

### observability (4)

| Workflow | Path |
|----------|------|
| metrics-collector-shared-mutex-ci.yml | `06-infrastructure/observability/metrics-collector-shared-mutex-ci.yml` |
| request-tracing-correlation-ids-ci.yml | `06-infrastructure/observability/request-tracing-correlation-ids-ci.yml` |
| root-cause-analyzer-ci.yml | `06-infrastructure/observability/root-cause-analyzer-ci.yml` |
| statistics-collector-ci.yml | `06-infrastructure/observability/statistics-collector-ci.yml` |

### networking (5)

| Workflow | Path |
|----------|------|
| api-gateway-enhancements-ci.yml | `06-infrastructure/networking/api-gateway-enhancements-ci.yml` |
| bandwidth-management-qos-ci.yml | `06-infrastructure/networking/bandwidth-management-qos-ci.yml` |
| udp-server-ci.yml | `06-infrastructure/networking/udp-server-ci.yml` |
| wire-protocol-optimizations-ci.yml | `06-infrastructure/networking/wire-protocol-optimizations-ci.yml` |
| wire-protocol-v2-ci.yml | `06-infrastructure/networking/wire-protocol-v2-ci.yml` |

---

## 07-data-pipelines (9 workflows)

| Workflow | Path |
|----------|------|
| cdc-websocket-streaming-ci.yml | `07-data-pipelines/cdc-websocket-streaming-ci.yml` |
| consumer-group-semantics-ci.yml | `07-data-pipelines/consumer-group-semantics-ci.yml` |
| content-embedding-pipeline-ci.yml | `07-data-pipelines/content-embedding-pipeline-ci.yml` |
| importer-tests.yml | `07-data-pipelines/importer-tests.yml` |
| ingestion-tests.yml | `07-data-pipelines/ingestion-tests.yml` |
| kafka-consumer-source-connector-ci.yml | `07-data-pipelines/kafka-consumer-source-connector-ci.yml` |
| kafka-importer-ci.yml | `07-data-pipelines/kafka-importer-ci.yml` |
| s3-compatible-object-storage-connector-ci.yml | `07-data-pipelines/s3-compatible-object-storage-connector-ci.yml` |
| wal-archival-object-storage-ci.yml | `07-data-pipelines/wal-archival-object-storage-ci.yml` |

---

## 08-maintenance (12 workflows)

| Workflow | Path |
|----------|------|
| acceleration-roadmap-audit.yml | `08-maintenance/acceleration-roadmap-audit.yml` |
| add-doc-metadata.yml | `08-maintenance/add-doc-metadata.yml` |
| auto-label.yml | `08-maintenance/auto-label.yml` |
| classify-bridge-ci.yml | `08-maintenance/classify-bridge-ci.yml` |
| code-maturity-analysis.yml | `08-maintenance/code-maturity-analysis.yml` |
| data-augmentation-tests.yml | `08-maintenance/data-augmentation-tests.yml` |
| github_workflows_label-governance.yml | `08-maintenance/github_workflows_label-governance.yml` |
| module-docs-sync.yml | `08-maintenance/module-docs-sync.yml` |
| sdk-generation.yml | `08-maintenance/sdk-generation.yml` |
| sync-milestones.yml | `08-maintenance/sync-milestones.yml` |
| sync-milestones-ci.yml | `08-maintenance/sync-milestones-ci.yml` |
| sync-roadmap-issues.yml | `08-maintenance/sync-roadmap-issues.yml` |

---

## 09-pr-gates (4 workflows)

| Workflow | Path |
|----------|------|
| pr-path-gate-enterprise.yml | `09-pr-gates/pr-path-gate-enterprise.yml` |
| pr-path-gate-hyperscaler.yml | `09-pr-gates/pr-path-gate-hyperscaler.yml` |
| pr-path-gate-main.yml | `09-pr-gates/pr-path-gate-main.yml` |
| pr-quick-checks.yml | `09-pr-gates/pr-quick-checks.yml` |

---

## docs (2 workflows)

| Workflow | Path |
|----------|------|
| docs-pipeline.yml | `docs/docs-pipeline.yml` |
| primary-docs-index.yml | `docs/primary-docs-index.yml` |

---

## Cross-Workflow Dependencies

| Caller | Callee |
|--------|--------|
| All `-ci.yml` workflows (107 total) | `01-core/ci-scope-classifier.yml` |
| `03-editions/edition-community-ci.yml` | `03-editions/edition-build-ci.yml` |
| `03-editions/edition-enterprise-ci.yml` | `03-editions/edition-build-ci.yml` |
| `03-editions/edition-hyperscaler-ci.yml` | `03-editions/edition-build-ci.yml` |
| `03-editions/edition-military-ci.yml` | `03-editions/edition-build-ci.yml` |
| `03-editions/edition-minimal-ci.yml` | `03-editions/edition-build-ci.yml` |
