# Changelog - Access Model Module

<!-- Status: current | validated: 2026-09-21 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

All notable changes to the access_model module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Added
- Governance documentation: AUDIT.md, CHANGELOG.md, FUTURE_ENHANCEMENTS.md,
  MODULE_GAPS.md, PERFORMANCE_EXPECTATIONS.md, PRODUCTION_REQUIREMENTS.md, SECURITY.md
  created to close compliance gap (issue #6466).

---

## [1.2.0] - 2026-08-17

### Added
- Phase 5 observability: structured logging (`access_model_logging.h/cpp`), trace correlation
  (`access_model_trace.h/cpp`), Prometheus-compatible metrics.
- Phase 6 E2E integration tests (`tests/access_model/test_access_model_e2e.cpp`, 15 scenarios).
- Concurrency tests (`tests/access_model/test_coordination_concurrency.cpp`, 12 patterns).
- Benchmark gates (`benchmarks/access_model/bench_access_coordinator_gates.cpp`,
  GATE-ACM-01..06).
- Operator runbooks (5 scenarios in `docs/operability/`).
- Dashboard guide and Prometheus panel definitions.

### Changed
- ROADMAP.md updated to reflect Phase 5-6 completion and Wave B readiness.

---

## [1.1.0] - 2026-08-09

### Added
- Phase 2 core coordinator implementation (`access_coordinator.cpp`, ~530 LOC).
- `age_based_policy.cpp` helper methods (~303 LOC).
- `access_metrics.cpp` collectors (~262 LOC).
- CMakeLists.txt module registration; sources added to cmake/CMakeLists.txt THEMIS_CORE_SOURCES.
- Unit tests ACM-01..ACM-08 (`tests/access_model/test_access_coordinator_focused.cpp`).

### Added (Phase 3 — Cache Integration)
- Eviction listener support in `cache/adaptive_query_cache.h/cpp`.
- `setEvictionListener()` method; `emitEvictionEvent()` helper.
- Full eviction event payload: key, tier, size_bytes, access_count, age_secs, reason.
- Integration tests CAI-01..CAI-10 (`tests/access_model/test_cache_storage_integration.cpp`).

### Added (Phase 4 — Storage Integration)
- `setPromotionListener()` and `emitPromotionEvent()` in `TieredStorageManager`.
- Hot pattern detection in `get()` and `runMigrationCycle()` for WARM/COLD tiers.
- Access window calculation and full promotion event payload.

---

## [1.0.0] - 2026-08-03

### Added
- Phase 1 architecture and interface definition:
  - `AccessTier` abstract interface (`include/access_model/access_tier_interface.h`).
  - `AccessCoordinator` broker interface (`include/access_model/access_coordinator.h`).
  - `PromotionRequest` / `DemotionRequest` data structures (`promotion_demotion.h`).
  - `AgeBasedPolicy` unified aging policy (`age_based_policy.h`).
  - `AccessMetrics` observability surface (`access_metrics.h`).
- Architecture documentation (`ARCHITECTURE.md`, `ROADMAP.md`, `README.md`).
- Cross-module integration guides (`docs/architecture/UNIFIED_ACCESS_MODEL.md`,
  `docs/architecture/CACHE_STORAGE_INTEGRATION.md`).
