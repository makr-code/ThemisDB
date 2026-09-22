> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

# Changelog — Execution Module

All notable changes to the Execution module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Added
- `src/execution/AUDIT.md`, `src/execution/CHANGELOG.md`, `src/execution/FUTURE_ENHANCEMENTS.md`, `src/execution/MODULE_GAPS.md`, `src/execution/PERFORMANCE_EXPECTATIONS.md`, `src/execution/PRODUCTION_REQUIREMENTS.md`, and `src/execution/SECURITY.md`: restored the required execution governance document set.

### Changed
- `src/execution/README.md`, `src/execution/ARCHITECTURE.md`, and `src/execution/ROADMAP.md`: aligned the execution narrative with the live bounded scheduler and fixed-size central-queue worker pool implementation.
- `include/execution/query_scheduler.h` and `include/execution/thread_pool_manager.h`: refreshed Doxygen comments so the public API documents current behavior, reserved configuration fields, and shutdown/error semantics.

## [1.0.0] — 2026-08-08

### Added
- `include/execution/query_scheduler.h`, `src/execution/query_scheduler.cpp`: initial deadline-ordered query scheduler with queue-depth backpressure, low-priority shedding, and SLA-completion metrics.
- `include/execution/thread_pool_manager.h`, `src/execution/thread_pool_manager.cpp`: initial bounded worker-pool implementation with central dispatch queue, graceful shutdown, and exception accounting.
- `tests/integration/test_load_balancing.cpp`, `tests/integration/test_resource_pooling.cpp`: focused scheduler and thread-pool regression coverage.

## [1.1.0] — 2026-09-16

### Added
- `tests/execution/test_execution_highcardinality_stress.cpp`: Wave-D stress coverage for high-cardinality dispatch and queue contention.
- `benchmarks/execution/bench_execution_dedicated_gates.cpp`: dedicated execution benchmark gates for enqueue, dequeue, drain, and dispatch throughput.

### Changed
- `src/server/http_server.cpp` and `include/server/http_server.h`: instantiate and tear down the execution scheduler and thread pool under `THEMIS_EXECUTION_MODULE`.
