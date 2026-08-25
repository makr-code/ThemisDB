# Batch 3 Implementation Plan: Phase 5 - Performance & Benchmarking

## Objectives
1. Validate proxy-benchmark behavior against release baselines
2. Establish p95/p99 envelopes for hot paths
3. Expand native toolbox benchmark suite
4. Certify release gates GATE-TBX-P1..P6

## Performance Gates to Validate

| Gate ID | Operation | Target | Benchmark Case |
|---------|-----------|--------|-----------------|
| GATE-TBX-P1 | extractEntities throughput | ≥ 100K ops/s | BM_ExtractEntities_Throughput |
| GATE-TBX-P2 | extractEntitySet latency | p95 ≤ 50ms | BM_ExtractEntitySet_Latency |
| GATE-TBX-P3 | Text normalization | p95 ≤ 10ms | BM_TextNormalization_Latency |
| GATE-TBX-P4 | Language detection | p95 ≤ 15ms | BM_LanguageDetection_Latency |
| GATE-TBX-P5 | Fingerprinting throughput | ≥ 1M ops/s | BM_ContentFingerprinting_Throughput |
| GATE-TBX-P6 | Bridge enrichment | p95 ≤ 100ms | BM_BridgeEnrichment_Placeholder |

## Baseline Measurement Plan

1. **Q3 2026 Baseline Collection** (Prerequisite)
   - Run all 6 gates on release profile hardware
   - Capture p50, p95, p99 percentiles
   - Document hardware/OS specifics

2. **Proxy-to-Native Delta Analysis**
   - Compare proxy benchmarks (bench_ingestion_*.cpp) vs native suite
   - Identify performance divergence points
   - Document expected variance

3. **Hot-Path Envelope Establishment**
   - extractEntities: < 1ms per entity @ 100K ops/s
   - Bridge::ingest: < 100ms @ p95 latency
   - Text normalization: < 10ms @ p95 latency
   - Fingerprint: > 1M ops/s throughput

## Files to Enhance

- `benchmarks/toolbox/bench_toolbox_release_gates.cpp` - Verify 4 existing gates (GATE-TBX-01..04)
- `benchmarks/toolbox/bench_toolbox_native_workloads.cpp` - Expand with 6 primary gates (GATE-TBX-P1..P6)
- `src/toolbox/PERFORMANCE_EXPECTATIONS.md` - Document p95/p99 baselines
- `src/toolbox/ROADMAP.md` - Mark Phase 5 as 50%→85% with gate certification status

## Benchmark Hygiene Rules (per benchmarks/MEASUREMENT_HYGIENE.md)

- Use kCanonicalRngSeed = 42 for deterministic randomization
- I/O benchmarks: use OS temp dir + steady_clock timestamp
- CPU benchmarks: use steady_clock (not UseRealTime)
- I/O registrations: use UseRealTime()
- All measurements: at least 5 repetitions with aggregate reporting

## Stress Patterns for Phase 5

1. **Sustained Load Testing** (10k+ iterations)
   - Verify p95/p99 envelopes under load
   - No regression vs baseline

2. **Concurrent Extraction** (8+ threads)
   - Verify gate thresholds under concurrency
   - Check metrics consistency

3. **Mixed Workload Simulation**
   - Combine extraction + bridge + fingerprinting
   - Measure tail latencies

## Success Criteria

✅ GATE-TBX-P1..P6 baseline measurements captured
✅ p95/p99 envelopes documented in PERFORMANCE_EXPECTATIONS.md
✅ Proxy-to-native delta analysis complete
✅ No regression > 10% vs Q3 2026 baseline
✅ 6 primary benchmark cases operational
✅ All gates report consistent results across 5 runs
