# ThemisDB v1.7.0 — Release Aggregation

**Release Date:** 2026-03-09
**Type:** Feature Release
**Previous Version:** v1.5.0
**Milestone:** v1.7.0
**Parent Issue:** [makr-code/ThemisDB#3073](https://github.com/makr-code/ThemisDB/issues/3073)
**Aggregation Issue:** [makr-code/ThemisDB#3486](https://github.com/makr-code/ThemisDB/issues/3486)

---

## 🎯 Overview

ThemisDB v1.7.0 is a feature and hardening release that delivers hierarchical config organisation, multi-GPU vector indexing scaffolding, Git-like 3-way merge and PITR integration, improved distributed query cost modelling, FAISS ADC search acceleration, the CHIMERA benchmark suite rebranding, API versioning infrastructure, query result pagination, plugin metrics, and a comprehensive documentation and test-coverage overhaul across all 44 source modules.

---

## ⚠️ Breaking Changes

| # | Module | Change | Migration |
|---|--------|--------|-----------|
| 1 | **themis** | Core module initialisation code migrated from `src/utils/` and `src/base/` to `src/themis/` | Update `#include` paths from `utils/themis_*.h` / `base/themis_*.h` to `themis/themis_*.h`; all symbols remain API-compatible |

---

## 📦 Included Pull Requests

### Post-v1.5.0 Merged PRs

| PR # | Module | Purpose |
|------|--------|---------|
| [#3471](https://github.com/makr-code/ThemisDB/pull/3471) | **tests / benchmarks** | Complete coverage audit — 6 new benchmark suites + 21 new unit test files across 3 waves; closes all addressable coverage gaps in the 44-module tree |
| [#3472](https://github.com/makr-code/ThemisDB/pull/3472) | **cdc** | `src/cdc/README.md` and `ROADMAP.md` aligned with actual source files; removes stale non-existent file references, adds implemented components, marks outbox pattern and WebSocket transport as complete |
| [#3473](https://github.com/makr-code/ThemisDB/pull/3473) | **cache** | `src/cache/` docs synced with Phases 3 & 4 deliverables; features previously marked "planned" or "in design" updated to production-ready status |
| [#3474](https://github.com/makr-code/ThemisDB/pull/3474) | **auth** | `src/auth/README.md` corrected: stale file references removed, production-ready components added, Kerberos/TOTP implementation status aligned |
| [#3475](https://github.com/makr-code/ThemisDB/pull/3475) | **base** | `src/base/` docs updated: 6 undocumented production-ready components added, hot-reload status corrected, stale interface references replaced |
| [#3476](https://github.com/makr-code/ThemisDB/pull/3476) | **acceleration** | `src/acceleration/` docs audited: README directory layout now lists all ~30 source files, ARCHITECTURE corrected for GPU backend selection flow, ROADMAP status updated |
| [#3477](https://github.com/makr-code/ThemisDB/pull/3477) | **docs** | Broad documentation audit aligning multiple module docs with source code implementation |
| [#3478](https://github.com/makr-code/ThemisDB/pull/3478) | **analytics** | `src/analytics/README.md` and `ARCHITECTURE.md` corrected: `window_function.cpp` → `streaming_window.cpp`, `process_miner.cpp` added, 5 undocumented production-ready components added |
| [#3479](https://github.com/makr-code/ThemisDB/pull/3479) | **aql** | `src/aql/README.md` and `ROADMAP.md` synced with actual implementation: 12 implemented components added, non-existent file references replaced, stale in-progress/planned status corrected |
| [#3480](https://github.com/makr-code/ThemisDB/pull/3480) | **ci** | New `.github/workflows/documentation-validation.yml` workflow implementing the CI contract described in `docs/DOCUMENTATION_VALIDATION.md`; 5 jobs: link-check, markdown-lint, spell-check, structure-check, summary |
| [#3481](https://github.com/makr-code/ThemisDB/pull/3481) | **aql** | AQL grammar and parser documentation synchronised with parser implementation (`aql/AQL_GRAMMAR.ebnf` updated to reflect actual parser, unimplemented features explicitly marked) |
| [#3482](https://github.com/makr-code/ThemisDB/pull/3482) | **ci / docs** | Documentation-validation CI workflow aligned with actual docs; broken badge links (`ci.yml`, `code-quality.yml`, `arm-build.yml`) corrected across the documentation tree |
| [#3483](https://github.com/makr-code/ThemisDB/pull/3483) | **docs / storage** | MVCC documentation consolidated: `ARCHITECTURE.md`, compendium, and tuning guide unified; `MVCCStore` / HLC coverage added, stale planning sections removed, broken ToC fixed |
| [#3484](https://github.com/makr-code/ThemisDB/pull/3484) | **docs** | 119 broken links corrected across `DOCUMENTATION_HUB.md`, `CATEGORY_INDEX.md`, and `00_DOCUMENTATION_INDEX.md`; `DOCS_ORGANIZATION_PLAN.md` brought up to date (387 root docs / 59 subdirs) |
| [#3485](https://github.com/makr-code/ThemisDB/pull/3485) | **rag / research** | `docs/en/rag/RAG_SCIENTIFIC_FOUNDATIONS.md` (460 lines): IEEE-formatted scientific reference document consolidating 40 peer-reviewed citations for the RAG subsystem |

### Feature PRs (from CHANGELOG [Unreleased] scope)

These features were developed after v1.5.0 and are included in the v1.7.0 release scope.

| Feature | Module | Status |
|---------|--------|--------|
| Config Architecture Reorganization | **config** | ✅ Merged |
| Multi-GPU Vector Indexing API (v2.4 scaffolding) | **gpu / index** | ✅ Merged |
| Git-Like Features: 3-Way Merge, PITR API, SnapshotManager | **storage / server** | ✅ Merged |
| HybridSearch production hardening | **search** | ✅ Merged |
| Distributed Query Optimizer (v1.5.x) | **query** | ✅ Merged |
| FAISS ADC distance table acceleration | **index** | ✅ Merged |
| README: Technology & Feature Badges | **docs** | ✅ Merged |
| CHIMERA Suite Branding | **benchmarks** | ✅ Merged |
| API Versioning and Compatibility Strategy | **server / api** | ✅ Merged |
| Query Result Pagination | **query / server** | ✅ Merged |
| Plugin Metrics and Monitoring | **plugins** | ✅ Merged |
| Documentation Archival System | **docs** | ✅ Merged |
| Retroactive Release Building System | **ci / docs** | ✅ Merged |
| Schema Manager | **storage** | ✅ Merged |
| Independent Health / Error Service | **server** | ✅ Merged |
| Root Cause Analyzer | **observability** | ✅ Merged |

---

## ✨ New Features

### Config Module — Hierarchical Architecture Reorganization

> **Files:** `config/` (all subdirectories), `src/server/http_server.cpp`, `src/server/mcp_server.cpp`, `src/utils/pii_detector.cpp`, `src/main_server.cpp`, `src/index/vector_index.cpp`, `src/content/mime_detector.cpp`, `config/README.md`, `config/MIGRATION_GUIDE.md`

- All configuration files reorganised into logical category subdirectories:

  | Directory | Contents |
  |-----------|----------|
  | `config/core/` | `config.yaml`, `security.yaml`, `updates.yaml` |
  | `config/platform/` | Raspberry Pi (rpi3/4/5), QNAP platform overrides |
  | `config/ai_ml/` | LLM, vision, LoRA, RAG configurations |
  | `config/security/` | RBAC, PII, Kerberos authentication |
  | `config/compliance/` | Ethical guidelines, audit, governance |
  | `config/performance/` | Scaling, query cache, hardware acceleration |
  | `config/data_management/` | Retention, redundancy, MIME types |
  | `config/distributed/` | Replication, sharding |
  | `config/licensing/` | Community, enterprise license files |
  | `config/networking/` | Connection pooling |
  | `config/content/` | Stream processors, edge types |
  | `config/monitoring/` | Prometheus metrics |
  | `config/features/` | Feature flags, capability generation |
  | `config/assistants/` | Documentation/feedback assistant configs |
  | `config/processing/` | CEP rules, stream/event processing |
  | `config/deprecated/` | Legacy backup files |

- **`ConfigPathResolver`**: automatic backward-compatibility layer resolving legacy paths to new locations with deprecation warnings; zero breaking changes to existing C++ code.
- All C++ code paths updated (`http_server.cpp`, `mcp_server.cpp`, `pii_detector.cpp`, `main_server.cpp`, `vector_index.cpp`, `mime_detector.cpp`).
- Full path mapping table (60+ config files) documented in `config/MIGRATION_GUIDE.md`.

### GPU / Index Modules — Multi-GPU Vector Indexing API (v2.4)

> **Files:** `include/index/multi_gpu_vector_index.h`, `src/index/multi_gpu_vector_index.cpp`, `tests/test_multi_gpu_vector_index.cpp`, `examples/multi_gpu_vector_example.cpp`, `docs/MULTI_GPU_VECTOR_INDEXING.md`

- **`MultiGPUVectorIndex`**: logical multi-device API with support for 2–8 devices via configurable partition strategies: round-robin, hash-based, range-based, balanced.
- Query fan-out with centralised top-k merge across all partitions.
- Fault-tolerant design: graceful degradation when partitions are unavailable.
- Per-partition statistics with hooks for future per-GPU metrics (VRAM, utilisation).
- `enableMultiGPU`, `deviceIds`, `partitionStrategy` configuration options.
- **Current execution**: CPU-based via `GPUVectorIndex` backend; actual multi-GPU execution (NCCL/RCCL, P2P transfers) planned for v2.5+.
- 394-line test suite validating partitioning/merge logic and API correctness.

### Storage / Server Modules — Git-Like Features Integration

> **Files:** `include/storage/snapshot_manager.h`, `include/storage/diff_engine.h`, `include/storage/merge_engine.h`, `src/storage/snapshot_manager.cpp`, `src/server/http_server.cpp`

**SnapshotManager (re-enabled):**
- Named snapshots for MVCC fully operational.
- 5 REST endpoints for snapshot/tag management.
- Integration with `DiffEngine` for tag-based diffs.
- Persistent snapshot storage in RocksDB.

**PITR API Handler:**

| Endpoint | Description |
|----------|-------------|
| `POST /api/v1/pitr/restore/sequence` | Restore to specific sequence number |
| `POST /api/v1/pitr/restore/tag` | Restore to named snapshot tag |
| `POST /api/v1/pitr/restore/timestamp` | Restore to timestamp |
| `POST /api/v1/pitr/preview` | Dry-run preview of restore operation |
| `GET /api/v1/pitr/progress` | Current restore progress |

**MergeEngine API:**

| Endpoint | Description |
|----------|-------------|
| `POST /api/v1/merge` | Perform three-way merge between sequences |
| `POST /api/v1/merge/preview` | Preview merge without applying (dry-run) |
| `POST /api/v1/merge/by-tag` | Merge using snapshot tags |
| `GET /api/v1/merge/can-fast-forward` | Check fast-forward merge possibility |

- `BranchManager` enhanced: non-fast-forward merges, automatic conflict detection with OURS / THEIRS / MANUAL / FAST_FORWARD resolution strategies.

### Search Module — HybridSearch Production Hardening

> **Files:** `include/search/hybrid_search.h`, `src/search/hybrid_search.cpp`, `tests/test_hybrid_search.cpp`, `tests/test_rrf_fusion.cpp`, `tests/test_score_normalization.cpp`, `tests/test_hybrid_search_integration.cpp`, `benchmarks/benchmark_hybrid_search.cpp`

- **Configurable vector metric**: `Config::vector_metric` (COSINE / DOT / L2) — was hardcoded to COSINE.
- **Strict config validation**: constructor throws `std::invalid_argument` on invalid `k`, `rrf_k`, weights, `max_k`, `max_candidates`, empty table/column.
- **Resource limits**: `Config::max_k` and `Config::max_candidates` bound unbounded index scans (default: 10,000 each).
- **Score normalization edge cases**: range == 0 now yields 1.0 for positive, 0.0 for zero scores.
- **Linear-combination pre-normalization**: BM25 and vector scores normalized to [0, 1] before weighting.
- **`SearchStats`**: appended to every `search()` return; exposes `bm25_ok`, `vector_ok`, `partial_result`, `bm25_count`, `vector_count`.
- **Exception safety**: `search()` catches all backend and fusion exceptions, logs via `THEMIS_ERROR`, returns empty/partial results.
- Tests: 35+ unit tests, 20 RRF fusion tests, 15 score normalization tests, 18 integration tests.

### Query Module — Distributed Query Optimizer (v1.5.x)

> **Files:** `include/query/distributed_query_cost_model.h`, `src/query/distributed_query_cost_model.cpp`, `tests/test_optimizer_v1_5_x_integration.cpp`

- `getShardRowCount()`: dynamic row-count estimates replacing hardcoded 10K constant; foundation for accurate cardinality estimation.
- `calculatePredicateSelectivity()`: histogram-based selectivity with column-specific heuristics (ID 0.1%, status 20%, names 5%); combined predicates use product of individual selectivities; bounded to [0.01%, 100%].
- `measureShardLatency()`: latency integration hook enabling locality detection (< 1 ms threshold) and network-aware parallelism optimisation.

### Index Module — FAISS ADC Distance Table Acceleration

> **Files:** `include/index/advanced_vector_index.h`, `src/index/advanced_vector_index.cpp`

- **ADC (Asymmetric Distance Computation) Tables**: ~40% faster vector search via precomputed distance tables for `IndexIVFPQ`.
- `use_adc_tables`: enabled by default in `AdvancedVectorIndex::Config`.
- `polysemous_ht`: optional polysemous hash tables for early termination (default: 0).
- No accuracy trade-off; minimal memory overhead (~1–2% of index size).
- Particularly effective for high-dimensional vectors (> 128 dimensions).

### Benchmarks — CHIMERA Suite Branding

> **Files:** `benchmarks/chimera/`, `docs/en/chimera/PRIMARY_SOURCES.md`, `docs/de/chimera/PRIMARY_SOURCES.md`

- Benchmark framework rebranded to **CHIMERA Suite** (_Comprehensive Hybrid Inferencing & Multi-model Evaluation Resource Assessment_).
- Tagline: "Benchmark the Unbenchmarkable".
- Vendor-neutral, scientifically rigorous multi-database benchmark framework.
- Updated all documentation, scripts, and CI workflows to use CHIMERA naming.
- Result files now follow the `CHIMERA_RESULTS_*` naming convention.
- `ThemisDBAdapter` provides the reference implementation against which vendor adapters are evaluated.
- See [CHIMERA Primary Sources — EN](../../en/chimera/PRIMARY_SOURCES.md) and [DE](../../de/chimera/PRIMARY_SOURCES.md).

### Server / API — API Versioning and Compatibility Strategy

> **Files:** `docs/api/API_VERSIONING.md`, `docs/api/DEPRECATION_REGISTRY.md`, `docs/migration/README.md`, `docs/migration/v1.3-to-v1.4.md`

- **`Accept-Version` header** support for REST APIs to specify the desired API version.
- **`API-Version` response header** indicating the version used to process each request.
- **Deprecation tracking system** with automated warning headers (`Deprecation`, `Sunset`, `Link`).
- **24-month deprecation policy** ensuring backward compatibility and smooth migrations.
- **gRPC version negotiation** via metadata (`api-version` key).
- **Version resolution** supporting formats: `v1.4.1`, `v1.4`, `v1`, `latest`.
- **`APIVersionManager`**: centralised version management class.
- **Compatibility matrix** documenting supported versions (v1.0.0 to v1.4.1).
- **Migration guide framework** with templates and best practices.

### Query / Server — Query Result Pagination

> **Files:** `include/query/paginated_response.h`, `src/query/paginated_response.cpp`, tests included

- **Cursor-based pagination** with expiration and versioning (1-hour TTL default).
- **Keyset pagination** using `ORDER BY` values for O(log n) performance.
- **Configurable page sizes** with validation (min: 1, max: 10,000, default: 100).
- Enhanced `PaginatedResponse` with detailed metadata: `PageInfo`, `has_next_page`, `has_prev_page`.
- `ORDER BY` value encoding in cursors eliminates additional database lookups for sort values.
- Cursor expiration prevents stale cursor accumulation.
- Pagination methods: `CURSOR`, `OFFSET`, `KEYSET`.
- 17 comprehensive tests with 100% pass rate.
- Backward compatible with the existing pagination API.

### Plugins — Plugin Metrics and Monitoring

> **Files:** `include/plugins/plugin_metrics.h`, `src/plugins/plugin_metrics.cpp`, `docs/plugins/PLUGIN_METRICS.md`

- **`PluginMetrics`** class for thread-safe metrics collection across all plugins.
- Automatic tracking of load time, reload time, function call latency (P95/P99).
- Resource usage monitoring (memory per plugin).
- Error tracking and count metrics.
- JSON API endpoint: `GET /api/plugins/metrics`.
- Prometheus metrics integrated into `/metrics` endpoint.
- < 1% performance overhead from instrumentation.

### Storage — Schema Manager

> **Files:** `include/storage/schema_manager.h`, `src/storage/schema_manager.cpp`

- Database self-awareness and introspection layer.
- Allows runtime inspection of collection schemas, field types, and index metadata.
- Foundation for schema evolution and validation pipelines.

### Server — Independent Health / Error Service

> **Files:** `src/server/health_server.cpp`, `include/server/health_server.h`

- Dedicated health and error reporting service running on alternate port **9090**.
- Decoupled from the main request-serving port (7777) to remain reachable during overload conditions.
- Exposes `/health`, `/readiness`, and `/error-summary` endpoints.

### Observability — Root Cause Analyzer

> **Files:** `include/observability/root_cause_analyzer.h`, `src/observability/root_cause_analyzer.cpp`
> **Issue:** [#84](https://github.com/makr-code/ThemisDB/issues/84)

- **`RootCauseAnalyzer`**: automated root-cause analysis engine for production incidents.
- `analyzeIssue(event)`: correlates symptoms to probable root causes using historical patterns.
- `findCorrelations(metrics, window)`: identifies statistically correlated metric streams.
- `buildCausalGraph(issues)`: produces a directed causal graph for incident post-mortems.

### Documentation — Archival System and Retroactive Release Building

> **Files:** `docs/RETROACTIVE_RELEASE_GITFLOW.md`, `docs/RETROACTIVE_RELEASE_BUILDING.md`, `docs/RETROACTIVE_RELEASE_EXAMPLES.md`

- **Documentation Archival System**: formal process for archiving outdated documentation; 70+ historical implementation documents moved to `docs/implementation-history/` archive with a comprehensive archive README.
- **Retroactive Release Building System**: pipeline for building binaries from historical version tags, enabling reproducible artifact generation for past releases.

---

## 📚 Documentation & Quality Improvements

### Documentation Audit — Module Sync (PRs #3472–#3479, #3481–#3484)

All 44 source module docs were audited and updated to match their actual implementations:

| Module | Key Changes |
|--------|------------|
| `cdc` | Removed stale file references; outbox + WebSocket marked complete |
| `cache` | Phases 3 & 4 features marked production-ready |
| `auth` | Non-existent file references replaced; Kerberos/TOTP status corrected |
| `base` | 6 undocumented components added; hot-reload status corrected |
| `acceleration` | Full ~30-file directory layout documented; GPU backend selection corrected |
| `analytics` | `streaming_window.cpp` reference corrected; 5 components added |
| `aql` | 12 implemented components added; unimplemented parser features marked |
| `storage (MVCC)` | Three MVCC doc sources consolidated; `MVCCStore`/HLC coverage added |

### CI — Documentation Validation Workflow (PR #3480, #3482)

> **File:** `.github/workflows/documentation-validation.yml`

- New CI workflow implementing the contract described in `docs/DOCUMENTATION_VALIDATION.md`.
- 5 jobs: link-check, markdown-lint, spell-check, structure-check, summary.
- Runs on push and pull-request for all documentation paths.

### Tests & Benchmarks — Coverage Audit (PR #3471)

**6 new benchmark suites:**

| File | Module | What's measured |
|------|--------|----------------|
| `benchmarks/bench_cache_operations.cpp` | cache | Read/write/eviction throughput |
| `benchmarks/bench_auth_operations.cpp` | auth | Token validation, session lookup |
| `benchmarks/bench_cdc_streaming.cpp` | cdc | Event emission and SSE delivery |
| `benchmarks/bench_analytics_aggregation.cpp` | analytics | Window aggregation and groupby |
| `benchmarks/bench_aql_parsing.cpp` | aql | AQL parse + plan latency |
| `benchmarks/bench_config_resolution.cpp` | config | Path resolution and override merge |

**21 new unit test files** closing coverage gaps across acceleration, base, cdc, cache, auth, analytics, aql, config, and other modules.

### Research Documentation (PR #3485)

> **File:** `docs/en/rag/RAG_SCIENTIFIC_FOUNDATIONS.md`

- 460-line IEEE-formatted scientific reference document for the RAG subsystem.
- 40 peer-reviewed citations across all RAG topic areas: dense retrieval, hybrid BM25/vector search, re-ranking, context compression, faithfulness evaluation.

---

## 🔄 Changed

### Storage Module — Write-Amplification Optimisation

- Default `memtable_size_mb` 256 MB → 512 MB: ~50% fewer L0 flushes, 30–40% lower write amplification.
- Default `max_write_buffer_number` 3 → 6: fewer write stalls under sustained load.
- `db_write_buffer_size_mb` capped at 2 GB (was unbounded).
- `enable_async_io` defaults to `true`; `async_io_readahead_size_mb` 64 MB → 128 MB: 2–5× faster sequential scans.
- Server startup now logs memtable size, write buffer count, and async I/O status.

### Configuration Paths (backward-compatible)

- All legacy config paths continue to work via `ConfigPathResolver` with deprecation warnings.
- Full migration guide: `config/MIGRATION_GUIDE.md`.

---

## 🐛 Fixed

- `DiffEngine` initialization updated to accept optional `SnapshotManager` reference.
- HTTP server properly converts between Beast and httplib types for git-feature endpoints.
- Re-enabled `SnapshotManager` (was disabled due to incomplete type issues).
- 119 broken documentation links corrected in hub/index files.
- `documentation-validation.yml` CI workflow created to fulfil existing documentation contract.

---

## 📊 Release Statistics

| Metric | Value |
|--------|-------|
| Post-v1.5.0 merged PRs | 15 |
| Feature PRs (unreleased → v1.7.0) | 16 |
| New benchmark suites | 6 |
| New unit test files | 21 + 17 (pagination) |
| Documentation files audited and corrected | 44+ (all modules) |
| Broken documentation links fixed | 119 |
| Config files reorganized | 60+ |
| New REST API endpoints | 9 (PITR + MergeEngine) + plugin metrics endpoint |
| IEEE citations added (RAG research) | 40 |
| API versions documented (compatibility matrix) | v1.0.0 – v1.4.1 |

---

## 🗺️ Roadmap Traceability

| Roadmap Item | Status | PR / Issue |
|---|---|---|
| Hierarchical config directory structure (`ConfigPathResolver`) | ✅ Done | Unreleased → v1.7.0 |
| Multi-GPU Vector Indexing API (v2.4 scaffolding) | ✅ Done | Unreleased → v1.7.0 |
| Git-Like Features: SnapshotManager + PITR API + MergeEngine | ✅ Done | Unreleased → v1.7.0 |
| HybridSearch production hardening (v1.4.0 config/validation) | ✅ Done | Unreleased → v1.7.0 |
| Distributed Query Optimizer (v1.5.x) | ✅ Done | Unreleased → v1.7.0 |
| FAISS ADC distance table acceleration | ✅ Done | Unreleased → v1.7.0 |
| CHIMERA Suite Branding | ✅ Done | Unreleased → v1.7.0 |
| API Versioning and Compatibility Strategy | ✅ Done | Unreleased → v1.7.0 |
| Query Result Pagination | ✅ Done | Unreleased → v1.7.0 |
| Plugin Metrics and Monitoring | ✅ Done | Unreleased → v1.7.0 |
| Schema Manager | ✅ Done | Unreleased → v1.7.0 |
| Independent Health / Error Service (port 9090) | ✅ Done | Unreleased → v1.7.0 |
| Root Cause Analyzer | ✅ Done | [#84](https://github.com/makr-code/ThemisDB/issues/84) → v1.7.0 |
| Documentation Archival System | ✅ Done | Unreleased → v1.7.0 |
| Retroactive Release Building System | ✅ Done | Unreleased → v1.7.0 |
| Complete 44-module documentation audit | ✅ Done | #3472–#3479, #3481–#3484 |
| Documentation validation CI workflow | ✅ Done | #3480, #3482 |
| Test + benchmark coverage audit (all 44 modules) | ✅ Done | #3471 |
| RAG scientific foundations research document | ✅ Done | #3485 |
| `themis` module initialisation code migration to `src/themis/` | ✅ Done | Roadmap v1.7.0 |

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
| Migration guides for breaking changes documented | ✅ Done (`config/MIGRATION_GUIDE.md`, `themis` include-path guide in Breaking Changes) |
| Documentation audit complete | ✅ Done |
| CI documentation-validation workflow green | ✅ Done |

### QA Status per PR Group

| PR(s) | Module | Tests | Docs | Security Review | QA Flag |
|-------|--------|-------|------|-----------------|---------|
| Config reorganization | config | ✅ | ✅ | — | ✅ |
| Multi-GPU scaffolding | gpu/index | ✅ | ✅ | — | ✅ |
| Git-Like Features | storage/server | ✅ | ✅ | — | ✅ |
| HybridSearch hardening | search | ✅ 88+ tests | ✅ | — | ✅ |
| Query Optimizer | query | ✅ | ✅ | — | ✅ |
| FAISS ADC tables | index | ✅ | ✅ | — | ✅ |
| CHIMERA Suite Branding | benchmarks | ✅ | ✅ | — | ✅ |
| API Versioning Strategy | server/api | ✅ | ✅ | — | ✅ |
| Query Result Pagination | query/server | ✅ 17 tests | ✅ | — | ✅ |
| Plugin Metrics | plugins | ✅ | ✅ | — | ✅ |
| Schema Manager | storage | ✅ | ✅ | — | ✅ |
| Health / Error Service | server | ✅ | ✅ | — | ✅ |
| Root Cause Analyzer | observability | ✅ | ✅ | — | ✅ |
| Documentation Archival | docs | — | ✅ | — | ✅ |
| Retroactive Release Building | ci/docs | — | ✅ | — | ✅ |
| #3471 | tests/benchmarks | ✅ | ✅ | — | ✅ |
| #3472–#3479, #3481–#3484 | docs (all modules) | — | ✅ | — | ✅ |
| #3480, #3482 | ci | ✅ workflow | ✅ | — | ✅ |
| #3485 | rag/research | — | ✅ | — | ✅ |
| themis module migration | themis | ✅ | ✅ | — | ✅ |

---

## 🚩 Pending / Descoped Items

| Item | Issue | Action Required |
|------|-------|-----------------|
| Voice SIP/WebRTC integration | #3431 WIP | Descoped from v1.5.0; target v1.8.0 |
| Build modularisation | #3429 WIP | Descoped from v1.5.0; target v1.8.0 |
| Chimera vendor adapter implementations | #3436 | Third-party adapters (PostgreSQL, MongoDB, etc.) planned for v1.9.x+ |
| Multi-GPU actual GPU execution (NCCL/RCCL) | Roadmap v2.5+ | Scaffolding shipped; GPU execution deferred |

---

## 🔗 Related Documentation

- [CHANGELOG.md](../../../CHANGELOG.md) — Full change log
- [ROADMAP.md](../../../ROADMAP.md) — Top-level project roadmap
- [config/MIGRATION_GUIDE.md](../../../config/MIGRATION_GUIDE.md) — Config path migration guide
- [docs/MULTI_GPU_VECTOR_INDEXING.md](../../MULTI_GPU_VECTOR_INDEXING.md) — Multi-GPU API documentation
- [docs/en/rag/RAG_SCIENTIFIC_FOUNDATIONS.md](../../en/rag/RAG_SCIENTIFIC_FOUNDATIONS.md) — RAG scientific reference
- [docs/api/API_VERSIONING.md](../../api/API_VERSIONING.md) — API versioning strategy
- [docs/api/DEPRECATION_REGISTRY.md](../../api/DEPRECATION_REGISTRY.md) — Deprecation registry
- [docs/plugins/PLUGIN_METRICS.md](../../plugins/PLUGIN_METRICS.md) — Plugin metrics documentation
- [docs/en/chimera/PRIMARY_SOURCES.md](../../en/chimera/PRIMARY_SOURCES.md) — CHIMERA Suite primary sources
- [docs/de/releases/RELEASE_NOTES_v1.5.0.md](RELEASE_NOTES_v1.5.0.md) — Previous release aggregation

---

*This document was produced as part of the v1.7.0 Release Aggregation ([Issue #3486](https://github.com/makr-code/ThemisDB/issues/3486), sub-issue of [Issue #3073](https://github.com/makr-code/ThemisDB/issues/3073)). Last updated: 2026-04-15.*
