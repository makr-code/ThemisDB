# Next Wave Implementation Plan — 2026-08-26

## Context
Wave 5 gap closure complete (81/81 checkboxes [x] in MODULE_GAP_ANALYSIS_WAVE2.md).
This plan targets Wave A closure + Wave B deferred production code gaps.

## Priority Ranking (P1 = highest business risk)

| ID | Module | Gap | Type | Priority |
|---|---|---|---|---|
| N1 | voice | Wake-word/intent/command pipeline fallback alignment; partial backend failure matrix; noisy wake-word adversarial expansion | Wave A | P1 |
| N2 | analytics | Federated query coordinator real wiring + forecasting model integrity check | Wave A/B | P2 |
| N3 | llm | Thread-safety audit — top-20 shared state sites (static caches, global registries) + `std::atomic`/mutex additions | Wave B | P3 |
| N4 | llm_wiki | RocksDB backend replacing in-memory mock (Wave B partial closure) | Wave B | P4 |

## Subagent Assignment
- **impl-voice-wave-a**: N1 (Voice A3 hardening)
- **impl-analytics-wave**: N2 (Analytics federated coordinator + forecasting)
- **impl-llm-threadsafety**: N3 (LLM thread-safety top-20)
- **impl-llm-wiki-rocksdb**: N4 (LLM Wiki RocksDB backend)

## Acceptance Criteria per Gap
### N1 Voice
- Wake-word/intent/command pipelines: each path has explicit fallback that returns a safe-default response + logs THEMIS_WARN
- Partial backend failure: if primary backend throws/times out, secondary path is tried before returning fail-closed error
- Noisy wake-word: test coverage for wake-word detection under noise (false-positive + true-positive cases)
- Tests: `tests/voice/test_wave_next_voice_hardening.cpp` (10+ tests)
- ROADMAP: `[ ]` items flipped to `[x]`

### N2 Analytics
- Federated coordinator: `distributed_analytics.cpp` executeDistributed() wires real cross-shard retry (not just caller-re-issue)
- Forecasting: model integrity check (checksum/version validation before serving)
- Tests: `tests/analytics/test_wave_next_analytics_hardening.cpp` (8+ tests)

### N3 LLM Thread-safety
- Top-20 sites: `std::atomic<>` wrapping for shared counters; `std::mutex`+`std::lock_guard` for shared maps/caches
- No data races on repeated concurrent `getAdapter()`, `getModel()`, `getCache()` calls
- Tests: thread-safety stress tests (2+ threads, 100+ iterations)

### N4 LLM Wiki RocksDB
- `LLMWikiPluginImpl` persistence backed by RocksDB column family (not in-memory hash map)
- Index/query/cache operations use `db->Put()`, `db->Get()`, `db->NewIterator()`
- STUB/SIMULATION NOTE updated: Removal Plan changed from "pending" to "done"
- Tests: verify persistence survives restart (write → close → reopen → read)

## Target Branch: develop (current: copilot/core-modules-gaps-analysis)
