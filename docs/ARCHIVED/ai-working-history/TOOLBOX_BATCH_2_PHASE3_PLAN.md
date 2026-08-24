# Batch 2 Implementation Plan: Phase 3 - Error Handling & Diagnostics

## Objectives
1. Unify incident taxonomy across 4 execution planes
2. Add Prometheus metrics for all error classes
3. Implement deterministic stress fixtures
4. Extend test coverage (IT-13..IT-20 from Phase 2 completion)

## Core Incident Taxonomy (to be unified across module)

### Layer 1: Orchestration (ingestion_toolbox.cpp)
- extraction_empty: No text extracted from content
- extraction_failed: Processor returned error
- extraction_timeout: Operation exceeded budget
- extraction_overflow: Output too large

### Layer 2: Bridge (content_toolbox_bridge.cpp)
- bridge_no_text: ContentManager couldn't extract text
- bridge_writer_failed: Writer sink (graph/vector) returned error
- bridge_toolbox_failed: toolbox_ returned error
- bridge_empty_result: Bridge succeeded but with no entities

### Layer 3: Registry (toolbox_registry.cpp)
- registry_not_initialized: globalToolbox() called before initialize()
- registry_double_init: initialize() called when already initialized
- registry_reset_during_active: reset() during active usage

### Layer 4: Helper (text_*.cpp)
- helper_empty_input: Empty text passed to helper
- helper_encoding_unsupported: Text encoding not supported
- helper_size_exceeded: Helper exceeded size limit
- helper_malformed_input: Invalid format for operation

## Prometheus Metrics to Add

- `toolbox_bridge_failures_total` - Counter for bridge soft-fail events
- `toolbox_registry_misuse_total` - Counter for registry misuse attempts
- `toolbox_helper_errors_total` - Counter for helper utility errors
- `toolbox_extraction_latency_us` - Histogram of extraction latencies
- `toolbox_bridge_latency_us` - Histogram of bridge operation latencies

## Stress Fixture Patterns

1. HighConcurrencyFixture: 8+ threads concurrent extraction
2. MixedContentFixture: Text + Binary + Structured metadata
3. DegradedPathFixture: Writer failures, null backends
4. LongRunStressFixture: 10k+ iterations with deterministic seed

## Files to Modify

- `src/toolbox/SECURITY.md` - Document incident taxonomy
- `src/toolbox/ingestion_toolbox.cpp` - Add metrics
- `src/toolbox/content_toolbox_bridge.cpp` - Add metrics
- `src/toolbox/toolbox_registry.cpp` - Add error tracking
- `tests/toolbox/test_toolbox_phase5.cpp` - Extend with stress scenarios
- `src/toolbox/ROADMAP.md` - Mark Phase 3 as 75%→90%

## Success Criteria

✅ Incident taxonomy documented in SECURITY.md
✅ All new metrics emitted in getMetricsText()
✅ Stress fixtures demonstrate deterministic behavior
✅ IT-13..IT-20 all pass (from Phase 2)
✅ No new CodeQL alerts
✅ Build succeeds on linux-release
