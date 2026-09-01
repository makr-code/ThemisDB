> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

<!-- Status: current | generated: 2026-03-12 | source: all src/*/FUTURE_ENHANCEMENTS.md -->
<!-- Use this file as the master backlog for GitHub Issue creation. -->
<!-- Each item row maps directly to one GitHub Issue. -->

# ThemisDB — Consolidated Source Roadmap

> **Purpose:** This document aggregates every open TODO, stub-replacement, and planned feature
> from all 50 module `FUTURE_ENHANCEMENTS.md` files.
> It is sorted by **Priority → Target Version → Module** and is the canonical input for
> creating GitHub Issues with full implementation context.

## Module Status Snapshot

For the per-module current state, use [`MODULE_INDEX.md`](MODULE_INDEX.md) as the canonical overview.

| Status | Module groups | Notes |
|---|---|---|
| Production-ready / mostly closed | `server`, `storage`, `network`, `auth`, `security`, `cache`, `analytics`, `failover`, `maintenance`, `updates`, `process`, `execution` | These modules are largely in the documentation/readiness phase rather than core implementation phase. |
| Active hardening | `themis`, `transaction`, `query`, `index`, `sharding`, `replication`, `graph`, `cdc`, `llm`, `rag`, `gpu`, `acceleration`, `geo`, `voice`, `access_model`, `ethics_ai` | These modules still carry real code or evidence gaps and remain the main source of roadmap work. |
| Planned / externalization | `chimera`, `user_storage`, plugin externalization tracks | Planning exists, but the implementation boundary is still being finalized. |

---

## Table of Contents

1. [How to Use for GitHub Issue Creation](#how-to-use-for-github-issue-creation)
2. [Status & Priority Legend](#status--priority-legend)
3. [Statistics](#statistics)
4. [🔴 Critical Priority](#-critical-priority)
5. [🟠 High Priority — Immediate (≤ v1.4.0)](#-high-priority--immediate--v140)
6. [🟠 High Priority — Near-term (v1.5.0 – v1.8.0)](#-high-priority--near-term-v150--v180)
7. [🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0)](#-medium-priority--near-term-v150--v180)
8. [🟡 Medium Priority — Mid-term (v1.9.0 – v2.0.0)](#-medium-priority--mid-term-v190--v200)
9. [🟢 Low Priority — Future (v1.9.0+)](#-low-priority--future-v190)
10. [Cross-Cutting Concerns](#cross-cutting-concerns)
11. [Milestone Summary](#milestone-summary)
12. [Suggested GitHub Label Taxonomy](#suggested-github-label-taxonomy)

---

## How to Use for GitHub Issue Creation

Each row in the tables below maps to **one GitHub Issue**. Columns:

| Column | GitHub Issue Field |
|--------|--------------------|
| `#` | Issue number suggestion (sequential within this file) |
| `Module` | Label `module:<name>` |
| `Title` | Issue title |
| `Target` | Milestone |
| `Labels` | Comma-separated label suggestions |
| `Detail` | Link to the full implementation description |

**Recommended issue body template:**
```
## Summary
<Title from this table>

## Module
`src/<module>/`

## Priority / Target Version
<Priority> · <Target Version>

## Detailed Implementation Description
See: <Detail link>

## Acceptance Criteria
- [ ] (copy `- [ ]` items from the linked FUTURE_ENHANCEMENTS.md section)

## Labels
<Labels>
```

---

## Status & Priority Legend

| Symbol | Meaning |
|--------|---------|
| 🔴 | **Critical** — Security vulnerability or data-loss risk; block GA |
| 🟠 | **High** — Required for production readiness; block next minor release |
| 🟡 | **Medium** — Significant improvement; plan within 2 minor releases |
| 🟢 | **Low** — Enhancement or cleanup; schedule opportunistically |
| `[ ]` | Open |
| `[x]` | Done |
| `[~]` | In Progress |

**Time Horizons:**

| Target Range | Horizon | Calendar Estimate |
|---|---|---|
| ≤ v1.4.0 | Immediate | Q2 2026 |
| v1.5.0 – v1.6.0 | Short-term | Q3 2026 |
| v1.7.0 – v1.8.0 | Near-term | Q4 2026 – Q1 2027 |
| v1.9.0 – v2.0.0 | Mid-term | Q2–Q3 2027 |
| v2.1.0+ | Long-term | 2027+ |

---

## Statistics

| Priority | Count |
|----------|-------|
| 🔴 Critical | 4 |
| 🟠 High | 89 |
| 🟡 Medium | 84 |
| 🟢 Low | 38 |
| **Total** | **215** |

| Target Version | Open Items |
|---|---|
| ≤ v1.2.0 | 34 |
| v1.3.0 – v1.6.0 | 63 |
| v1.7.0 – v1.8.0 | 86 |
| v1.9.0 – v2.0.0 | 21 |
| v2.1.0+ | 11 |

---

## 🔴 Critical Priority

> Block release. Must be fixed before any production deployment.

| # | Module | Title | Target | Labels | Issue | Detail |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | auth | Thread-Safety: Add `std::mutex` to `JWTValidator` JWKS Cache | v1.1.0 | `security`, `thread-safety`, `module:auth`, `correctness` | #3825 | [→ Detail](auth/FUTURE_ENHANCEMENTS.md#1-thread-safety-add-mutex-to-jwtvalidator-jwks-cache) |
| 2 | auth | LDAP DN and Filter Injection Prevention | v1.1.0 | `security`, `injection`, `module:auth` | #3826 | [→ Detail](auth/FUTURE_ENHANCEMENTS.md#3-ldap-dn-and-filter-injection-prevention) |
| 3 | themis | Modular Build System | v1.7.0 | `build`, `module:themis`, `infrastructure` | #3827 | [→ Detail](themis/FUTURE_ENHANCEMENTS.md#modular-build-system) |
| 4 | themis | Module Loader Implementation | v1.7.0 | `build`, `module:themis`, `infrastructure` | #3828 | [→ Detail](themis/FUTURE_ENHANCEMENTS.md#module-loader-implementation) |

---

## 🟠 High Priority — Immediate (≤ v1.4.0)

> Calendar: Q2 2026. Required for the next minor release.

| # | Module | Title | Target | Labels | Issue | Detail |
| --- | --- | --- | --- | --- | --- | --- |
| 5 | auth | Constant-Time Comparison for Recovery Codes and Session IDs | v1.1.0 | `security`, `timing-attack`, `module:auth` | #3833 | [→ Detail](auth/FUTURE_ENHANCEMENTS.md#4-constant-time-comparison-for-recovery-codes-and-session-ids) |
| 6 | auth | Mandatory JWT Issuer and Audience Validation | v1.1.0 | `security`, `jwt`, `module:auth` | #3834 | [→ Detail](auth/FUTURE_ENHANCEMENTS.md#5-mandatory-jwt-issuer-and-audience-validation) |
| 7 | auth | Secure Memory for Key Material in `jwks_security.cpp` | v1.2.0 | `security`, `memory`, `module:auth` | #3835 | [→ Detail](auth/FUTURE_ENHANCEMENTS.md#10-secure-memory-for-key-material-in-jwks_securitycpp) |
| 8 | auth | Async / Non-Blocking LDAP and HTTP Authentication Calls | v1.2.0 | `performance`, `async`, `module:auth` | #3836 | [→ Detail](auth/FUTURE_ENHANCEMENTS.md#2-async--non-blocking-ldap-and-http-authentication-calls) |
| 9 | auth | Token Blacklist Persistence and Distributed Support | v1.3.0 | `distributed`, `auth`, `module:auth` | #3837 | [→ Detail](auth/FUTURE_ENHANCEMENTS.md#7-token-blacklist-persistence-and-distributed-support) |
| 10 | auth | LDAP Connection Pooling | v1.2.0 | `performance`, `module:auth` | #3838 | [→ Detail](auth/FUTURE_ENHANCEMENTS.md#8-ldap-connection-pooling) |
| 11 | base | O(1) Module Lookup — Replace `loadedModules_` Vector with Unordered Map | v1.2.0 | `performance`, `module:base` | #3839 | [→ Detail](base/FUTURE_ENHANCEMENTS.md#o1-module-lookup--replace-loadedmodules_-vector-with-unordered-map) |
| 12 | base | cgroup v2 Resource Enforcement for Module Sandbox | v1.2.0 | `security`, `sandbox`, `module:base` | #3840 | [→ Detail](base/FUTURE_ENHANCEMENTS.md#cgroup-v2-resource-enforcement-for-module-sandbox) |
| 13 | base | WASM Instruction Fuel Metering | v1.2.0 | `security`, `wasm`, `module:base` | #3841 | [→ Detail](base/FUTURE_ENHANCEMENTS.md#wasm-instruction-fuel-metering) |
| 14 | chimera | Production ThemisDB Adapter Integration | v1.1.0 | `stub-replacement`, `module:chimera` | #3842 | [→ Detail](chimera/FUTURE_ENHANCEMENTS.md#production-themisdb-adapter-integration) |
| 15 | chimera | MongoDB / Qdrant / Neo4j: Replace In-Process Simulation with Real Drivers | v1.2.0 | `stub-replacement`, `module:chimera` | #3843 | [→ Detail](chimera/FUTURE_ENHANCEMENTS.md#mongodb--qdrant--neo4j-replace-in-process-simulation-with-real-drivers) |
| 16 | chimera | Transaction Management Enhancements | v1.1.0 | `correctness`, `module:chimera` | #3844 | [→ Detail](chimera/FUTURE_ENHANCEMENTS.md#transaction-management-enhancements) |
| 17 | chimera | Error Recovery and Retry Logic | v1.1.0 | `reliability`, `module:chimera` | #3845 | [→ Detail](chimera/FUTURE_ENHANCEMENTS.md#error-recovery-and-retry-logic) |
| 18 | chimera | Batch Operation Enhancements | v1.1.0 | `performance`, `module:chimera` | #3846 | [→ Detail](chimera/FUTURE_ENHANCEMENTS.md#batch-operation-enhancements) |
| 19 | maintenance | Schedule Persistence (RocksDB) | v1.1.0 | `persistence`, `module:maintenance` | #3847 | [→ Detail](maintenance/FUTURE_ENHANCEMENTS.md#schedule-persistence-rocksdb) |
| 20 | maintenance | Force-Run Endpoint: Window Override | v1.1.0 | `api`, `module:maintenance` | #3848 | [→ Detail](maintenance/FUTURE_ENHANCEMENTS.md#force-run-endpoint-window-override) |
| 21 | temporal | Full System-Versioned Table Support | v1.1.0 | `feature`, `module:temporal` | #3849 | [→ Detail](temporal/FUTURE_ENHANCEMENTS.md#full-system-versioned-table-support) |
| 22 | temporal | Temporal Conflict Detection and Resolution | v1.1.0 | `correctness`, `module:temporal` | #3850 | [→ Detail](temporal/FUTURE_ENHANCEMENTS.md#temporal-conflict-detection-and-resolution) |
| 23 | temporal | Snapshot Isolation | v1.1.0 | `correctness`, `module:temporal` | #3851 | [→ Detail](temporal/FUTURE_ENHANCEMENTS.md#snapshot-isolation) |
| 24 | temporal | Application-Versioned Tables (Bi-Temporal) | v1.2.0 | `feature`, `module:temporal` | #3852 | [→ Detail](temporal/FUTURE_ENHANCEMENTS.md#application-versioned-tables-bi-temporal) |
| 25 | temporal | Time-Travel Query Engine | v1.2.0 | `feature`, `module:temporal` | #3853 | [→ Detail](temporal/FUTURE_ENHANCEMENTS.md#time-travel-query-engine) |
| 26 | temporal | Temporal Indexes | v1.2.0 | `performance`, `module:temporal` | #3854 | [→ Detail](temporal/FUTURE_ENHANCEMENTS.md#temporal-indexes) |
| 27 | security | `AQLInjectionDetector`: AST-Level Validation | v1.4.0 | `security`, `injection`, `module:security` | #3855 | [→ Detail](security/FUTURE_ENHANCEMENTS.md#aqlinjectdetector-ast-level-validation) |
| 28 | gpu | `query_accelerator.cpp`: Replace CPU Fallback Stubs with Real CUDA/HIP | v1.4.0 | `gpu`, `stub-replacement`, `performance`, `module:gpu` | #3856 | [→ Detail](gpu/FUTURE_ENHANCEMENTS.md#query_acceleratorcpp-replace-cpu-fallback-stubs-with-real-cudahip-dispatch) |
| 29 | index | GPU Vector Index: CUDA and HIP Backend Implementation | v1.4.0 | `gpu`, `stub-replacement`, `module:index` | #3857 | [→ Detail](index/FUTURE_ENHANCEMENTS.md#gpu-vector-index-cuda-and-hip-backend-implementation) |
| 30 | geo | CUDA and OpenCL Implementation in `gpu_backend_production.cpp` | v1.4.0 | `gpu`, `stub-replacement`, `module:geo` | #3858 | [→ Detail](geo/FUTURE_ENHANCEMENTS.md#cuda-and-opencl-implementation-in-gpu_backend_productioncpp) |
| 31 | aql | Post-Generation AQL Validation in `translateNLToAQL()` | v1.6.0 | `correctness`, `module:aql` | #3859 | [→ Detail](aql/FUTURE_ENHANCEMENTS.md#1--post-generation-aql-validation-in-translatenlttoaql) |
| 32 | aql | Eliminate Thread Leak in `LLMTimeoutManager::executeWithTimeout()` | v1.6.0 | `thread-safety`, `correctness`, `module:aql` | #3860 | [→ Detail](aql/FUTURE_ENHANCEMENTS.md#2--eliminate-thread-leak-in-llmtimeoutmanagerexecutewithtimeout) |
| 33 | aql | Per-Operation-Type Circuit Breakers | v1.6.0 | `reliability`, `module:aql` | #3861 | [→ Detail](aql/FUTURE_ENHANCEMENTS.md#3--per-operation-type-circuit-breakers) |
| 34 | aql | Bounded Conversation History with Context-Window Budget | v1.6.0 | `correctness`, `module:aql` | #3862 | [→ Detail](aql/FUTURE_ENHANCEMENTS.md#9--bounded-conversation-history-with-context-window-budget) |

---

## 🟠 High Priority — Near-term (v1.5.0 – v1.8.0)

> Calendar: Q3 2026 – Q1 2027.

| # | Module | Title | Target | Labels | Issue | Detail |
| --- | --- | --- | --- | --- | --- | --- |
| 35 | acceleration | CUDA Kernel Completion for Vector Similarity Search | v1.7.0 | `gpu`, `stub-replacement`, `module:acceleration` | #3863 | [→ Detail](acceleration/FUTURE_ENHANCEMENTS.md#cuda-kernel-completion-for-vector-similarity-search) |
| 36 | acceleration | Vulkan Compute Shader Pipeline for Cross-Platform GPU | v1.7.0 | `gpu`, `module:acceleration` | #3864 | [→ Detail](acceleration/FUTURE_ENHANCEMENTS.md#vulkan-compute-shader-pipeline-for-cross-platform-gpu) |
| 37 | acceleration | Runtime Device Capability Negotiation | v1.7.0 | `gpu`, `module:acceleration` | #3865 | [→ Detail](acceleration/FUTURE_ENHANCEMENTS.md#runtime-device-capability-negotiation) |
| 38 | acceleration | Plugin Security: CRL and OCSP Certificate Revocation Checking | v1.8.0 | `security`, `module:acceleration` | #3866 | [→ Detail](acceleration/FUTURE_ENHANCEMENTS.md#plugin-security-crl-and-ocsp-certificate-revocation-checking) |
| 39 | acceleration | NCCL/RCCL Distributed `mergeTopK` Implementation | v1.9.0 | `gpu`, `distributed`, `module:acceleration` | #3867 | [→ Detail](acceleration/FUTURE_ENHANCEMENTS.md#ncclrccl-distributed-mergetopk-implementation) |
| 40 | analytics | ExporterFactory Stub Replacement | v1.8.0 | `stub-replacement`, `module:analytics` | #3868 | [→ Detail](analytics/FUTURE_ENHANCEMENTS.md#1--exporterfactory-stub-replacement) |
| 41 | analytics | Lock Held Across User Callbacks in `CEPEngine::timerLoop()` | v1.8.0 | `thread-safety`, `module:analytics` | #3869 | [→ Detail](analytics/FUTURE_ENHANCEMENTS.md#2--lock-held-across-user-callbacks-in-cepenginetimerloop) |
| 42 | analytics | `StreamingAnomalyDetector::process()` — Training Under Lock | v1.8.0 | `thread-safety`, `performance`, `module:analytics` | #3870 | [→ Detail](analytics/FUTURE_ENHANCEMENTS.md#3--streaminganomalydetectorprocess--training-under-lock) |
| 43 | analytics | `ModelServingEngine::predict()` — Inference Under Registry Lock | v1.8.0 | `thread-safety`, `performance`, `module:analytics` | #3871 | [→ Detail](analytics/FUTURE_ENHANCEMENTS.md#4--modelservingenginepredict--inference-under-registry-lock) |
| 44 | analytics | `MLServingEngine::infer()` — TOCTOU Session Load + Full-Inference Lock | v1.8.0 | `thread-safety`, `correctness`, `module:analytics` | #3872 | [→ Detail](analytics/FUTURE_ENHANCEMENTS.md#5--mlservingengineinfer--toctou-session-load--full-inference-lock) |
| 45 | analytics | `IncrementalView::applyChanges()` — Exclusive Lock for Entire Batch | v1.8.0 | `thread-safety`, `performance`, `module:analytics` | #3873 | [→ Detail](analytics/FUTURE_ENHANCEMENTS.md#6--incrementalviewapplychanges--exclusive-lock-for-entire-batch) |
| 46 | analytics | SIMD Vectorization — AVX-512 and ARM NEON | v1.8.0 | `performance`, `simd`, `module:analytics` | #3874 | [→ Detail](analytics/FUTURE_ENHANCEMENTS.md#14--simd-vectorization--avx-512-and-arm-neon) |
| 47 | analytics | Memory Pool Allocator for Hot Analytics Paths | v1.8.0 | `performance`, `memory`, `module:analytics` | #3875 | [→ Detail](analytics/FUTURE_ENHANCEMENTS.md#15--memory-pool-allocator-for-hot-analytics-paths) |
| 48 | api | GraphQL Schema Completion and Subscription Support | v1.7.0 | `api`, `graphql`, `module:api` | #3876 | [→ Detail](api/FUTURE_ENHANCEMENTS.md#graphql-schema-completion-and-subscription-support) |
| 49 | api | WebSocket Real-Time Change Streaming Endpoint | v1.7.0 | `api`, `websocket`, `module:api` | #3877 | [→ Detail](api/FUTURE_ENHANCEMENTS.md#websocket-real-time-change-streaming-endpoint) |
| 50 | api | Versioned API Routing and `/v2/` Prefix | v1.8.0 | `api`, `module:api` | #3878 | [→ Detail](api/FUTURE_ENHANCEMENTS.md#versioned-api-routing-and-v2-prefix) |
| 51 | api | gRPC API Surface — Wire Stub Implementations | v2.0.0 | `grpc`, `stub-replacement`, `module:api` | #3879 | [→ Detail](api/FUTURE_ENHANCEMENTS.md#grpc-api-surface--wire-stub-implementations) |
| 52 | api | GraphQL WebSocket Handler — CDC Callback Lifetime Safety | v1.8.0 | `correctness`, `memory-safety`, `module:api` | #3880 | [→ Detail](api/FUTURE_ENHANCEMENTS.md#graphql-websocket-handler--cdc-callback-lifetime-safety) |
| 53 | cache | Lock-Free L1 Read Path | v1.7.0 | `performance`, `thread-safety`, `module:cache` | #3881 | [→ Detail](cache/FUTURE_ENHANCEMENTS.md#lock-free-l1-read-path) |
| 54 | cache | `RedisCacheCoordinator` Async Pub/Sub Subscription Loop | v1.7.0 | `async`, `module:cache` | #3882 | [→ Detail](cache/FUTURE_ENHANCEMENTS.md#rediscachecoordinator-async-pubsub-subscription-loop) |
| 55 | cdc | WebSocket Change Streaming Transport | v1.7.0 | `websocket`, `module:cdc` | #3883 | [→ Detail](cdc/FUTURE_ENHANCEMENTS.md#websocket-change-streaming-transport) |
| 56 | cdc | Consumer Group Semantics and Offset Tracking | v1.8.0 | `feature`, `module:cdc` | #3884 | [→ Detail](cdc/FUTURE_ENHANCEMENTS.md#consumer-group-semantics-and-offset-tracking) |
| 57 | config | Prometheus Metrics Exporter for Path Resolution | v1.7.0 | `observability`, `module:config` | #3885 | [→ Detail](config/FUTURE_ENHANCEMENTS.md#prometheus-metrics-exporter-for-path-resolution) |
| 58 | config | Deprecation Warning Aggregation Report | v1.7.0 | `dx`, `module:config` | #3886 | [→ Detail](config/FUTURE_ENHANCEMENTS.md#deprecation-warning-aggregation-report) |
| 59 | config | Config Audit Trail | v1.8.0 | `security`, `audit`, `module:config` | #3887 | [→ Detail](config/FUTURE_ENHANCEMENTS.md#config-audit-trail) |
| 60 | config | CLI Migration Scanner | v1.8.0 | `dx`, `module:config` | #3888 | [→ Detail](config/FUTURE_ENHANCEMENTS.md#cli-migration-scanner) |
| 61 | content | Abuse Detection Stub Replacement | v1.8.0 | `security`, `stub-replacement`, `module:content` | #3889 | [→ Detail](content/FUTURE_ENHANCEMENTS.md#abuse-detection-stub-replacement) |
| 62 | content | Embedding Generation Pipeline (Text → Vector) | v1.8.0 | `feature`, `ml`, `module:content` | #3890 | [→ Detail](content/FUTURE_ENHANCEMENTS.md#embedding-generation-pipeline-text--vector) |
| 63 | core | Dynamic Adapter Reconfiguration | v1.6.0 | `feature`, `module:core` | #3891 | [→ Detail](core/FUTURE_ENHANCEMENTS.md#dynamic-adapter-reconfiguration) |
| 64 | core | Distributed Cache Integration | v1.6.0 | `distributed`, `module:core` | #3892 | [→ Detail](core/FUTURE_ENHANCEMENTS.md#distributed-cache-integration) |
| 65 | core | Zero-Copy Logging | v1.6.0 | `performance`, `module:core` | #3893 | [→ Detail](core/FUTURE_ENHANCEMENTS.md#zero-copy-logging) |
| 66 | core | Lock-Free Metrics | v1.6.0 | `performance`, `thread-safety`, `module:core` | #3894 | [→ Detail](core/FUTURE_ENHANCEMENTS.md#lock-free-metrics) |
| 67 | geo | R-tree Spatial Index for CPU Backend | v1.5.0 | `performance`, `module:geo` | #3895 | [→ Detail](geo/FUTURE_ENHANCEMENTS.md#r-tree-spatial-index-for-cpu-backend) |
| 68 | importers | MySQL / MariaDB Importer (wire & verify) | v1.8.0 | `feature`, `module:importers` | #3896 | [→ Detail](importers/FUTURE_ENHANCEMENTS.md#mysql--mariadb-importer) |
| 69 | importers | MongoDB Document Importer (BSON types) | v1.8.0 | `feature`, `module:importers` | #3897 | [→ Detail](importers/FUTURE_ENHANCEMENTS.md#mongodb-document-importer) |
| 70 | importers | Apache Kafka Consumer Importer | v1.7.0 | `feature`, `streaming`, `module:importers` | #3898 | [→ Detail](importers/FUTURE_ENHANCEMENTS.md#apache-kafka-consumer-importer) |
| 71 | index | Distributed Index Partitioning | v1.6.0 | `distributed`, `module:index` | #3902 | [→ Detail](index/FUTURE_ENHANCEMENTS.md#distributed-index-partitioning) |
| 72 | index | GPU Memory Oversubscription | v1.7.0 | `gpu`, `memory`, `module:index` | #3903 | [→ Detail](index/FUTURE_ENHANCEMENTS.md#gpu-memory-oversubscription) |
| 73 | ingestion | `LLMIngestionAdapter` Phase 2: Wire llama.cpp | v1.8.0 | `stub-replacement`, `llm`, `module:ingestion` | #3904 | [→ Detail](ingestion/FUTURE_ENHANCEMENTS.md#llmingestionadapter-phase-2-wire-llamacpp) |
| 74 | ingestion | Kafka Consumer Source Connector | v1.7.0 | `streaming`, `module:ingestion` | #3905 | [→ Detail](ingestion/FUTURE_ENHANCEMENTS.md#kafka-consumer-source-connector) |
| 75 | llm | `LoraSecurityValidator`: Certificate Store Integration | v1.8.0 | `security`, `stub-replacement`, `module:llm` | #3906 | [→ Detail](llm/FUTURE_ENHANCEMENTS.md#lorasecurityvalidator-certificate-store-integration) |
| 76 | llm | Streaming Token Output (SSE / Chunked Response) | v1.7.0 | `api`, `performance`, `module:llm` | #3907 | [→ Detail](llm/FUTURE_ENHANCEMENTS.md#streaming-token-output-sse--chunked-response) |
| 77 | llm | OpenAI-Compatible `/v1/chat/completions` Adapter | v1.7.0 | `api`, `module:llm` | #3908 | [→ Detail](llm/FUTURE_ENHANCEMENTS.md#openai-compatible-v1chatcompletions-adapter) |
| 78 | network | Load Balancing with Raft Coordination | v1.8.0 | `distributed`, `module:network` | #3909 | [→ Detail](network/FUTURE_ENHANCEMENTS.md#load-balancing-with-raft-coordination) |
| 79 | observability | OpenTelemetry Full Integration | v1.6.0 | `observability`, `module:observability` | #3910 | [→ Detail](observability/FUTURE_ENHANCEMENTS.md#opentelemetry-full-integration) |
| 80 | observability | Custom Metric Types | v1.6.0 | `observability`, `module:observability` | #3911 | [→ Detail](observability/FUTURE_ENHANCEMENTS.md#custom-metric-types) |
| 81 | observability | Metric Aggregation Pipeline | v1.6.0 | `observability`, `module:observability` | #3912 | [→ Detail](observability/FUTURE_ENHANCEMENTS.md#metric-aggregation-pipeline) |
| 82 | observability | Streaming Metrics | v1.6.0 | `observability`, `streaming`, `module:observability` | #3913 | [→ Detail](observability/FUTURE_ENHANCEMENTS.md#streaming-metrics) |
| 83 | observability | ML-Based Anomaly Detection | v1.7.0 | `ml`, `observability`, `module:observability` | #3914 | [→ Detail](observability/FUTURE_ENHANCEMENTS.md#machine-learning-based-anomaly-detection) |
| 84 | observability | Root Cause Analysis | v1.7.0 | `observability`, `module:observability` | #3915 | [→ Detail](observability/FUTURE_ENHANCEMENTS.md#root-cause-analysis) |
| 85 | performance | Hardware-Accelerated Query Execution | v1.8.0 | `performance`, `gpu`, `module:performance` | #3916 | [→ Detail](performance/FUTURE_ENHANCEMENTS.md#hardware-accelerated-query-execution) |
| 86 | performance | Adaptive Query Compilation | v1.8.0 | `performance`, `jit`, `module:performance` | #3917 | [→ Detail](performance/FUTURE_ENHANCEMENTS.md#adaptive-query-compilation) |
| 87 | query | `QueryOptimizer`: Wire Real MetadataShard, Prometheus, and Statistics | v1.6.0 | `correctness`, `stub-replacement`, `module:query` | #3918 | [→ Detail](query/FUTURE_ENHANCEMENTS.md#queryoptimizer-wire-real-metadatashard-prometheus-and-statistics) |
| ~~88~~ | query | ~~`QueryFederation`: Real Shard Determination Logic~~ | v1.6.0 | `correctness`, `stub-replacement`, `performance`, `module:query` | #3919 | [→ Detail](query/FUTURE_ENHANCEMENTS.md#queryfederation-real-shard-determination-logic) |
| 89 | query | Query Compilation & JIT | v1.8.0 | `performance`, `jit`, `module:query` | #3920 | [→ Detail](query/FUTURE_ENHANCEMENTS.md#query-compilation--jit) |
| 90 | query | Columnar Execution Engine | v1.7.0 | `performance`, `module:query` | #3921 | [→ Detail](query/FUTURE_ENHANCEMENTS.md#columnar-execution-engine) |
| 91 | query | Adaptive Join Strategies | v1.7.0 | `performance`, `module:query` | #3922 | [→ Detail](query/FUTURE_ENHANCEMENTS.md#adaptive-join-strategies) |
| 92 | query | Parallel Query Execution (Intra-Query) | v1.7.0 | `performance`, `module:query` | #3923 | [→ Detail](query/FUTURE_ENHANCEMENTS.md#parallel-query-execution-intra-query) |
| 93 | query | Predicate Pushdown to Storage Layer | v1.7.0 | `performance`, `module:query` | #3924 | [→ Detail](query/FUTURE_ENHANCEMENTS.md#predicate-pushdown-to-storage-layer) |
| 94 | rag | `LLMIntegration` / `LLMJudgeIntegration`: Replace Stub/Mock Mode | v1.8.0 | `stub-replacement`, `correctness`, `module:rag` | #3925 | [→ Detail](rag/FUTURE_ENHANCEMENTS.md#llmintegration-and-llmjudgeintegration-replace-stubmock-mode-with-real-engine) |
| 95 | replication | Logical Replication | v1.7.0 | `feature`, `module:replication` | #3926 | [→ Detail](replication/FUTURE_ENHANCEMENTS.md#logical-replication) |
| 96 | replication | Bidirectional Replication | v1.7.0 | `feature`, `module:replication` | #3927 | [→ Detail](replication/FUTURE_ENHANCEMENTS.md#bidirectional-replication) |
| 97 | replication | Parallel Replication | v1.6.0 | `performance`, `module:replication` | #3928 | [→ Detail](replication/FUTURE_ENHANCEMENTS.md#parallel-replication) |
| 98 | scheduler | `TaskScheduler`: Propagate Authenticated User Context to Audit Events | v1.8.0 | `security`, `audit`, `module:scheduler` | #3929 | [→ Detail](scheduler/FUTURE_ENHANCEMENTS.md#taskscheduler-propagate-authenticated-user-context-to-audit-events) |
| 99 | security | `ArrowUserRegistrationPlugin`: Implement Apache Arrow Integration | v1.8.0 | `stub-replacement`, `module:security` | #3930 | [→ Detail](security/FUTURE_ENHANCEMENTS.md#arrowuserregistrationplugin-implement-apache-arrow-integration) |
| 100 | server | `AuthMiddleware`: JWT Scope Extraction and Role-to-Scope Mapping | v1.8.0 | `security`, `auth`, `module:server` | #3931 | [→ Detail](server/FUTURE_ENHANCEMENTS.md#authmiddleware-jwt-scope-extraction-and-role-to-scope-mapping) |
| 101 | server | HTTP/3 Production Readiness | v1.6.0 | `network`, `module:server` | #3932 | [→ Detail](server/FUTURE_ENHANCEMENTS.md#http3-production-readiness) |
| 102 | server | API Gateway Enhancements | v1.7.0 | `api`, `module:server` | #3933 | [→ Detail](server/FUTURE_ENHANCEMENTS.md#api-gateway-enhancements) |
| 103 | server | Rate Limiting Improvements | v1.6.0 | `performance`, `module:server` | #3934 | [→ Detail](server/FUTURE_ENHANCEMENTS.md#rate-limiting-improvements) |
| 104 | server | API Versioning & Evolution | v1.6.0 | `api`, `module:server` | #3935 | [→ Detail](server/FUTURE_ENHANCEMENTS.md#api-versioning--evolution) |
| 105 | sharding | `GpuErasureCoderOpenCL`: Implement OpenCL Encode/Decode | v1.8.0 | `gpu`, `stub-replacement`, `module:sharding` | #3936 | [→ Detail](sharding/FUTURE_ENHANCEMENTS.md#gpuerasurecoderopencl-implement-opencl-encodedecode) |
| 106 | sharding | `CrossShardTransaction`: Fix Coordinator ID + Compensation RPC | v1.8.0 | `correctness`, `distributed`, `module:sharding` | #3937 | [→ Detail](sharding/FUTURE_ENHANCEMENTS.md#crossshardtransaction-hardcode-coordinator-id-and-missing-compensation-rpc) |
| 107 | sharding | Percolator-Style Distributed Transaction Coordinator | v1.7.0 | `distributed`, `transactions`, `module:sharding` | #3938 | [→ Detail](sharding/FUTURE_ENHANCEMENTS.md#-percolator-style-distributed-transaction-coordinator) |
| 108 | sharding | Raft Snapshot Compaction and Log Truncation | v1.6.0 | `distributed`, `storage`, `module:sharding` | #3939 | [→ Detail](sharding/FUTURE_ENHANCEMENTS.md#-raft-snapshot-compaction-and-log-truncation) |
| 109 | storage | Distributed Transactions | v1.7.0 | `distributed`, `transactions`, `module:storage` | #3940 | [→ Detail](storage/FUTURE_ENHANCEMENTS.md#distributed-transactions) |
| 110 | storage | Tiered Storage (Hot/Warm/Cold) | v1.6.0 | `feature`, `module:storage` | #3941 | [→ Detail](storage/FUTURE_ENHANCEMENTS.md#tiered-storage-hotwarmcold) |
| 111 | storage | GPU-Accelerated Compression | v1.6.0 | `gpu`, `performance`, `module:storage` | #3942 | [→ Detail](storage/FUTURE_ENHANCEMENTS.md#gpu-accelerated-compression) |
| 112 | storage | NVMe Optimizations | v1.6.0 | `performance`, `io`, `module:storage` | #3943 | [→ Detail](storage/FUTURE_ENHANCEMENTS.md#nvme-optimizations) |
| 113 | themis | Wire Protocol Performance Optimizations | v1.7.0 | `performance`, `module:themis` | #3944 | [→ Detail](themis/FUTURE_ENHANCEMENTS.md#wire-protocol-performance-optimizations) |
| 114 | themis | Build Reproducibility Implementation | v1.7.0 | `build`, `module:themis` | #3945 | [→ Detail](themis/FUTURE_ENHANCEMENTS.md#build-reproducibility-implementation) |
| 115 | themis | Wire Protocol V2 Implementation | v1.8.0 | `feature`, `module:themis` | #3946 | [→ Detail](themis/FUTURE_ENHANCEMENTS.md#wire-protocol-v2-implementation) |
| 116 | timeseries | `TSStore`: Single-Point Insert Buffering for Gorilla Compression | v1.8.0 | `performance`, `correctness`, `module:timeseries` | #3947 | [→ Detail](timeseries/FUTURE_ENHANCEMENTS.md#tsstore-single-point-insert-buffering-for-gorilla-compression) |
| 117 | timeseries | Vectorised Gorilla Chunk Decoder with SIMD | v1.6.0 | `performance`, `simd`, `module:timeseries` | #3948 | [→ Detail](timeseries/FUTURE_ENHANCEMENTS.md#-vectorised-gorilla-chunk-decoder-with-simd) |
| ~~118~~ | timeseries | ~~Incremental Continuous Aggregation with Watermark Pushdown~~ | v1.6.0 | `feature`, `module:timeseries` | #3949 | [→ Detail](timeseries/FUTURE_ENHANCEMENTS.md#-incremental-continuous-aggregation-with-watermark-pushdown) |
| 119 | timeseries | Chunk-Level Encryption at Rest | v1.7.0 | `security`, `module:timeseries` | #3950 | [→ Detail](timeseries/FUTURE_ENHANCEMENTS.md#-chunk-level-encryption-at-rest) |
| 120 | training | `ProvenanceTracker`: Replace AQL Template Stubs with Live Connection | v1.8.0 | `stub-replacement`, `correctness`, `module:training` | #3951 | [→ Detail](training/FUTURE_ENHANCEMENTS.md#provenancetracker-replace-aql-template-stubs-with-live-connection) |
| 121 | training | Multi-Modality Legal Document Parser | v1.6.0 | `feature`, `module:training` | #3952 | [→ Detail](training/FUTURE_ENHANCEMENTS.md#x-multi-modality-legal-document-parser) |
| 122 | transaction | Serializable Snapshot Isolation (SSI) | v1.8.0 | `correctness`, `transactions`, `module:transaction` | #3953 | [→ Detail](transaction/FUTURE_ENHANCEMENTS.md#serializable-snapshot-isolation-ssi) |
| 123 | transaction | Distributed Transaction Coordinator (2PC) | v1.9.0 | `distributed`, `transactions`, `module:transaction` | #3954 | [→ Detail](transaction/FUTURE_ENHANCEMENTS.md#distributed-transaction-coordinator-2pc) |
| 124 | transaction | Distributed SAGA Coordinator | v1.9.0 | `distributed`, `transactions`, `module:transaction` | #3955 | [→ Detail](transaction/FUTURE_ENHANCEMENTS.md#distributed-saga-coordinator) |
| 125 | updates | Distributed Cluster Updates | v1.7.0 | `feature`, `module:updates` | #3956 | [→ Detail](updates/FUTURE_ENHANCEMENTS.md#distributed-cluster-updates) |
| 126 | updates | Binary Delta Patches | v1.6.0 | `performance`, `module:updates` | #3957 | [→ Detail](updates/FUTURE_ENHANCEMENTS.md#binary-delta-patches) |
| 127 | updates | Automatic Schema Migration Framework | v1.7.0 | `feature`, `module:updates` | #3958 | [→ Detail](updates/FUTURE_ENHANCEMENTS.md#automatic-schema-migration-framework) |
| 128 | updates | Parallel File Downloads | v1.6.0 | `performance`, `module:updates` | #3959 | [→ Detail](updates/FUTURE_ENHANCEMENTS.md#parallel-file-downloads) |

---

## 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0)

> Calendar: Q3 2026 – Q1 2027.

| # | Module | Title | Target | Labels | Issue | Detail |
| --- | --- | --- | --- | --- | --- | --- |
| 129 | acceleration | Plugin Security: PE Certificate Table Extraction | v1.8.0 | `security`, `module:acceleration` | #3960 | [→ Detail](acceleration/FUTURE_ENHANCEMENTS.md#plugin-security-pe-certificate-table-extraction) |
| 130 | acceleration | VLLMResourceManager: OS-Level CPU and RAM Monitoring | v1.8.0 | `observability`, `module:acceleration` | #3961 | [→ Detail](acceleration/FUTURE_ENHANCEMENTS.md#vllmresourcemanager-os-level-cpu-and-ram-monitoring) |
| 131 | acceleration | VLLMResourceManager: Multi-GPU NVML Monitoring (Beyond GPU 0) | v1.8.0 | `observability`, `gpu`, `module:acceleration` | #3962 | [→ Detail](acceleration/FUTURE_ENHANCEMENTS.md#vllmresourcemanager-multi-gpu-nvml-monitoring-beyond-gpu-0) |
| 132 | acceleration | CUDA HNSW Kernel: Remove Silent `k > kMaxK` Clamping | v1.8.0 | `correctness`, `gpu`, `module:acceleration` | #3963 | [→ Detail](acceleration/FUTURE_ENHANCEMENTS.md#cuda-hnsw-kernel-remove-silent-k--kmaxk-clamping) |
| 133 | acceleration | BackendRegistry: Thread-Safe Read Access After Initialization | v1.8.0 | `thread-safety`, `module:acceleration` | #3964 | [→ Detail](acceleration/FUTURE_ENHANCEMENTS.md#backendregistry-thread-safe-read-access-after-initialization) |
| 134 | analytics | `LLMProcessAnalyzer` — O(N) Cache Eviction Under Lock | v1.8.0 | `performance`, `thread-safety`, `module:analytics` | #3965 | [→ Detail](analytics/FUTURE_ENHANCEMENTS.md#7--llmprocessanalyzer--on-cache-eviction-under-lock) |
| 135 | analytics | `DistributedAnalyticsSharding::getHealthyShardCount()` — Network I/O Under Lock | v1.8.0 | `thread-safety`, `module:analytics` | #3966 | [→ Detail](analytics/FUTURE_ENHANCEMENTS.md#8--distributedanalyticsshardingcountgethealthyshardcount--network-io-under-lock) |
| 136 | analytics | `DiffEngine::computeDiff()` — Cache Stampede / O(N) Changefeed Scan | v1.8.0 | `performance`, `module:analytics` | #3967 | [→ Detail](analytics/FUTURE_ENHANCEMENTS.md#9--diffenginecomputediff--cache-stampede--on-changefeed-scan) |
| 137 | analytics | `automl.cpp` — `KNNRegressorModel::predictOneReg()` Stub | v1.8.0 | `stub-replacement`, `module:analytics` | #3968 | [→ Detail](analytics/FUTURE_ENHANCEMENTS.md#10--automlcpp--knnregressormodelpredictoreg-stub) |
| 138 | analytics | `streaming_window.cpp` — 8 Open TODOs + Hard-coded Poll Intervals | v1.8.0 | `correctness`, `module:analytics` | #3969 | [→ Detail](analytics/FUTURE_ENHANCEMENTS.md#13--streaming_windowcpp--8-open-todos--hard-coded-poll-intervals) |
| 139 | analytics | Arrow Zero-Copy Integration and Result Cache with LRU Eviction | v1.8.0 | `performance`, `module:analytics` | #3970 | [→ Detail](analytics/FUTURE_ENHANCEMENTS.md#17--arrow-zero-copy-integration-and-result-cache-with-lru-eviction) |
| 140 | api | Request Tracing and Correlation IDs | v1.7.0 | `observability`, `module:api` | #3971 | [→ Detail](api/FUTURE_ENHANCEMENTS.md#request-tracing-and-correlation-ids) |
| 141 | api | Rate Limiter — Stale Bucket Eviction and Nested Lock Contention | v2.0.0 | `performance`, `thread-safety`, `module:api` | #3972 | [→ Detail](api/FUTURE_ENHANCEMENTS.md#rate-limiter--stale-bucket-eviction-and-nested-lock-contention) |
| 142 | api | GraphQL Response Cache — Pattern-Based Invalidation | v2.0.0 | `performance`, `module:api` | #3973 | [→ Detail](api/FUTURE_ENHANCEMENTS.md#graphql-response-cache--pattern-based-invalidation) |
| 143 | api | Audit Logger — Non-Blocking Handler Dispatch | v2.0.0 | `performance`, `module:api` | #3974 | [→ Detail](api/FUTURE_ENHANCEMENTS.md#audit-logger--non-blocking-handler-dispatch) |
| 144 | aql | Runtime-Configurable Confidence Scoring Weights | v1.6.0 | `dx`, `module:aql` | #3975 | [→ Detail](aql/FUTURE_ENHANCEMENTS.md#4--runtime-configurable-confidence-scoring-weights) |
| 145 | aql | Accurate Token-Count Estimation | v1.6.0 | `correctness`, `module:aql` | #3976 | [→ Detail](aql/FUTURE_ENHANCEMENTS.md#5--accurate-token-count-estimation) |
| 146 | aql | Wire `detectIntentWithNativeNLP()` to the CLASSIFY Function | v1.7.0 | `feature`, `module:aql` | #3977 | [→ Detail](aql/FUTURE_ENHANCEMENTS.md#6--wire-detectintentwithnativenlp-to-the-classify-function) |
| 147 | aql | Parallel Execution of `translateBatchNLToAQL()` | v1.7.0 | `performance`, `module:aql` | #3978 | [→ Detail](aql/FUTURE_ENHANCEMENTS.md#7--parallel-execution-of-translatebatchnltoaql) |
| 148 | aql | `AQLQueryBuilder` — Graph Traversal and DML Support | v1.7.0 | `feature`, `module:aql` | #3979 | [→ Detail](aql/FUTURE_ENHANCEMENTS.md#11--aqlquerybuilder--graph-traversal-and-dml-support) |
| 149 | auth | JWT JTI Replay Prevention Warning | v1.2.0 | `security`, `module:auth` | #3980 | [→ Detail](auth/FUTURE_ENHANCEMENTS.md#6-jwt-jti-replay-prevention-warning-when-jti-is-absent) |
| 150 | auth | TOTP/MFA: Configurable Window and Audit on Drift | v1.2.0 | `security`, `module:auth` | #3981 | [→ Detail](auth/FUTURE_ENHANCEMENTS.md#11-totpmfa-configurable-window-and-audit-on-drift) |
| 151 | auth | Rate Limiter: Distributed State Synchronisation | v1.3.0 | `distributed`, `module:auth` | #3982 | [→ Detail](auth/FUTURE_ENHANCEMENTS.md#12-rate-limiter-distributed-state-synchronisation) |
| 152 | auth | Credential Stuffing Detection: Persistent Cross-Session State | v1.3.0 | `security`, `module:auth` | #3983 | [→ Detail](auth/FUTURE_ENHANCEMENTS.md#13-credential-stuffing-detection-persistent-cross-session-state) |
| 153 | base | WASM Non-Function Import Parsing Completeness | v1.2.0 | `correctness`, `wasm`, `module:base` | #3984 | [→ Detail](base/FUTURE_ENHANCEMENTS.md#wasm-non-function-import-parsing-completeness) |
| 154 | base | A/B Test Persistence and Observability Export | v1.3.0 | `feature`, `module:base` | #3985 | [→ Detail](base/FUTURE_ENHANCEMENTS.md#ab-test-persistence-and-observability-export) |
| 155 | base | Async Retry Back-Off in `RemoteRegistryClient` | v1.3.0 | `reliability`, `module:base` | #3986 | [→ Detail](base/FUTURE_ENHANCEMENTS.md#async-retry-back-off-in-remoteregistryclient) |
| 156 | base | Hot-Reload Reader/Writer Lock Upgrade | v1.3.0 | `thread-safety`, `module:base` | #3987 | [→ Detail](base/FUTURE_ENHANCEMENTS.md#hot-reload-readerwriter-lock-upgrade) |
| 157 | cache | Predictive Prefetcher: ML-Based Access Pattern Model | v1.8.0 | `ml`, `performance`, `module:cache` | #3988 | [→ Detail](cache/FUTURE_ENHANCEMENTS.md#predictive-prefetcher-ml-based-access-pattern-model) |
| 158 | cache | SLO Monitor: Latency Percentile Tracking | v1.8.0 | `observability`, `module:cache` | #3989 | [→ Detail](cache/FUTURE_ENHANCEMENTS.md#slo-monitor-latency-percentile-tracking) |
| 159 | cache | In-Process Replication Coordinator: Network-Backed Peer Discovery | v1.8.0 | `distributed`, `module:cache` | #3990 | [→ Detail](cache/FUTURE_ENHANCEMENTS.md#in-process-replication-coordinator-network-backed-peer-discovery) |
| 160 | cdc | Changefeed Sequence Counter: RocksDB Merge Operator | v1.8.0 | `correctness`, `module:cdc` | #3991 | [→ Detail](cdc/FUTURE_ENHANCEMENTS.md#changefeed-sequence-counter-rocksdb-merge-operator) |
| 161 | cdc | Kafka-Compatible Producer Interface | v1.9.0 | `feature`, `streaming`, `module:cdc` | #3992 | [→ Detail](cdc/FUTURE_ENHANCEMENTS.md#kafka-compatible-producer-interface) |
| 162 | chimera | Async/Promise-Based API | v1.2.0 | `async`, `module:chimera` | #3993 | [→ Detail](chimera/FUTURE_ENHANCEMENTS.md#asyncpromise-based-api) |
| 163 | chimera | Adapter Configuration Validation | v1.2.0 | `correctness`, `module:chimera` | #3994 | [→ Detail](chimera/FUTURE_ENHANCEMENTS.md#adapter-configuration-validation) |
| 164 | config | `ConfigEncryptedStore` Read-Path Lock Upgrade | v1.8.0 | `thread-safety`, `module:config` | #3995 | [→ Detail](config/FUTURE_ENHANCEMENTS.md#configencryptedstore-read-path-lock-upgrade) |
| 165 | config | Configurable LRU Cache Size and TTL via Environment Variables | v1.7.0 | `dx`, `module:config` | #3996 | [→ Detail](config/FUTURE_ENHANCEMENTS.md#configurable-lru-cache-size-and-ttl-via-environment-variables) |
| 166 | config | Multi-Environment Config Overlay (dev/staging/prod) | v1.9.0 | `dx`, `module:config` | #3997 | [→ Detail](config/FUTURE_ENHANCEMENTS.md#multi-environment-config-overlay-devstatingprod) |
| 167 | content | `AsyncIngestionWorker`: YAML Config Loading and User Context | v1.8.0 | `correctness`, `module:content` | #3998 | [→ Detail](content/FUTURE_ENHANCEMENTS.md#asyncingestionworker-yaml-config-loading-and-user-context) |
| 168 | content | Content Deduplication via Perceptual Hashing | v1.8.0 | `feature`, `module:content` | #3999 | [→ Detail](content/FUTURE_ENHANCEMENTS.md#content-deduplication-via-perceptual-hashing) |
| 169 | exporters | HuggingFace Hub Client: HTTP 429 Back-Off | v1.8.0 | `reliability`, `module:exporters` | #4000 | [→ Detail](exporters/FUTURE_ENHANCEMENTS.md#huggingface-hub-client-http-rate-limit-handling-429-back-off) |
| 170 | geo | Spatial JOIN Support | v1.6.0 | `feature`, `module:geo` | #4001 | [→ Detail](geo/FUTURE_ENHANCEMENTS.md#spatial-join-support) |
| 171 | geo | Temporal-Spatial Queries (Location at Time T) | v1.7.0 | `feature`, `module:geo` | #4002 | [→ Detail](geo/FUTURE_ENHANCEMENTS.md#temporal-spatial-queries-location-at-time-t) |
| 172 | geo | Geo Point Clustering: DBSCAN and k-means | v1.8.0 | `feature`, `module:geo` | #4003 | [→ Detail](geo/FUTURE_ENHANCEMENTS.md#geo-point-clustering-dbscan-and-k-means) |
| 173 | governance | CSV Export for Generic JSON Compliance Reports | v1.8.0 | `correctness`, `stub-replacement`, `module:governance` | #4004 | [→ Detail](governance/FUTURE_ENHANCEMENTS.md#csv-export-for-generic-json-compliance-reports) |
| 174 | graph | `DistributedGraphManager`: Read-Path Lock Upgrade | v1.8.0 | `thread-safety`, `module:graph` | #4005 | [→ Detail](graph/FUTURE_ENHANCEMENTS.md#distributedgraphmanager-read-path-lock-upgrade) |
| 175 | importers | Import Conflict Resolution Strategies | v1.7.0 | `feature`, `module:importers` | #4006 | [→ Detail](importers/FUTURE_ENHANCEMENTS.md#import-conflict-resolution-strategies) |
| 176 | index | Index Compression | v1.7.0 | `performance`, `module:index` | #4007 | [→ Detail](index/FUTURE_ENHANCEMENTS.md#index-compression) |
| 177 | index | Learned Indexes | v1.8.0 | `ml`, `module:index` | #4008 | [→ Detail](index/FUTURE_ENHANCEMENTS.md#learned-indexes) |
| 178 | ingestion | S3-Compatible Object Storage Source Connector | v1.7.0 | `feature`, `module:ingestion` | #4009 | [→ Detail](ingestion/FUTURE_ENHANCEMENTS.md#s3-compatible-object-storage-source-connector) |
| 179 | ingestion | Distributed Ingestion Coordinator | v1.8.0 | `distributed`, `module:ingestion` | #4010 | [→ Detail](ingestion/FUTURE_ENHANCEMENTS.md#distributed-ingestion-coordinator) |
| 180 | llm | `LLMDeploymentPlugin`: RocksDB Model Storage | v1.8.0 | `persistence`, `stub-replacement`, `module:llm` | #4011 | [→ Detail](llm/FUTURE_ENHANCEMENTS.md#llmdeploymentplugin-rocksdb-model-storage) |
| 181 | llm | `AIOrchestrator`: Tool Call Parsing | v1.8.0 | `correctness`, `module:llm` | #4012 | [→ Detail](llm/FUTURE_ENHANCEMENTS.md#aiorchestrator-tool-call-parsing) |
| 182 | llm | LoRA Adapter Hot-Loading at Inference Time | v1.8.0 | `feature`, `module:llm` | #4013 | [→ Detail](llm/FUTURE_ENHANCEMENTS.md#lora-adapter-hot-loading-at-inference-time) |
| 183 | maintenance | Explicit Per-Task DAG with `depends_on` | v1.2.0 | `feature`, `module:maintenance` | #4014 | [→ Detail](maintenance/FUTURE_ENHANCEMENTS.md#explicit-per-task-dag-with-depends_on) |
| 184 | maintenance | Module Task Wiring: `IMaintenanceTaskHandler` Registry | v1.2.0 | `stub-replacement`, `module:maintenance` | #4015 | [→ Detail](maintenance/FUTURE_ENHANCEMENTS.md#module-task-wiring-imaintenancetaskhandler-registry) |
| 185 | maintenance | `schedules_mutex_` Read-Path Upgrade | v1.2.0 | `thread-safety`, `module:maintenance` | #4016 | [→ Detail](maintenance/FUTURE_ENHANCEMENTS.md#schedules_mutex_-read-path-upgrade) |
| 186 | metadata | `IndexRecommender`: Access-Pattern Persistence and ML Model | v1.8.0 | `persistence`, `ml`, `module:metadata` | #4017 | [→ Detail](metadata/FUTURE_ENHANCEMENTS.md#indexrecommender-access-pattern-persistence-and-ml-model) |
| 187 | metadata | Statistics Collector | v1.6.0 | `feature`, `module:metadata` | #4018 | [→ Detail](metadata/FUTURE_ENHANCEMENTS.md#statistics-collector) |
| 188 | metadata | Schema Versioning | v1.8.0 | `feature`, `module:metadata` | #4019 | [→ Detail](metadata/FUTURE_ENHANCEMENTS.md#schema-versioning) |
| 189 | network | UDP Protocol Support | v1.8.0 | `network`, `module:network` | #4020 | [→ Detail](network/FUTURE_ENHANCEMENTS.md#udp-protocol-support) |
| 190 | network | Bandwidth Management and QoS | v1.8.0 | `network`, `module:network` | #4021 ✅ | [→ Detail](network/FUTURE_ENHANCEMENTS.md#bandwidth-management-and-qos) |
| 191 | observability | `MetricsCollector`: Upgrade to `shared_mutex` | v1.8.0 | `thread-safety`, `performance`, `module:observability` | #4022 | [→ Detail](observability/FUTURE_ENHANCEMENTS.md#metricscollecter-upgrade-to-shared_mutex-for-metric-read-path) |
| 192 | performance | Intelligent Prefetching System | v1.8.0 | `performance`, `module:performance` | #4023 | [→ Detail](performance/FUTURE_ENHANCEMENTS.md#intelligent-prefetching-system) |
| 193 | plugins | `PluginRegistry`: Upgrade Global Mutex to `shared_mutex` | v1.8.0 | `thread-safety`, `module:plugins` | #4024 | [→ Detail](plugins/FUTURE_ENHANCEMENTS.md#pluginregistry-upgrade-global-mutex-to-shared_mutex) |
| 194 | query | `CTESubquery`: Replace Phase 1 Stub | v1.7.0 | `correctness`, `stub-replacement`, `module:query` | #4025 | [→ Detail](query/FUTURE_ENHANCEMENTS.md#ctesubquery-replace-phase-1-stub) |
| 195 | query | Materialized Views & Incremental Maintenance | v1.8.0 | `feature`, `module:query` | #4026 | [→ Detail](query/FUTURE_ENHANCEMENTS.md#materialized-views--incremental-maintenance) |
| 196 | query | Query Plan Caching | v1.7.0 | `performance`, `module:query` | #4027 | [→ Detail](query/FUTURE_ENHANCEMENTS.md#query-plan-caching) |
| 197 | replication | Compressed Replication | v1.6.0 | `performance`, `module:replication` | #4028 | [→ Detail](replication/FUTURE_ENHANCEMENTS.md#compressed-replication) |
| 198 | replication | Geo-Replication with Consistency Levels | v1.7.0 | `feature`, `distributed`, `module:replication` | #4029 | [→ Detail](replication/FUTURE_ENHANCEMENTS.md#geo-replication-with-consistency-levels) |
| 199 | replication | WAL Archival to Object Storage | v1.6.0 | `feature`, `module:replication` | #4030 | [→ Detail](replication/FUTURE_ENHANCEMENTS.md#wal-archival-to-object-storage) |
| 200 | replication | Quorum-Based Reads | v1.6.0 | `correctness`, `module:replication` | #4031 | [→ Detail](replication/FUTURE_ENHANCEMENTS.md#quorum-based-reads) |
| 201 | server | `HttpServer`: Initialize Real `ShardingManager` | v1.8.0 | `correctness`, `stub-replacement`, `module:server` | #4032 | [→ Detail](server/FUTURE_ENHANCEMENTS.md#httpserver-initialize-real-shardingmanager) |
| 202 | sharding | `OrphanDetector`: Wire to `DistributedCoordinator` Transaction List | v1.8.0 | `correctness`, `stub-replacement`, `module:sharding` | #4033 | [→ Detail](sharding/FUTURE_ENHANCEMENTS.md#orphandetector-wire-to-distributedcoordinator-transaction-list) |
| 203 | sharding | Adaptive Shard Rebalancer with Load-Based Splitting | v1.7.0 | `feature`, `module:sharding` | #4034 | [→ Detail](sharding/FUTURE_ENHANCEMENTS.md#-adaptive-shard-rebalancer-with-load-based-splitting) |
| 204 | sharding | Reed-Solomon Repair Engine Parallelisation | v1.6.0 | `performance`, `module:sharding` | #4035 | [→ Detail](sharding/FUTURE_ENHANCEMENTS.md#-reed-solomon-repair-engine-parallelisation) |
| 205 | storage | `RocksDBWrapper`: Implement Proper Size Calculation | v1.8.0 | `correctness`, `module:storage` | #4036 | [→ Detail](storage/FUTURE_ENHANCEMENTS.md#rocksdbwrapper-implement-proper-size-calculation) |
| 206 | storage | `SecuritySignatureManager`: Implement RocksDB Iteration | v1.8.0 | `correctness`, `security`, `module:storage` | #4037 | [→ Detail](storage/FUTURE_ENHANCEMENTS.md#securitysignaturemanager-implement-rocksdb-iteration) |
| 207 | storage | Erasure Coding for Blob Storage | v1.7.0 | `feature`, `reliability`, `module:storage` | #4038 | [→ Detail](storage/FUTURE_ENHANCEMENTS.md#erasure-coding-for-blob-storage) |
| 208 | storage | Online Schema Migration | v1.7.0 | `feature`, `module:storage` | #4039 | [→ Detail](storage/FUTURE_ENHANCEMENTS.md#online-schema-migration) |
| 209 | storage | Adaptive Compaction | v1.7.0 | `performance`, `module:storage` | #4040 | [→ Detail](storage/FUTURE_ENHANCEMENTS.md#adaptive-compaction) |
| 210 | temporal | Automated Retention Policies | v1.3.0 | `feature`, `module:temporal` | #4041 | [→ Detail](temporal/FUTURE_ENHANCEMENTS.md#automated-retention-policies) |
| 211 | temporal | Temporal Aggregations | v1.3.0 | `feature`, `module:temporal` | #4042 | [→ Detail](temporal/FUTURE_ENHANCEMENTS.md#temporal-aggregations) |
| 212 | transaction | SAGA Orchestration Engine | v1.8.0 | `feature`, `transactions`, `module:transaction` | #4043 | [→ Detail](transaction/FUTURE_ENHANCEMENTS.md#saga-orchestration-engine) |
| 213 | transaction | Write Batching and Coalescing | v1.8.0 | `performance`, `module:transaction` | #4044 | [→ Detail](transaction/FUTURE_ENHANCEMENTS.md#write-batching-and-coalescing) |
| 214 | updates | `ManifestDatabase`: Delete Associated Files on Entry Removal | v1.8.0 | `correctness`, `module:updates` | #4045 | [→ Detail](updates/FUTURE_ENHANCEMENTS.md#manifestdatabase-delete-associated-files-on-entry-removal) |
| 215 | updates | Canary Deployments | v1.7.0 | `feature`, `module:updates` | #4046 | [→ Detail](updates/FUTURE_ENHANCEMENTS.md#canary-deployments) |
| 216 | updates | Dependency Resolution Engine | v1.6.0 | `feature`, `module:updates` | #4047 | [→ Detail](updates/FUTURE_ENHANCEMENTS.md#dependency-resolution-engine) |
| 217 | utils | `CapabilityAutoGenerator`: Persist Schedule and Document Count State | v1.8.0 | `persistence`, `correctness`, `module:utils` | #4048 | [→ Detail](utils/FUTURE_ENHANCEMENTS.md#capabilityautogenerator-persist-schedule-and-document-count-state) |
| 218 | utils | `PKIClient`: Replace Fallback Stub Verification | v1.8.0 | `security`, `stub-replacement`, `module:utils` | #4049 | [→ Detail](utils/FUTURE_ENHANCEMENTS.md#pkiclient-replace-fallback-stub-verification) |

---

## 🟡 Medium Priority — Mid-term (v1.9.0 – v2.0.0)

> Calendar: Q2–Q3 2027.

| # | Module | Title | Target | Labels | Issue | Detail |
| --- | --- | --- | --- | --- | --- | --- |
| 219 | acceleration | CUDA HNSW Kernel: Visited Array Memory Scaling | v1.9.0 | `gpu`, `memory`, `module:acceleration` | #4050 | [→ Detail](acceleration/FUTURE_ENHANCEMENTS.md#cuda-hnsw-kernel-visited-array-memory-scaling) |
| 220 | acceleration | TensorCore Matmul: INT8 Quantized Precision Path | v1.9.0 | `gpu`, `ml`, `module:acceleration` | #4051 | [→ Detail](acceleration/FUTURE_ENHANCEMENTS.md#tensorcore-matmul-int8-quantized-precision-path) |
| 221 | acceleration | FAISS GPU Backend: HNSW and ScalarQuantizer Index Types | v1.9.0 | `gpu`, `module:acceleration` | #4052 | [→ Detail](acceleration/FUTURE_ENHANCEMENTS.md#faiss-gpu-backend-hnsw-and-scalarquantizer-index-types) |
| 222 | acceleration | Multi-GPU Sharding for Large Embedding Datasets | v1.9.0 | `gpu`, `distributed`, `module:acceleration` | #4053 | [→ Detail](acceleration/FUTURE_ENHANCEMENTS.md#multi-gpu-sharding-for-large-embedding-datasets) |
| 223 | analytics | Forecasting: Batch Prediction, Streaming Update, SIMD Fit | v1.9.0 | `ml`, `module:analytics` | #4054 | [→ Detail](analytics/FUTURE_ENHANCEMENTS.md#16--forecasting-batch-prediction-streaming-update-simd-fit) |
| 224 | config | Multi-Environment Config Overlay (dev/staging/prod) | v1.9.0 | `dx`, `module:config` | #3997 | [→ Detail](config/FUTURE_ENHANCEMENTS.md#multi-environment-config-overlay-devstatingprod) |
| 225 | llm | Speculative Decoding for Latency Reduction | v1.9.0 | `performance`, `module:llm` | #4055 | [→ Detail](llm/FUTURE_ENHANCEMENTS.md#speculative-decoding-for-latency-reduction) |
| 226 | network | QUIC Protocol Support | v2.0.0 | `network`, `module:network` | #4056 | [→ Detail](network/FUTURE_ENHANCEMENTS.md#quic-protocol-support) |
| 227 | network | Kernel Bypass (DPDK/io_uring) | v1.9.0 | `performance`, `network`, `module:network` | #4057 | [→ Detail](network/FUTURE_ENHANCEMENTS.md#kernel-bypass-dpdk--io_uring) |
| 228 | performance | NUMA-Aware Memory Management | v1.9.0 | `performance`, `memory`, `module:performance` | #4058 | [→ Detail](performance/FUTURE_ENHANCEMENTS.md#numa-aware-memory-management) |
| 229 | performance | Advanced Cache Optimization | v1.9.0 | `performance`, `module:performance` | #4059 | [→ Detail](performance/FUTURE_ENHANCEMENTS.md#advanced-cache-optimization) |
| 230 | performance | Workload-Adaptive Optimization | v1.9.0 | `performance`, `ml`, `module:performance` | #4060 | [→ Detail](performance/FUTURE_ENHANCEMENTS.md#workload-adaptive-optimization) |
| 231 | storage | Zero-Copy Blob Transfers | v1.7.0 | `performance`, `io`, `module:storage` | #4061 | [→ Detail](storage/FUTURE_ENHANCEMENTS.md#zero-copy-blob-transfers) |
| 232 | transaction | Transaction Savepoints | v1.8.0 | `feature`, `transactions`, `module:transaction` | #4062 | [→ Detail](transaction/FUTURE_ENHANCEMENTS.md#transaction-savepoints) |
| 233 | transaction | Optimistic Concurrency Control (OCC) | v1.8.0 | `feature`, `transactions`, `module:transaction` | #4063 | [→ Detail](transaction/FUTURE_ENHANCEMENTS.md#optimistic-concurrency-control-occ) |

---

## 🟢 Low Priority — Future (v1.9.0+)

> Calendar: 2027+. Scheduled opportunistically.

| # | Module | Title | Target | Labels | Issue | Detail |
| --- | --- | --- | --- | --- | --- | --- |
| 234 | acceleration | Kernel Block-Dimension Occupancy Tuning | v1.9.0 | `gpu`, `performance`, `module:acceleration` | #4064 | [→ Detail](acceleration/FUTURE_ENHANCEMENTS.md#kernel-block-dimension-occupancy-tuning) |
| 235 | acceleration | OpenGL Compute Shader Backend: Complete 5 Remaining Stubs | v2.0.0 | `gpu`, `stub-replacement`, `module:acceleration` | #4065 | [→ Detail](acceleration/FUTURE_ENHANCEMENTS.md#opengl-compute-shader-backend-complete-5-remaining-stubs) |
| 236 | acceleration | BackendRegistry: O(n²) Backend Selection Index | v1.9.0 | `performance`, `module:acceleration` | #4066 | [→ Detail](acceleration/FUTURE_ENHANCEMENTS.md#backendregistry-on%C2%B2-backend-selection-index) |
| 237 | acceleration | BackendRegistry: Replace `std::cout` with Structured Logger | v1.8.0 | `dx`, `module:acceleration` | #4067 | [→ Detail](acceleration/FUTURE_ENHANCEMENTS.md#backendregistry-replace-stdcout-with-structured-logger) |
| 238 | analytics | Windows Platform Stubs — `olap.cpp` and `process_mining.cpp` | v2.0.0 | `platform`, `stub-replacement`, `module:analytics` | #4068 | [→ Detail](analytics/FUTURE_ENHANCEMENTS.md#12--windows-platform-stubs--olapcpp-and-process_miningcpp) |
| 239 | api | OTLP Exporter Performance and Reliability | v2.1.0 | `observability`, `module:api` | #4069 | [→ Detail](api/FUTURE_ENHANCEMENTS.md#otlp-exporter-performance-and-reliability) |
| 240 | auth | Zero-Trust Continuous Verification: Async Policy Re-evaluation | v1.4.0 | `security`, `module:auth` | #4070 | [→ Detail](auth/FUTURE_ENHANCEMENTS.md#14-zero-trust-continuous-verification-async-policy-re-evaluation) |
| 241 | auth | SAML Assertion Encryption Support | v1.4.0 | `security`, `module:auth` | #4071 | [→ Detail](auth/FUTURE_ENHANCEMENTS.md#15-saml-assertion-encryption-support) |
| 242 | auth | Federated Identity Manager: Token Exchange (RFC 8693) | v1.4.0 | `security`, `module:auth` | #4072 | [→ Detail](auth/FUTURE_ENHANCEMENTS.md#16-federated-identity-manager-token-exchange-rfc-8693) |
| 243 | base | Cross-Platform Module Format | v1.4.0 | `platform`, `module:base` | #4073 | [→ Detail](base/FUTURE_ENHANCEMENTS.md#cross-platform-module-format) |
| 244 | cache | Warmup: Parallel Bulk Load | v1.8.0 | `performance`, `module:cache` | #4074 | [→ Detail](cache/FUTURE_ENHANCEMENTS.md#warmup-parallel-bulk-load) |
| 245 | cdc | GDPR-Aware Change Log Redaction | v2.0.0 | `security`, `compliance`, `module:cdc` | #4075 | [→ Detail](cdc/FUTURE_ENHANCEMENTS.md#gdpr-aware-change-log-redaction) |
| 246 | chimera | Multi-Database Adapter Registration | v1.3.0 | `feature`, `module:chimera` | #4076 | [→ Detail](chimera/FUTURE_ENHANCEMENTS.md#multi-database-adapter-registration) |
| 247 | config | SIGHUP Hot-Reload: inotify-Based File Watch | v1.8.0 | `dx`, `module:config` | #4077 | [→ Detail](config/FUTURE_ENHANCEMENTS.md#sighup-hot-reload-inotify-based-file-watch) |
| 248 | config | ConfigSchemaValidator: Extended JSON Schema Keyword Support | v2.0.0 | `feature`, `module:config` | #4078 | [→ Detail](config/FUTURE_ENHANCEMENTS.md#configschemavalidator-extended-json-schema-keyword-support) |
| 249 | exporters | `StreamWriter`: Replace zlib with ZSTD as Sole Compression Backend | v1.8.0 | `cleanup`, `performance`, `module:exporters` | #4079 | [→ Detail](exporters/FUTURE_ENHANCEMENTS.md#streamwriter-replace-zlib-with-zstd-as-sole-compression-backend) |
| 250 | graph | Query Rewriting for Graph Optimization | v1.9.0 | `performance`, `module:graph` | #4080 | [→ Detail](graph/FUTURE_ENHANCEMENTS.md#query-rewriting-for-graph-optimization) |
| 251 | importers | Importer Plugin API | v1.8.0 | `feature`, `plugin`, `module:importers` | #4081 | [→ Detail](importers/FUTURE_ENHANCEMENTS.md#importer-plugin-api) |
| 252 | maintenance | Distributed Maintenance Coordination via Raft | v2.0.0 | `distributed`, `module:maintenance` | #4082 | [→ Detail](maintenance/FUTURE_ENHANCEMENTS.md#distributed-maintenance-coordination-via-raft) |
| 253 | maintenance | Multi-Tenant Schedule Isolation | v2.0.0 | `feature`, `multi-tenant`, `module:maintenance` | #4083 | [→ Detail](maintenance/FUTURE_ENHANCEMENTS.md#multi-tenant-schedule-isolation) |
| 254 | metadata | Automatic Indexing Recommendations | v1.9.0 | `feature`, `ml`, `module:metadata` | #4084 | [→ Detail](metadata/FUTURE_ENHANCEMENTS.md#automatic-indexing-recommendations) |
| 255 | network | Process Graph Visit Timestamp TODO | v1.8.0 | `correctness`, `module:network` | #4085 | [→ Detail](network/FUTURE_ENHANCEMENTS.md#wireprotocolserver-processgraph-visit-timestamp-todo) |
| 256 | performance | Phase 4: PMU Counters — Non-Linux Stub Coverage | v1.9.0 | `platform`, `observability`, `module:performance` | #4086 | [→ Detail](performance/FUTURE_ENHANCEMENTS.md#phase-4-pmu-counters--non-linux-stub-coverage) |
| 257 | rag | Adversarial Robustness Testing | v1.18.0 | `testing`, `module:rag` | #4087 | [→ Detail](rag/FUTURE_ENHANCEMENTS.md#10-adversarial-robustness-testing) |
| 258 | replication | Multi-Tier Replication | v1.8.0 | `feature`, `module:replication` | #4088 | [→ Detail](replication/FUTURE_ENHANCEMENTS.md#multi-tier-replication) |
| 259 | storage | `BlobRedundancyManager`: Implement RocksDB Event Listener | v1.8.0 | `reliability`, `module:storage` | #4089 | [→ Detail](storage/FUTURE_ENHANCEMENTS.md#blobredundancymanager-implement-rocksdb-event-listener) |
| 260 | storage | Write-Optimized Merge (WOM) Tree | v1.8.0 | `performance`, `module:storage` | #4090 | [→ Detail](storage/FUTURE_ENHANCEMENTS.md#write-optimized-merge-wom-tree) |
| 261 | transaction | Adaptive Deadlock Prevention | v1.9.0 | `correctness`, `module:transaction` | #4091 | [→ Detail](transaction/FUTURE_ENHANCEMENTS.md#adaptive-deadlock-prevention) |
| 262 | updates | Multi-Tenant Update Scheduling | v1.8.0 | `feature`, `multi-tenant`, `module:updates` | #4092 | [→ Detail](updates/FUTURE_ENHANCEMENTS.md#multi-tenant-update-scheduling) |

---

## Cross-Cutting Concerns

These themes affect multiple modules and should be tracked as **Epic-level GitHub Issues**:

### 🔧 Code Consolidation Epic
**Status:** ✅ **COMPLETE (2026-05-19)** · **Epic Label:** `epic:consolidation` · **Target:** v1.5.0

Tracks deduplication of scattered implementations across the codebase.
See `src/UNUSED_FUNCTIONS_REPORT.md` for per-symbol triage decisions.

#### Phase 1: Geometric Distance Functions (Target: v1.4.0)
- [x] `include/utils/geometric_distances.h` als zentralen Header angelegt
- [x] `src/acceleration/cpu_backend.cpp` → `simd::l2_distance_sq` / `simd::cosine_distance`
- [x] `src/acceleration/cpu_backend_mt.cpp` → `simd::cosine_distance` (80-Zeilen AVX2/NEON-Duplikat entfernt)
- [x] `src/acceleration/graphics_backends.cpp` file-local `haversine_km`-Varianten → `geo::haversine_km` (v1.9.0)
- [x] `src/geo/*.cpp` file-local `haversineDistanceM()` → `geo::haversine_m` (v1.9.0)
- [x] `src/index/secondary_index.cpp`, `src/index/spatial_index.cpp` → `geo::haversine_km` (v1.9.0)
- [x] Unit-Tests für `geometric_distances.h` ergänzen (v1.9.0)

#### Phase 2: Compression Codec Registry (Target: v1.4.0)
- [x] `include/storage/codec_tags.h` als centrales Tag-Byte-Register angelegt
- [x] `src/performance/advanced_cache_manager.cpp` auf centrale Tags umgestellt
- [x] `src/storage/compression_strategy.h` auf `codec_tags.h` umstellen: `#include "storage/codec_tags.h"` + `method_to_tag()` / `tag_to_method()` constexpr bridge (v1.9.0)

#### Phase 3: Cache Interface Konsolidierung (Target: v1.5.0)
- [x] `ICacheBackend<K,V>` zu `include/cache/cache_interfaces.h` hinzugefügt
- [x] `AdaptiveQueryCache` von `ICacheBackend<std::string, nlohmann::json>` erben lassen (v1.9.0)
- [x] `BoundedLRUCache` von `ICacheBackend` erben lassen (v1.9.0)
- [x] `llm/active_vram_allocator.cpp` lokale LRU-Logik gegen `ICacheBackend`-Implementierung tauschen (Target: v1.6.0)
  > **WONTFIX — Interface-Mismatch (v1.9.0):** `ICacheBackend<K,V>` ist für allgemeine String→JSON-Caches ausgelegt.
  > Der `ActiveVRAMAllocator` verwaltet `uint64_t`-Keys → GPU-Device-Pointer + CPU-Spill-Buffer + externe Allocations.
  > Die domain-spezifischen Operationen (`spillLRUToCPU`, `defragment`, `external`-Flag) passen nicht in das Interface.
  > Der O(n)-LRU-Scan ist für VRAM-Allocations (typisch < 100 Einträge) nicht bottleneck-relevant.
  > Entscheidung: Status quo beibehalten; eine eigene `IVRAMAllocatorPolicy`-Schnittstelle wäre der korrekte Weg wenn needed.

#### Phase 4: UNGENUTZT-Symbole Triage (Target: v1.4.0)
- [x] Entscheidungsmatrix für 35 Symbole ausgefüllt (UNUSED_FUNCTIONS_REPORT.md)
- [x] `EnumerateCUDA`, `EnumerateROCm`, `MakeCPUFallback` → INTERNAL_ONLY (falsch klassifiziert)
- [x] `getHooks`, `hookId`, `registerHook`, `unregisterHook` → CANDIDATE_FOR_REMOVAL (kein Signal)
- [x] GitHub Issues für alle 4 CANDIDATE_FOR_REMOVAL anlegen (v1.9.0)
  > `[[deprecated]]` + Doxygen `@deprecated` zu `hookId`, `registerHook`, `unregisterHook`, `getHooks` in `include/api/api_gateway_hook.h` ergänzt.
  > Compiler emittiert Deprecation-Warnung bei jedem Aufrufer; externe Ticket-Anlage nach nächstem Release.
- [x] Mindest-Tests für KEEP-Symbole mit Status UNGENUTZT anlegen (v1.9.0)
  > - `logCapabilities` (AiHardwareDispatcher): LC-01..02 in `tests/test_acceleration.cpp`
  > - `attackCategoryName` (prompt_engineering): ACN-01..03 in `tests/test_prompt_engineering_phase6.cpp`
  > - `strengthToScore` (ethics_ai, static): CC-01..05 via `EthicsEvaluator::computeConfidence` in `tests/test_ethics_ai_pipeline.cpp`
  > - `AIPluginGenerator::generatePlugin`: APG-01..06 in `tests/test_ai_plugin_generator.cpp` + minimal impl in `src/plugins/ai/ai_plugin_generator.cpp`
  > - `parseWav` (WavAudioChunkReader): already covered by `src/whisper/tests/test_whisper_plugin.cpp` (Group C)

#### Phase 5: Stub/Simulation Lifecycle (Target: v1.4.0)
- [x] `src/query/optimizer_cost_model.cpp` Statistik-Stubs: Roadmap-Referenz + Target v2.0.0 ergänzt
- [x] `src/governance/opa_adapter.cpp` WASM-Stub: Roadmap-Referenz + Target v1.6.0 ergänzt
- [x] `src/performance/advanced_cache_manager.cpp` Passthrough-Stub: Roadmap-Referenz ergänzt
- [x] `src/stubs.cpp` LoRA-Stubs prüfen ob mit `llm/lora_*.h` synchron — `getFeedbackForAdapter` Signatur korrigiert: `unsigned __int64` → `std::size_t` (v1.9.0)
- [x] `src/stubs.cpp` Mock-Implementierungen entfernt — `Feedback`, `LoRATrainingConfig`, `TrainingTriggerPlugin`, `CacheAwareWeightingPlugin`, `FeedbackStorageService` migriert zu ihren kanonischen Quellen in `src/llm/lora_framework/`; Datei bleibt als leerer Platzhalter erhalten (v1.9.0)
- [x] `tests/CMakeLists.txt` `THEMIS_ENABLE_DEV_STUBS`-Block entfernt (stubs.cpp ist jetzt leer) (v1.9.0)
- [x] Alle verbleibenden STUB/SIMULATION-Blöcke ohne Roadmap-Referenz bereinigen: `Roadmap ref:` in 18+19 STUB-Blöcken in 13+18 Dateien ergänzt (v1.9.0)
- [x] **Zentrales STUB-Inventory**: `src/STUB_INVENTORY.md` mit 31 Einträgen erstellt; auto-scan Befehl dokumentiert (v1.9.x)

#### Phase 6: Retry/Backoff Zentralisierung (Target: v1.5.0)
- [x] `include/utils/retry_policy.h` mit `RetryConfig`, `ExponentialBackoff`, `retry_with_backoff<>` angelegt
- [x] `src/rag/http_metrics_client.cpp` `requestWithRetry()` → `retry_with_backoff()` / iterative `ExponentialBackoff` (v1.9.0)
- [x] `src/rag/llm_judge_integration.cpp` inline-while-loop → `retry_with_backoff()` (v1.9.0)
- [x] `src/network/` Subsysteme → `retry_with_backoff()` analysiert (v1.9.0)
  > `wire_protocol_connection_pool.cpp` nutzt einen deadline-bounded `cv.wait_until()`-Loop — kein attempt-bounded Retry.
  > Der 100ms-Sleep im catch-Block ist ein exception-recovery-Pause, kein Retry-Policy-Kandidat.
  > Alle anderen network-Subsysteme: single-shot sleeps (drain-timeout, QoS-throttle).
  > Kein Migrationsbedarf. `retry_policy.h`-Kommentar aktualisiert.
- [x] Unit-Tests für `retry_policy.h` ergänzen + Assertions für `bo.attempts()` nach ok2/ok3 (v1.9.0)
- [x] **Tier 1 (Phase 2a):** `src/exporters/huggingface_hub_client.cpp` — beide exponential-backoff Retry-Loops (file-upload + shard-upload) auf `ExponentialBackoff` umgestellt; `src/updates/parallel_downloader.cpp` — Retry-Loop auf `ExponentialBackoff` umgestellt. Delay-Sequenz identisch zur vorherigen Impl. (v2.0+)
  > Regression-Tests `HubClientDelaySequenceMatchesOldImpl` + `ParallelDownloaderDelaySequenceMatchesOldImpl` in `tests/test_retry_policy.cpp`.
- [x] **Tier 2 (Phase 2b):** `src/sharding/wal_applier.cpp` linearer Backoff 100×(attempt+1) ms → `ExponentialBackoff` (initial=100 ms, multiplier=2.0, jitter=0). `WALApplierConfig` um `retry_initial_delay_ms` erweitert; Regression-Test `WALApplierDelaySequenceIsExponential` in `tests/test_retry_policy.cpp`. (v2.0+)

---

### 🔒 Security Hardening Epic
**Status:** ✅ **COMPLETE (2026-05-19)** · **Epic Label:** `epic:security-hardening` · **Target:** v1.8.0

- Auth: JWT JWKS `shared_mutex`, LDAP DN/filter injection prevention, constant-time TOTP compare, mandatory issuer+audience validation (items #1–6 ✅)
- LLM: LoRA cert store TLS verification via injectable `VerifyCertFn` (#75 ✅)
- Server: JWT scope-based RBAC in VectorApiHandler (#100 ✅); ROPE `requireAccess()` scope enforcement (#280 ✅); Voice API JWT middleware injection (#302 ✅)
- Utils: PKI client HKDF-hash comparison (#218 ✅)
- Security: Arrow plugin bridge (#99 ✅); AQL injection AST-level validation (#27 ✅)
- Storage: SecuritySignatureManager `iterateRange()` via RocksDBWrapper (#206 ✅)

---

### 🔀 Concurrency / Thread-Safety Epic
**Status:** ✅ **COMPLETE (2026-05-19)** · **Epic Label:** `epic:thread-safety` · **Target:** v1.8.0

Affects: `analytics`, `acceleration`, `cache`, `config`, `graph`, `maintenance`, `observability`, `plugins`

All modules upgraded from exclusive mutexes on read paths to `std::shared_mutex`:
- [x] `analytics` items #41–45 — all lock-under-callback / lock-under-inference issues resolved:
  - **#41 `CEPEngine::timerLoop()`**: snapshot window events + timestamps under `windows_mutex_`, release, then dispatch user callbacks; same pattern in `closeWindow()` and `expiryLoop()`.
  - **#42 `StreamingAnomalyDetector::process()`**: separate `window_mu_` and `detector_mu_` (`std::shared_mutex`); training runs fully off-lock via `std::async`; `predict()` under `shared_lock`; stress test `StreamingConcurrencyStress.EightProducersP99Latency` validates P99 ≤ 1 ms.
  - **#43 `ModelServingEngine::predict()`**: `shared_mutex` registry; `lookupEntryOrThrow_()` captures a `shared_ptr<Entry>` under a brief `shared_lock`, releases lock, runs inference outside any registry lock; `health_mu` per-entry for metrics. Concurrent-readers test + concurrent-unregister test + throughput benchmark added.
  - **#44 `MLServingEngine::infer()`**: `shared_mutex sessions_mutex`; per-model `std::mutex` serialises concurrent loads of the same model without blocking unrelated models; `OrtSession::Run()` executes outside `sessions_mutex`. Two-thread concurrent inference test validates no inter-model blocking.
  - **#45 `IncrementalView::applyChanges()`**: micro-batch loop (≤ 256 rows/batch); exclusive lock acquired + released per micro-batch; `std::this_thread::yield()` between batches; `passesBaseFilters()` pre-computed outside the lock. Read-latency regression test `IncrementalViewPerfTest.ReaderP99DuringBatchApply` validates P99 ≤ 10 ms.
  - **`ExpertSystemEngine`** (v1.8.0, 2026-05-19): `std::mutex` → `std::shared_mutex`; read-only methods use `shared_lock`; `forwardChain()` snapshots scorer state under lock, releases lock before external scorer callback, re-acquires before writing; tests ES-21 (concurrent readers) + ES-22 (scorer-callback no-deadlock).
- ~~`observability` MetricsCollector #191~~ ✅ Done (v1.8.0)
- ~~`plugins` PluginRegistry #193~~ ✅ Done (already `std::shared_mutex`)
- ~~`maintenance` schedules_mutex_ #185~~ ✅ Done (already `std::shared_mutex`)
- ~~`graph` DistributedGraphManager #174~~ ✅ Done (already `std::shared_mutex`)
- ~~`config` ConfigEncryptedStore #164~~ ✅ Done (already `std::shared_mutex`)

---

### 🔌 Stub/Mock Replacement Epic
Affects: `analytics`, `chimera`, `content`, `gpu`, `index`, `geo`, `governance`, `ingestion`, `llm`, `query`, `rag`, `security`, `server`, `sharding`, `storage`, `training`, `utils`

All items with label `stub-replacement`. Most critical:
- `gpu/query_accelerator.cpp`: 5 GPU stubs (#28)
- `index/gpu_vector_index.cpp`: CUDA/HIP never dispatched (#29)
- `chimera`: 7 ThemisDB adapter stubs + 3 in-process DB simulations (#14, #15)
- `rag`: mock mode silently returns fixed evaluation scores (#94)
- `query`: federation always broadcasts to all shards (#88)

**Suggested Epic Label:** `epic:stub-replacement` · **Target:** v1.8.0

---

### 🏗️ Build System & Infrastructure Epic
Affects: `themis`, `acceleration`, `analytics`

- Themis: Modular Build System + Module Loader (Critical, #3–4)
- Themis: Wire Protocol V2 (#115), Build Reproducibility (#114)

**Suggested Epic Label:** `epic:infrastructure` · **Target:** v1.7.0

---

### 🚀 GPU Compute Epic
Affects: `acceleration`, `geo`, `gpu`, `index`, `sharding`, `storage`

GPU backend hardening, portability validation, and operational rollout for the
formerly stubbed CUDA/HIP/OpenCL/Vulkan feature set:
- #28, #29, #30, #35, #36, #37, #38, #39, #105 (implementation complete; focus shifts to benchmarking, hardware validation, and ops readiness)

**Suggested Epic Label:** `epic:gpu-compute` · **Target:** v1.8.0

---

### 🌐 Distributed Systems Epic
Affects: `sharding`, `replication`, `transaction`, `maintenance`, `cache`, `query`

- Cross-shard transaction correctness (#106, #107, #108)
- Replication (#95–100, #197–200)
- Distributed 2PC + SAGA (#123, #124)
- Query federation shard routing (#88)

**Suggested Epic Label:** `epic:distributed-systems` · **Target:** v1.9.0

---

## Milestone Summary

### v1.1.0 — Q2 2026 (Immediate)
**Focus:** Security fixes, persistence, adapter wiring

| Count | Priority | Modules |
|-------|----------|---------|
| 4 | 🔴 Critical | auth, themis |
| 10 | 🟠 High | auth, base, chimera, maintenance, temporal |
| 3 | 🟡 Medium | auth |

→ **17 open items**
→ Primary labels: `security`, `persistence`, `stub-replacement`

---

### v1.2.0 – v1.4.0 — Q2–Q3 2026 (Short-term)
**Focus:** Security hardening, concurrency, base module completeness

| Count | Priority | Modules |
|-------|----------|---------|
| 5 | 🟠 High | auth, base, chimera, geo, security, temporal |
| 10 | 🟡 Medium | auth, base, chimera, maintenance, temporal |
| 5 | 🟢 Low | auth, base |

→ **20 open items**
→ Primary labels: `security`, `thread-safety`, `wasm`, `transactions`

---

### v1.5.0 – v1.6.0 — Q3 2026 (Short-term)
**Focus:** Core features, replication, query optimizer, observability

| Count | Priority | Modules |
|-------|----------|---------|
| 15 | 🟠 High | aql, core, geo, index, ingestion, observability, query, replication, server, sharding, storage, updates |
| 10 | 🟡 Medium | aql, analytics, ingestion, replication, storage, updates |

→ **25 open items**
→ Primary labels: `feature`, `performance`, `observability`, `distributed`

---

### v1.7.0 – v1.8.0 — Q4 2026 – Q1 2027 (Near-term)
**Focus:** Production hardening, GPU backends, API completeness, concurrency

| Count | Priority | Modules |
|-------|----------|---------|
| 50 | 🟠 High | acceleration, analytics, api, cache, cdc, config, content, gpu, importers, ingestion, llm, network, observability, query, rag, replication, scheduler, security, server, sharding, storage, themis, timeseries, training, transaction, updates |
| 60 | 🟡 Medium | (most modules) |
| 15 | 🟢 Low | various |

→ **~125 open items** — largest milestone
→ Primary labels: `stub-replacement`, `thread-safety`, `gpu`, `performance`, `security`

---

### v1.9.0 – v2.0.0 — Q2–Q3 2027 (Mid-term)
**Focus:** Distributed systems, advanced performance, platform coverage

| Count | Priority | Modules |
|-------|----------|---------|
| 8 | 🟠 High | api, sharding, transaction |
| 10 | 🟡 Medium | acceleration, analytics, config, llm, network, performance, storage, transaction |
| 6 | 🟢 Low | cdc, chimera, config, graph, maintenance |

→ **24 open items**
→ Primary labels: `distributed`, `gpu`, `performance`, `platform`

---

### v2.1.0+ — 2027+ (Long-term)
**Focus:** Research features, advanced hardware, ecosystem expansion

→ Primary labels: `research`, `long-term`

---

## Suggested GitHub Label Taxonomy

### Priority Labels
```
P0-critical   (color: #d73a4a)
P1-high       (color: #e4e669)
P2-medium     (color: #0075ca)
P3-low        (color: #cfd3d7)
```

### Category Labels
```
security          (color: #d73a4a)  Security vulnerability or hardening
thread-safety     (color: #e4e669)  Concurrency / race conditions
stub-replacement  (color: #f9d0c4)  Replace stub/mock with real implementation
correctness       (color: #e4e669)  Bug or wrong result
performance       (color: #0075ca)  Performance improvement
gpu               (color: #1d76db)  GPU / CUDA / HIP / OpenCL / Vulkan
distributed       (color: #5319e7)  Distributed systems
persistence       (color: #006b75)  Persistence / storage / RocksDB
async             (color: #0052cc)  Async / non-blocking implementation
observability     (color: #bfd4f2)  Metrics / tracing / logging
ml                (color: #c2e0c6)  Machine learning / inference
api               (color: #c5def5)  API / REST / gRPC / GraphQL
streaming         (color: #bfd4f2)  Streaming / CDC / Kafka
testing           (color: #e4e669)  Test coverage
dx                (color: #cfd3d7)  Developer experience / tooling
cleanup           (color: #cfd3d7)  Code cleanup / tech debt
platform          (color: #fef2c0)  Platform compatibility (Windows, macOS, etc.)
compliance        (color: #d4c5f9)  GDPR / HIPAA / audit
```

### Module Labels
One per module: `module:acceleration`, `module:analytics`, `module:api`, `module:aql`, `module:auth`, `module:base`, `module:cache`, `module:cdc`, `module:chimera`, `module:config`, `module:content`, `module:core`, `module:exporters`, `module:geo`, `module:governance`, `module:gpu`, `module:graph`, `module:importers`, `module:index`, `module:ingestion`, `module:llm`, `module:maintenance`, `module:metadata`, `module:network`, `module:observability`, `module:performance`, `module:plugins`, `module:prompt_engineering`, `module:query`, `module:rag`, `module:replication`, `module:scheduler`, `module:search`, `module:security`, `module:server`, `module:sharding`, `module:storage`, `module:temporal`, `module:themis`, `module:timeseries`, `module:training`, `module:transaction`, `module:updates`, `module:utils`, `module:voice`

### Epic Labels
```
epic:security-hardening    All security vulnerability fixes
epic:thread-safety         All concurrency/mutex upgrade items
epic:stub-replacement      All stub/mock → real implementation items
epic:gpu-compute           All GPU backend items
epic:distributed-systems   All cross-node distributed items
epic:infrastructure        Build system, module loader, wire protocol
epic:query-engine          Query optimizer, JIT, federation
epic:llm-integration       LLM inference, LoRA, training pipeline
```

---

*Generated: 2026-04-04 · Based on: `src/*/FUTURE_ENHANCEMENTS.md` (50 modules)*
*Next update: when any module FUTURE_ENHANCEMENTS.md is changed.*
*Issues tracker: [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)*
