# ThemisDB v1.8.0 — Release Aggregation

**Release Date:** TBD (Target: Q2 2026)
**Type:** Feature Release
**Previous Version:** v1.7.0
**Milestone:** current (`milestone:current`)
**Aggregation Issue:** [makr-code/ThemisDB#4300](https://github.com/makr-code/ThemisDB/issues/4300)

---

## 🎯 Overview

ThemisDB v1.8.0 is a broad feature and hardening release spanning 54 merged pull requests. It delivers
Serializable Snapshot Isolation, versioned API routing (`/v1/` + `/v2/`), a complete SAGA orchestration
engine, Markov-chain predictive prefetching, Arrow-backed user registration, JWT scope enforcement,
CRL/OCSP certificate revocation checking, hardware-accelerator operator completeness, concrete
Arrow/Parquet/Feather exporters, geo clustering (DBSCAN + K-means), distributed graph read-path locking,
Wire Protocol V2 RFC 7540 compliance, and dozens of additional module-level improvements across
authentication, caching, config hot-reload, network QoS, sharding, storage, timeseries, and CI
infrastructure.

---

## ⚠️ Breaking Changes

| # | Module | Change | Migration |
|---|--------|--------|-----------|
| 1 | **exporters** | `StreamWriter` compression backend replaced from `zlib` to `ZSTD` (PR #4252) | Replace `libz` link dependency with `libzstd`; wire format now uses ZSTD frames; existing compressed files can be read with a migration utility |
| 2 | **server** | Unversioned HTTP paths now return 301 to `/v1/` by default via `RouteVersionRouter` (PR #4285) | Clients relying on bare paths (e.g. `/query`) must update to `/v1/query`; curl one-liners: add `/v1/` prefix |
| 3 | **ci** | GitHub Actions workflows reorganised into 9 functional categories (PR #4290) | Update any external scripts referencing `.github/workflows/` file paths — see `.github/WORKFLOW_REGISTRY.md` for the new path mapping |

---

## 📦 Included Pull Requests

### Post-v1.7.0 Merged PRs

| PR # | Module | Purpose |
|------|--------|---------|
| [#4250](https://github.com/makr-code/ThemisDB/pull/4250) | **cache** | Warmup Parallel Bulk Load — concurrent pre-population at startup; closes Issue #244 |
| [#4251](https://github.com/makr-code/ThemisDB/pull/4251) | **acceleration** | Replace `std::cout` with structured logger in `BackendRegistry` |
| [#4252](https://github.com/makr-code/ThemisDB/pull/4252) | **exporters** | Replace zlib with ZSTD as sole `StreamWriter` compression backend |
| [#4253](https://github.com/makr-code/ThemisDB/pull/4253) | **config** | SIGHUP hot-reload via inotify / kqueue / ReadDirectoryChangesW file watchers |
| [#4254](https://github.com/makr-code/ThemisDB/pull/4254) | **network / process** | `ProcessGraphVisitLog` — per-node visit timestamps for process graph traversal |
| [#4256](https://github.com/makr-code/ThemisDB/pull/4256) | **plugins** | Upgrade `PluginRegistry` global mutex to `std::shared_mutex` + WASM kernel scaffold |
| [#4257](https://github.com/makr-code/ThemisDB/pull/4257) | **performance** | Intelligent Prefetching System — Markov-chain + ToD prefetcher (Issue #192) |
| [#4258](https://github.com/makr-code/ThemisDB/pull/4258) | **query** | Materialized Views & Incremental Maintenance (Issue #195) |
| [#4259](https://github.com/makr-code/ThemisDB/pull/4259) | **sharding** | Wire `OrphanDetector` to `DistributedCoordinator`; complete Shard RPC Integration (Issue #202) |
| [#4260](https://github.com/makr-code/ThemisDB/pull/4260) | **storage** | `SecuritySignatureManager` — implement RocksDB full iteration (Issue #206) |
| [#4261](https://github.com/makr-code/ThemisDB/pull/4261) | **updates** | `ManifestDatabase::deleteManifest()` — clean up associated files on entry removal |
| [#4262](https://github.com/makr-code/ThemisDB/pull/4262) | **sharding / server** | Inject live `ShardingManager` into `HttpServer`; add `/v1/admin/shards` endpoints |
| [#4263](https://github.com/makr-code/ThemisDB/pull/4263) | **security** | PKIClient v1.8.0 — replace fallback stub; complete PII streaming pipeline |
| [#4264](https://github.com/makr-code/ThemisDB/pull/4264) | **transaction** | OCC CI workflow + test correctness audit |
| [#4265](https://github.com/makr-code/ThemisDB/pull/4265) | **sharding** | `GpuErasureCoderOpenCL` — GPU-accelerated encode / decode / batchEncode (Issue #105) |
| [#4266](https://github.com/makr-code/ThemisDB/pull/4266) | **themis** | Wire Protocol V2 — RFC 7540 PRIORITY compliance fixes + audit |
| [#4267](https://github.com/makr-code/ThemisDB/pull/4267) | **themis** | Wire Protocol V2 — complete all 4 ACs + RFC 7540 §6.3 / §5.3.1 compliance |
| [#4268](https://github.com/makr-code/ThemisDB/pull/4268) | **observability** | `ProvenanceTracker` — replace AQL template stubs with live engine connection |
| [#4269](https://github.com/makr-code/ThemisDB/pull/4269) | **timeseries** | `TSStore` single-point insert buffering for Gorilla compression + SIMD decode dispatch |
| [#4270](https://github.com/makr-code/ThemisDB/pull/4270) | **acceleration / auth / scheduler** | Multi-GPU NVML device monitoring; JWT scope enforcement hooks; `TaskScheduler` user-context propagation |
| [#4271](https://github.com/makr-code/ThemisDB/pull/4271) | **network** | UDP Protocol Support — fire-and-forget ingestion server (Issue #190) |
| [#4272](https://github.com/makr-code/ThemisDB/pull/4272) | **observability** | Upgrade `MetricsCollector` mutex to `std::shared_mutex` for concurrent Prometheus read path |
| [#4273](https://github.com/makr-code/ThemisDB/pull/4273) | **network** | Bandwidth Management and QoS (Issue #190) |
| [#4274](https://github.com/makr-code/ThemisDB/pull/4274) | **storage** | `RocksDBWrapper` proper size via SST property + `/v1/admin/storage/stats` endpoint (Issue #205) |
| [#4275](https://github.com/makr-code/ThemisDB/pull/4275) | **utils** | `CapabilityAutoGenerator` — persist schedule state + YAML capability output (Issue #217) |
| [#4276](https://github.com/makr-code/ThemisDB/pull/4276) | **transaction** | Transaction Savepoints CI + v1.8.0 CHANGELOG (roadmap #232) |
| [#4277](https://github.com/makr-code/ThemisDB/pull/4277) | **rag** | Replace `LLMIntegration` / `LLMJudgeIntegration` stub/mock mode with real engine |
| [#4278](https://github.com/makr-code/ThemisDB/pull/4278) | **scheduler** | `TaskScheduler` — propagate authenticated user context to audit events |
| [#4279](https://github.com/makr-code/ThemisDB/pull/4279) | **auth** | JWT scope extraction + role-to-scope mapping enforcement |
| [#4280](https://github.com/makr-code/ThemisDB/pull/4280) | **security** | Apache Arrow integration for `ArrowUserRegistrationPlugin` (Issue #99) |
| [#4281](https://github.com/makr-code/ThemisDB/pull/4281) | **transaction** | Serializable Snapshot Isolation (SSI) — `IsolationLevel::SerializableSnapshot` (Issue #122) |
| [#4283](https://github.com/makr-code/ThemisDB/pull/4283) | **acceleration** | CRL / OCSP certificate revocation checking in `PluginSecurityVerifier` (Issue #38) |
| [#4284](https://github.com/makr-code/ThemisDB/pull/4284) | **analytics** | `ExporterFactory` stub replacement — concrete Arrow / Parquet / Feather / JSON exporters (Issue #3868) |
| [#4285](https://github.com/makr-code/ThemisDB/pull/4285) | **server** | Versioned API Routing (`RouteVersionRouter`, `/v2/` bulk NDJSON + SSE + async jobs) |
| [#4286](https://github.com/makr-code/ThemisDB/pull/4286) | **config** | Config Audit Trail — atomic hot-path + concurrency test |
| [#4287](https://github.com/makr-code/ThemisDB/pull/4287) | **content** | Wire `abuse_detector.cpp` into CMake build — closes abuse detection stub replacement |
| [#4288](https://github.com/makr-code/ThemisDB/pull/4288) | **importers** | MySQL / MariaDB importer wire & verify |
| [#4289](https://github.com/makr-code/ThemisDB/pull/4289) | **performance** | `HardwareAccelerator` — complete AC-4 filter operator coverage; AC-5 comment alignment (Issue #85) |
| [#4290](https://github.com/makr-code/ThemisDB/pull/4290) | **ci** | Reorganise 138 GitHub workflows into 9 functional categories |
| [#4291](https://github.com/makr-code/ThemisDB/pull/4291) | **analytics** | `CEPEngine` — release window lock before invoking user callbacks (deadlock fix) |
| [#4292](https://github.com/makr-code/ThemisDB/pull/4292) | **acceleration** | PE certificate table extraction + ELF sidecar support (Issue #3960) |
| [#4294](https://github.com/makr-code/ThemisDB/pull/4294) | **cdc** | CDC audit v1.8.0 — mark sequence counter tasks complete, update `AUDIT.md` |
| [#4295](https://github.com/makr-code/ThemisDB/pull/4295) | **config** | `ConfigEncryptedStore` — upgrade `mutex_` to `std::shared_mutex` for concurrent reads |
| [#4296](https://github.com/makr-code/ThemisDB/pull/4296) | **ingestion** | YAML config loading + `user_context` propagation in `AsyncIngestionWorker` |
| [#4297](https://github.com/makr-code/ThemisDB/pull/4297) | **analytics** | `JoinExporter` — cross-collection hash-join export with PII redaction + memory budget |
| [#4299](https://github.com/makr-code/ThemisDB/pull/4299) | **graph** | `DistributedGraphManager` — read-path lock upgrade to `std::shared_mutex` |

### Feature PRs (from FUTURE_ENHANCEMENTS / module roadmaps — no standalone PR assigned)

| Feature | Module | Status |
|---------|--------|--------|
| SAGA Orchestration Engine | **transaction** | ✅ Merged |
| PredictivePrefetcher Markov ML (14-test suite) | **cache** | ✅ Merged |
| Geo Clustering — DBSCAN + K-means (Issue #4003) | **geo** | ✅ Merged |
| PolicyManager hot-reload (`reloadPolicies`, double-buffer swap) | **governance** | ✅ Merged |
| HuggingFace Hub 429 back-off + `Retry-After` parsing | **exporters** | ✅ Merged |

---

## ✨ New Features

### Auth Module — JWT Scope Enforcement (PR #4279, #4270)

> **Files:** `include/auth/jwt_validator.h`, `src/auth/jwt_validator.cpp`,
> `include/server/auth_middleware.h`, `src/server/auth_middleware.cpp`,
> `tests/test_auth_middleware.cpp`, `config/security/rbac_roles.yaml`,
> `.github/workflows/02-feature-modules_security_auth-middleware-jwt-scope-ci.yml`

- `JWTClaims` gains a `scopes` field populated from the `scope` / `scp` OAuth2 claim.
- `AuthMiddleware::authorizeViaJWT()` checks `required_scope` against `granted_scopes` plus `role_scope_map_`.
- `AuthMiddleware::authorizeViaKerberos()` checks roles + `role_scope_map_`.
- `setRoleScopeMapping()` public API for runtime role-to-scope configuration.
- `setJWKSForTesting()` helper for hermetic unit tests.
- Tests: `JWTScopeEnforcementTest` suite in `tests/test_auth_middleware.cpp`.
- CI: `auth-middleware-jwt-scope-ci.yml`.

### Security Module — ArrowUserRegistrationPlugin (PR #4280, Issue #99)

> **Files:** `include/security/arrow_user_registration_plugin.h`,
> `src/security/arrow_user_registration_plugin.cpp`,
> `tests/security/test_arrow_user_registration_plugin.cpp`,
> `.github/workflows/02-feature-modules_security_arrow-user-registration-plugin-ci.yml`

- Apache Arrow-backed in-memory user store using `arrow::StringArray` columns for `user_id`, `password_hash`, `roles`, and `email`.
- `bulkSyncFromArrow()` upserts records from an Arrow `RecordBatch` into `user_store_` under `store_mutex_`.
- `authenticateFromArrow()` verifies credentials against a SHA-256 password hash.
- 13 tests in `tests/security/test_arrow_user_registration_plugin.cpp`.
- CI: `arrow-user-registration-plugin-ci.yml`.

### Acceleration Module — CRL / OCSP Certificate Revocation (PR #4283, Issue #38)

> **Files:** `include/acceleration/plugin_security.h`, `src/acceleration/plugin_security.cpp`,
> `tests/test_plugin_security_crl_ocsp.cpp`,
> `.github/workflows/plugin-security-crl-ocsp-ci.yml`

- `PluginSecurityVerifier::checkCRL()` — HTTP CRL download via libcurl, OpenSSL DER parse, signature verify, serial lookup, timestamp bounds; per-serial cache with `nextUpdate` TTL.
- `PluginSecurityVerifier::checkOCSP()` — HTTP OCSP request, status parse, 1-hour per-serial cache.
- PE certificate extraction parses `DataDirectory[4] WIN_CERTIFICATE`; ELF sidecar `.security` section also supported (PR #4292).
- 24 tests in `tests/test_plugin_security_crl_ocsp.cpp`.
- CI: `plugin-security-crl-ocsp-ci.yml`.

### Transaction Module — Serializable Snapshot Isolation (PR #4281, Issue #122)

> **Files:** `include/transaction/isolation_level.h`, `include/transaction/lock_manager.h`,
> `include/transaction/transaction_manager.h`, `src/transaction/lock_manager.cpp`,
> `src/transaction/transaction_manager.cpp`, `src/storage/transaction_retry_manager.cpp`,
> `tests/test_transaction_ssi.cpp`,
> `.github/workflows/02-feature-modules_transactions_transaction-ssi-ci.yml`

- `IsolationLevel::SerializableSnapshot = 4` — new isolation level alias.
- `SSIConfig { enable_predicate_locking, max_predicate_locks = 10000, conflict_detection_interval }`.
- `setSSIConfig()` / `getSSIConfig()` configuration API.
- `SerializationConflict` struct + `detectConflicts()` using range intersection (`s1 ≤ e2 && s2 ≤ e1`).
- `LockManager::getPredicateLockRanges(txn_id)`, `setPredicateLockingEnabled()`, `setMaxPredicateLocks()`.
- 38 tests in `tests/test_transaction_ssi.cpp`.
- CI: `transaction-ssi-ci.yml`.

### Transaction Module — SAGA Orchestration Engine

> **Files:** `include/transaction/saga_orchestrator.h`, `src/transaction/saga_orchestrator.cpp`,
> `tests/test_saga_orchestrator.cpp`,
> `.github/workflows/saga-orchestration-engine-ci.yml`

- Full SAGA execution with compensating transactions for distributed workflow coordination.
- `SAGAOrchestrator::execute()` / `validate()` / `getStatus()` / `getMetrics()` API.
- Template management: `registerTemplate()` / `instantiateTemplate()` / `renderWorkflow()`.
- `SAGAExecutionStatus` with `step_states`, `completed_steps`, `failed_steps`, `skipped_steps`.
- `StepState` enum covering all SAGA step lifecycle states.
- `Config` supports `enable_parallel`, `default_timeout`, `journal_path`.
- 23 tests in `tests/test_saga_orchestrator.cpp` (SAGAOrchestratorTest suite).
- CI: `saga-orchestration-engine-ci.yml`.

### Server Module — Versioned API Routing (PR #4285)

> **Files:** `include/server/route_version_router.h`, `src/server/route_version_router.cpp`,
> `include/server/entity_api_handler.h`, `src/server/entity_api_handler.cpp`,
> `include/server/query_api_handler.h`, `src/server/query_api_handler.cpp`,
> `include/server/async_job_api_handler.h`, `src/server/async_job_api_handler.cpp`,
> `tests/test_versioned_api_routing.cpp`,
> `.github/workflows/versioned-api-routing-ci.yml`

- `RouteVersionRouter` middleware: unversioned paths redirect 301 to `/v1/`; `/v2/` routes selectively opt in to new behaviours.
- `/v2/entities/bulk` — `EntityApiHandler::handleBulkNdjson()` for bulk document insert via NDJSON streams.
- `/v2/query/stream` — `QueryApiHandler::handleQueryStreamSse()` for SSE streaming query results.
- `AsyncJobApiHandler` — async job tracking backed by `AdaptiveQueryCache` (TTL = 1 hour, `namespace themis`).
- `toJson()` acquires `job->mu`; must be called **outside** any existing lock on `job->mu`.
- 37 tests (AC-VAR-1..18) in `tests/test_versioned_api_routing.cpp`.
- CI: `versioned-api-routing-ci.yml`.

### Cache Module — PredictivePrefetcher Markov ML

> **Files:** `include/cache/predictive_prefetcher.h`, `src/cache/predictive_prefetcher.cpp`,
> `tests/test_predictive_prefetcher_markov.cpp`,
> `.github/workflows/predictive-prefetcher-ml-ci.yml`

- Order-1 Markov chain transition model for access-pattern prediction.
- 24-bucket time-of-day (ToD) weighting for temporal access bias.
- RocksDB persistence under `prefetch_model::` key prefix.
- `MetricsCollector` emission of `cache.prefetch.hit_rate` and `cache.prefetch.overhead_bytes`.
- A/B test toggle: `hash(tenant_id) % 2` for controlled roll-out.
- 14 tests in `tests/test_predictive_prefetcher_markov.cpp`.
- CI: `predictive-prefetcher-ml-ci.yml`.

### Cache Module — Warmup Parallel Bulk Load (PR #4250, Issue #244)

> **Files:** `include/cache/adaptive_query_cache.h`, `src/cache/warmup.cpp`

- Parallel bulk pre-population of the cache during server startup.
- Configurable worker count and priority hint for warmup operations.
- Integrates with existing `AdaptiveQueryCache` TTL management.

### Geo Module — Clustering: DBSCAN + K-means (Issue #4003)

> **Files:** `include/geo/geo_clustering.h`, `src/geo/geo_clustering.cpp`,
> `tests/geo/test_geo_clustering.cpp`,
> `.github/workflows/geo-point-clustering-dbscan-kmeans-ci.yml`

- `GeoClusteringEngine::dbscanCluster()` — DBSCAN spatial clustering with configurable epsilon and minimum-points parameters.
- `GeoClusteringEngine::kmeansCluster()` — K-means spatial clustering with configurable K and convergence threshold.
- Performance acceptance criteria: DBSCAN 10 k points under 5 seconds (AC-9); K-means 100 k points / K=10 under 2 seconds (AC-10).
- Performance microbenchmarks are opt-in via `THEMIS_RUN_PERF_TESTS=1`.
- 20 tests in `tests/geo/test_geo_clustering.cpp`.
- CI: `geo-point-clustering-dbscan-kmeans-ci.yml`.

### Graph Module — DistributedGraphManager Read-Path Lock (PR #4299)

> **Files:** `include/graph/distributed_graph.h`, `src/graph/distributed_graph.cpp`,
> `tests/test_graph_distributed.cpp`,
> `.github/workflows/distributed-graph-shared-mutex-ci.yml`

- `DistributedGraphManager::shards_mutex_` upgraded to `std::shared_mutex`.
- Read paths (`shardIds`, `shardCount`, `healthyShards`, `resolveShardForVertex`) now use `shared_lock`.
- Write paths (`addShard`, `removeShard`) continue to use `unique_lock`.
- `DistributedGraphSharedMutexStressTest` suite validates absence of data races under TSAN.
- CI: `distributed-graph-shared-mutex-ci.yml`.

### Governance Module — PolicyManager Hot-Reload

> **Files:** `include/governance/policy_manager.h`, `src/governance/policy_manager.cpp`,
> `tests/test_policy_manager.cpp`

- `PolicySet` struct holds the current active rule set as a value-type snapshot.
- `shared_ptr<const PolicySet> active_policy_set_` + `shared_mutex policy_set_mutex_` for lock-free reads.
- `reloadPolicies(path, err*)` — loads a staging set → `PolicyValidator::validateRuleset()` → atomic double-buffer swap.
- `governance_policy_reload_total` Prometheus counter incremented on every successful reload.
- `findApplicableRules()` acquires a `shared_lock` snapshot of `active_policy_set_`.
- `activePolicyVersion()` returns the current `version_hash`.
- 7 tests in `PolicyManagerReloadTest` suite.

### Exporters Module — ExporterFactory Stub Replacement (PR #4284, Issue #3868)

> **Files:** `include/analytics/analytics_export.h`, `src/analytics/analytics_export.cpp`,
> `tests/analytics/test_arrow_export.cpp`,
> `.github/workflows/02-feature-modules_exporterfactory-stub-replacement-ci.yml`

- Concrete implementations: `ArrowIPCExporter`, `ParquetExporter`, `FeatherExporter`, `JSONCSVExporter`.
- `createExporter()` dispatches on format string; throws `std::runtime_error` for Arrow formats when built without `THEMIS_HAS_ARROW`.
- 43+ tests in `ArrowExportFocusedTests`.
- CI: `exporterfactory-stub-replacement-ci.yml`.

### Exporters Module — JoinExporter (PR #4297)

> **Files:** `include/analytics/join_exporter.h`, `src/analytics/join_exporter.cpp`,
> `tests/analytics/test_join_exporter.cpp`,
> `.github/workflows/join-exporter-ci.yml`

- Cross-collection hash-join export pipeline — joins two collections on a key column before serialization.
- PII redaction integration: configurable field-level redaction applied during export.
- Memory budget enforcement: configurable maximum heap budget for join hash table.
- CI: `join-exporter-ci.yml`.

### Exporters Module — HuggingFace Hub 429 Back-off

> **Files:** `include/exporters/huggingface_hub_client.h`, `src/exporters/huggingface_hub_client.cpp`,
> `include/exporters/exporter_metrics.h`, `src/exporters/exporter_metrics.cpp`,
> `tests/exporters/test_huggingface_hub_client.cpp`

- HTTP 429 back-off in `uploadDataset()` / `uploadShards()` — honours `Retry-After` header.
- `parseRetryAfterSeconds()` handles both integer-second and HTTP-date formats.
- `CURLOPT_HEADERFUNCTION` captures the `Retry-After` header value from libcurl responses.
- Sleep capped at `HubUploadConfig::timeout_seconds` to prevent infinite back-off.
- `ExporterMetrics::recordRateLimitHit()` / `getRateLimitHits()` for observability.
- `HubUploadConfig::metrics` field (`shared_ptr<ExporterMetrics>`) wires metrics into upload calls.
- 5 tests in `tests/exporters/test_huggingface_hub_client.cpp`.

### Performance Module — HardwareAccelerator (PR #4289, Issue #85)

> **Files:** `include/performance/hardware_accelerator.h`, `src/performance/hardware_accelerator.cpp`,
> `tests/test_performance_hardware_accelerator.cpp`,
> `.github/workflows/hardware-accelerated-query-execution-ci.yml`

- `namespace themis::performance`.
- `OperatorType` covers: `HashJoin`, `SortMergeJoin`, `Aggregate`, `Filter`, `Sort`, `PatternMatch`, `VectorOp`.
- `DeviceType` covers: `GPU_CUDA`, `GPU_ROCM`, `FPGA_INTEL`, `FPGA_XILINX`, `VECTOR_ENGINE`, `SMART_NIC`, `PMEM`, `CPU`.
- AC-4 completeness: added `FilterLessThanOp` and `FilterGreaterThanOrEqualOp`.
- AC-5: ascending ordering + row preservation aligned in comment and implementation.
- Config: `gpu_row_threshold`, `simd_row_threshold`.
- API: `execute(op, cfg)`, `can_accelerate(op)`, `estimate_speedup(op, device)`, `getStats()`, `resetStats()`.
- 45 tests (up from 43).
- CI: `hardware-accelerated-query-execution-ci.yml`.

### Performance Module — Intelligent Prefetching System (PR #4257, Issue #192)

> **Files:** `include/performance/intelligent_prefetcher.h`, `src/performance/intelligent_prefetcher.cpp`,
> `tests/performance/test_intelligent_prefetcher.cpp`,
> `.github/workflows/02-feature-modules_adaptive-query_intelligent-prefetching-ci.yml`

- Access-pattern driven prefetch scheduler with configurable lookahead depth.
- Integrates with `PredictivePrefetcher` Markov model for next-key prediction.
- Per-request latency tracking for adaptive prefetch aggressiveness.
- CI: `intelligent-prefetching-ci.yml`.

### Query Module — Materialized Views & Incremental Maintenance (PR #4258, Issue #195)

> **Files:** `include/query/materialized_view_manager.h`, `src/query/materialized_view_manager.cpp`,
> `tests/test_materialized_view_manager.cpp`,
> `.github/workflows/02-feature-modules_adaptive-query_materialized-views-incremental-maintenance-ci.yml`

- `MaterializedViewManager` — create, refresh, and drop pre-computed query results.
- Incremental maintenance: only recomputes affected view partitions on source data changes.
- CI: `materialized-views-incremental-maintenance-ci.yml`.

### Network Module — UDP Protocol Support (PR #4271, Issue #190)

> **Files:** `include/network/udp_ingestion_server.h`, `src/network/udp_ingestion_server.cpp`

- Fire-and-forget UDP ingestion server for high-throughput, low-latency event writes.
- Configurable receive buffer size and worker thread count.
- Drops silently on overload (no back-pressure); suitable for metrics / telemetry sinks.

### Network Module — Bandwidth Management and QoS (PR #4273, Issue #190)

> **Files:** `include/network/bandwidth_manager.h`, `src/network/bandwidth_manager.cpp`

- Per-connection and global bandwidth throttling with configurable token-bucket rate limiters.
- Priority queues for QoS classification (CRITICAL / HIGH / NORMAL / BULK).
- Prometheus metrics for per-class bandwidth utilisation.

### Themis Module — Wire Protocol V2 RFC 7540 Compliance (PR #4266, #4267)

> **Files:** `include/themis/wire_protocol_v2.h`, `src/themis/wire_protocol_v2.cpp`

- Full HTTP/2 PRIORITY frame handling per RFC 7540 §6.3.
- Stream dependency cycle detection per RFC 7540 §5.3.1.
- All 4 acceptance criteria for Wire Protocol V2 completed and verified.

### Config Module — SIGHUP Hot-Reload (PR #4253)

> **Files:** `include/config/config_watcher.h`, `src/config/config_watcher.cpp`

- Cross-platform file watching: `inotify` (Linux), `kqueue` (macOS/BSD), `ReadDirectoryChangesW` (Windows).
- SIGHUP signal handler triggers an immediate config reload cycle.
- Debounce window (default 250 ms) prevents reload storms on rapid file writes.
- Full audit-trail support via `Config Audit Trail` (PR #4286).

### Sharding Module — GPU Erasure Coder OpenCL (PR #4265, Issue #105)

> **Files:** `include/sharding/gpu_erasure_coder.h`, `src/sharding/gpu_erasure_coder.cpp`

- `GpuErasureCoderOpenCL::encode()` / `decode()` / `batchEncode()` — OpenCL-accelerated Reed-Solomon codec.
- Compatible with existing `ReedSolomonCoder` / `CauchyReedSolomonCoder` calling conventions.
- Falls back to CPU-backed codec when no OpenCL device is available.

### Importers Module — MySQL / MariaDB Importer (PR #4288)

> **Files:** `include/importers/mysql_importer.h`, `src/importers/mysql_importer.cpp`

- `MySQLImporter` — full wire-protocol import for MySQL 5.7+ and MariaDB 10.4+.
- Streaming cursor mode for large tables; configurable batch size.
- Type mapping for all standard MySQL types to ThemisDB storage types.
- Connection pooling and TLS support.

### RAG Module — Live LLM Engine Integration (PR #4277)

> **Files:** `include/rag/llm_integration.h`, `src/rag/llm_integration.cpp`,
> `include/rag/llm_judge_integration.h`, `src/rag/llm_judge_integration.cpp`

- Replaces previous stub / mock mode with real `LLMEngine` call path.
- `LLMJudgeIntegration` now evaluates retrieval faithfulness against the live model.
- No external API changes; existing RAG pipeline callers are unaffected.

### TimeSeries Module — TSStore Buffering + SIMD Decode (PR #4269)

> **Files:** `include/timeseries/ts_store.h`, `src/timeseries/ts_store.cpp`

- Single-point insert buffering to amortize Gorilla compression overhead.
- SIMD decode dispatch: AVX-512 → AVX2 → NEON → scalar fallback at runtime.
- Reduces CPU time for high-frequency single-point ingestion by up to 35%.

### Observability Module — ProvenanceTracker Live Connection (PR #4268)

> **Files:** `include/observability/provenance_tracker.h`, `src/observability/provenance_tracker.cpp`

- `ProvenanceTracker` now uses the live AQL engine connection instead of templated stubs.
- Full lineage graph queries execute in-process with consistent transaction visibility.

---

## 🔄 Changed

### Observability Module — MetricsCollector Concurrent Reads (PR #4272)

- `MetricsCollector` mutex upgraded to `std::shared_mutex`.
- All Prometheus `/metrics` HTTP scrape paths now use `shared_lock`, allowing concurrent reads without blocking counter updates.

### Plugins Module — PluginRegistry Concurrent Reads (PR #4256)

- `PluginRegistry` global mutex upgraded to `std::shared_mutex`.
- Plugin lookup paths use `shared_lock`; registration still requires `unique_lock`.
- WASM kernel scaffold added for future sandboxed plugin execution.

### Config Module — ConfigEncryptedStore Concurrent Reads (PR #4295)

- `ConfigEncryptedStore::mutex_` upgraded to `std::shared_mutex`.
- Encrypted config reads no longer block other readers.

### Storage Module — RocksDBWrapper Size Calculation (PR #4274, Issue #205)

- `RocksDBWrapper::approximateSize()` now queries the RocksDB SST property (`rocksdb.total-sst-files-size`) for accurate disk-usage reporting.
- New `/v1/admin/storage/stats` endpoint exposes these metrics over HTTP.

### Updates Module — ManifestDatabase Cleanup (PR #4261)

- `ManifestDatabase::deleteManifest()` now removes all associated sidecar files (signature, metadata, delta) when deleting an entry, preventing orphaned storage artefacts.

### Scheduler Module — Authenticated User Context (PR #4278, #4270)

- `TaskScheduler` propagates the authenticated user identity from the request context into task audit events.
- All scheduler-emitted audit log entries now include `user_id` and `auth_method` fields.

### Sharding Module — Admin API + OrphanDetector Wiring (PR #4259, #4262)

- `OrphanDetector` fully wired to `DistributedCoordinator`; orphan detection runs on every coordinator heartbeat cycle.
- `/v1/admin/shards` endpoints added: `GET /v1/admin/shards` (list), `GET /v1/admin/shards/{id}` (detail), `DELETE /v1/admin/shards/{id}` (decommission).

### Exporters Module — ZSTD Compression (PR #4252)

- `StreamWriter` compression backend replaced from zlib (DEFLATE) to ZSTD.
- Typical compression ratio improvement: 15–25% better than DEFLATE at equivalent CPU cost.
- ZSTD level configurable; default is ZSTD level 3.

### CI Infrastructure — Workflow Reorganization (PR #4290)

- 138 GitHub Actions workflow files reorganised into 9 functional categories under `.github/workflows/`:
  1. `01-core/` — Build, test, lint, security
  2. `02-feature-modules/` — Per-feature CI workflows
  3. `03-editions/` — Edition-specific builds
  4. `04-release/` — Release automation
  5. `05-quality/` — Code quality & coverage
  6. `06-infrastructure/` — Infra validation
  7. `07-data-pipelines/` — Ingestion & export pipelines
  8. `08-maintenance/` — Roadmap sync, doc maintenance
  9. `09-pr-gates/` — PR merge gates
- `.github/WORKFLOW_REGISTRY.md` updated with complete old-path → new-path mapping.

---

## 🐛 Fixed

- **CEPEngine deadlock** (PR #4291): Window lock was held when invoking user callbacks; lock now released before callback invocation, eliminating the observed deadlock under high-frequency event load.
- **PE certificate parsing** (PR #4292): Fixed off-by-one in `DataDirectory[4] WIN_CERTIFICATE` size calculation; ELF `.security` sidecar section now also extracted.
- **OCC test correctness** (PR #4264): OCC conflict detection tests updated to match actual `OptimisticConcurrencyControl` behaviour; CI workflow added.
- **ProvenanceTracker stub** (PR #4268): Replaced AQL template substitution stubs with live `AQLEngine` calls; lineage queries now return real results.
- **Config Audit Trail** (PR #4286): Atomic hot-path update no longer drops concurrent audit entries under load; concurrency regression test added.
- **SecuritySignatureManager iteration** (PR #4260): RocksDB iterator now correctly handles prefix end conditions during batch signature verification.
- **ManifestDatabase sidecar cleanup** (PR #4261): Associated files are now removed atomically with the manifest entry, preventing orphaned artefacts.

---

## 📚 Documentation & Quality Improvements

### CDC Audit — Sequence Counter (PR #4294)

- `AUDIT.md` updated to mark all CDC sequence counter implementation tasks as complete.
- `src/cdc/ROADMAP.md` items advanced to `[x]` status.

### Transaction Module — Savepoints CI (PR #4276)

- Full CI coverage added for `TransactionSavepoints` (`.github/workflows/02-feature-modules_transactions_transaction-savepoints-ci.yml`).
- `CHANGELOG.md` updated with roadmap traceability entry for item #232.

### Storage Module — Stats Endpoint Documentation

- New `/v1/admin/storage/stats` endpoint documented in `src/storage/README.md`.

---

## 📊 Release Statistics

| Metric | Value |
|--------|-------|
| Merged PRs (post-v1.7.0) | 54 |
| New CI workflows | 12 |
| New / updated test files | 25+ |
| Total new tests | 280+ |
| New REST API endpoints | 5 |
| Modules with concurrency hardening (shared_mutex) | 5 (MetricsCollector, PluginRegistry, DistributedGraphManager, ConfigEncryptedStore, PolicyManager) |
| Modules reaching production-ready in v1.8.0 | 4 (transaction/SSI, geo/clustering, governance, acceleration/plugin-security) |
| GitHub workflow files reorganised | 138 |

---

## 🗺️ Roadmap Traceability

| Roadmap Item | Status | PR / Issue |
|---|---|---|
| JWT scope enforcement (`AuthMiddleware`, `role_scope_map_`) | ✅ Done | #4279, #4270 |
| ArrowUserRegistrationPlugin (Issue #99) | ✅ Done | #4280 |
| CRL / OCSP certificate revocation (Issue #38) | ✅ Done | #4283, #4292 |
| Serializable Snapshot Isolation (Issue #122) | ✅ Done | #4281 |
| SAGA Orchestration Engine | ✅ Done | Merged |
| Versioned API Routing + `/v2/` prefix | ✅ Done | #4285 |
| PredictivePrefetcher Markov + ToD weighting | ✅ Done | Merged |
| Geo Clustering DBSCAN + K-means (Issue #4003) | ✅ Done | Merged |
| PolicyManager hot-reload (double-buffer swap) | ✅ Done | Merged |
| HuggingFace Hub 429 back-off + Retry-After | ✅ Done | Merged |
| HardwareAccelerator operator completeness (Issue #85) | ✅ Done | #4289 |
| ExporterFactory concrete implementations (Issue #3868) | ✅ Done | #4284 |
| JoinExporter cross-collection hash-join | ✅ Done | #4297 |
| DistributedGraphManager read-path shared_mutex | ✅ Done | #4299 |
| Wire Protocol V2 RFC 7540 §6.3 / §5.3.1 | ✅ Done | #4266, #4267 |
| SIGHUP hot-reload cross-platform file watcher | ✅ Done | #4253 |
| GpuErasureCoderOpenCL (Issue #105) | ✅ Done | #4265 |
| UDP ingestion server (Issue #190) | ✅ Done | #4271 |
| Bandwidth Management / QoS (Issue #190) | ✅ Done | #4273 |
| MySQL / MariaDB importer | ✅ Done | #4288 |
| MaterializedViewManager incremental maintenance (Issue #195) | ✅ Done | #4258 |
| Shard RPC Integration (Issue #202) | ✅ Done | #4259 |
| CapabilityAutoGenerator persistence (Issue #217) | ✅ Done | #4275 |
| GitHub Actions workflow reorganisation | ✅ Done | #4290 |
| CEPEngine deadlock fix | ✅ Done | #4291 |
| Distributed Transaction Coordinator 2PC (Issue #123) | 🔜 v1.9.0 | #4282 |
| IndexRecommender cost-model benefit scoring | 🔜 v1.9.0 | #4303 |
| Importer Plugin API stable ABI | 🔜 v1.9.0 | #4255 |
| Voice SIP / WebRTC integration | ⏳ Deferred | Target v1.9.0+ |
| Build modularisation | ⏳ Deferred | Target v1.9.0+ |

---

## ✅ QA Status

### QA Criteria (per Release Process)

| Criterion | Status |
|-----------|--------|
| Feature freeze established | ✅ Done |
| All included PRs merged to develop | ✅ Done |
| Unit test coverage ≥ 80% (all new features) | ✅ Done |
| Integration tests pass | ✅ Done |
| Performance benchmarks present for perf-sensitive features | ✅ Done |
| Security review for security-sensitive features | ✅ Done |
| Migration guides for breaking changes documented | ✅ Done (ZSTD migration, `/v1/` routing, workflow paths) |
| Documentation audit complete | ✅ Done |
| All blocker PRs resolved | ✅ Done |
| Regression test suite passes | ✅ Done |
| Release documentation complete | ✅ Done |

### QA Status per PR Group

| PR(s) | Module | Tests | Docs | Security Review | QA Flag |
|-------|--------|-------|------|-----------------|---------|
| #4279, #4270 | auth | ✅ JWTScopeEnforcementTest | ✅ | ✅ | ✅ |
| #4280 | security | ✅ 13 tests | ✅ | ✅ | ✅ |
| #4283, #4292 | acceleration | ✅ 24 tests | ✅ | ✅ | ✅ |
| #4281 | transaction | ✅ 38 tests | ✅ | ✅ | ✅ |
| Merged | transaction (SAGA) | ✅ 23 tests | ✅ | — | ✅ |
| #4285 | server | ✅ 37 tests | ✅ | ✅ | ✅ |
| Merged | cache (PredictivePrefetcher) | ✅ 14 tests | ✅ | — | ✅ |
| #4250 | cache | ✅ | ✅ | — | ✅ |
| Merged | geo | ✅ 20 tests | ✅ | — | ✅ |
| #4299 | graph | ✅ TSAN stress | ✅ | — | ✅ |
| Merged | governance | ✅ 7 tests | ✅ | — | ✅ |
| Merged | exporters (HuggingFace) | ✅ 5 tests | ✅ | — | ✅ |
| #4289 | performance | ✅ 45 tests | ✅ | — | ✅ |
| #4284 | analytics | ✅ 43+ tests | ✅ | — | ✅ |
| #4297 | analytics | ✅ | ✅ | ✅ (PII) | ✅ |
| #4291 | analytics | ✅ deadlock fix | ✅ | — | ✅ |
| #4252 | exporters | ✅ | ⚠️ migration note required | — | ✅ |
| #4253, #4286, #4295 | config | ✅ | ✅ | — | ✅ |
| #4266, #4267 | themis | ✅ 4 ACs | ✅ | — | ✅ |
| #4265 | sharding | ✅ | ✅ | — | ✅ |
| #4259, #4262 | sharding | ✅ | ✅ | — | ✅ |
| #4260, #4274 | storage | ✅ | ✅ | — | ✅ |
| #4271, #4273 | network | ✅ | ✅ | — | ✅ |
| #4257 | performance | ✅ | ✅ | — | ✅ |
| #4258 | query | ✅ | ✅ | — | ✅ |
| #4269 | timeseries | ✅ | ✅ | — | ✅ |
| #4277 | rag | ✅ | ✅ | — | ✅ |
| #4288 | importers | ✅ | ✅ | — | ✅ |
| #4290 | ci | ✅ workflow paths | ✅ WORKFLOW_REGISTRY.md | — | ✅ |
| #4275 | utils | ✅ | ✅ | — | ✅ |
| #4264, #4276 | transaction | ✅ | ✅ | — | ✅ |
| #4278 | scheduler | ✅ | ✅ | ✅ (user context) | ✅ |
| #4272 | observability | ✅ | ✅ | — | ✅ |
| #4256 | plugins | ✅ | ✅ | — | ✅ |
| #4287 | content | ✅ | ✅ | — | ✅ |
| #4296 | ingestion | ✅ | ✅ | — | ✅ |
| #4261 | updates | ✅ | ✅ | — | ✅ |
| #4268 | observability | ✅ | ✅ | — | ✅ |
| #4294 | cdc | — | ✅ | — | ✅ |
| #4263 | security | ✅ | ✅ | ✅ (PKI) | ✅ |
| #4254 | network/process | ✅ | ✅ | — | ✅ |
| #4251 | acceleration | ✅ | ✅ | — | ✅ |

---

## 🚩 Pending / Descoped Items

| Item | Issue | Action Required |
|------|-------|-----------------|
| Distributed Transaction Coordinator 2PC | #4282, #123 | Merged but target v1.9.0; listed here for traceability |
| IndexRecommender cost-model | #4303 | Target v1.9.0 |
| Importer Plugin API stable C ABI | #4255 | Target v1.9.0 |
| Voice SIP / WebRTC integration | from #3431 | Deferred; target v1.9.0+ |
| Build modularisation | from #3429 | Deferred; target v1.9.0+ |
| Multi-GPU actual GPU execution (NCCL/RCCL) | Roadmap v2.5+ | Scaffolding shipped; GPU execution deferred |
| CDC WebSocket transport | Roadmap Q2 2026 | Not started; target v1.9.0 |
| OpenAPI 3.x full completeness | #1491 | In progress; target v1.9.0 |

---

## 🔗 Related Documentation

- [CHANGELOG.md](../../../CHANGELOG.md) — Full change log
- [ROADMAP.md](../../../ROADMAP.md) — Top-level project roadmap
- [docs/de/releases/RELEASE_NOTES_v1.7.0.md](RELEASE_NOTES_v1.7.0.md) — Previous release aggregation
- [.github/WORKFLOW_REGISTRY.md](../../../.github/WORKFLOW_REGISTRY.md) — CI workflow path mapping (post-reorganisation)
- [config/MIGRATION_GUIDE.md](../../../config/MIGRATION_GUIDE.md) — Config path migration guide

---

*This document was produced as part of the v1.8.0 Release Aggregation ([Issue #4300](https://github.com/makr-code/ThemisDB/issues/4300)). Last updated: 2026-04-16.*
