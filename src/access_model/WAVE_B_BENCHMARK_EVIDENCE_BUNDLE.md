---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Access Model Module — Wave B Benchmark Evidence Bundle

## Status and scope

This bundle documents the six target constants and benchmark cases in
`benchmarks/access_model/bench_access_coordinator_gates.cpp`. It is not evidence
that a performance gate passed. Source inspection on 2026-10-31 found that the
current fixture starts the coordinator without setting an `AgeBasedPolicy`.
Consequently, callback cases use the no-policy observation path rather than
queueing policy-driven tier work.

The source also calculates the reported latency as total elapsed time divided by
iterations, not p99; the memory case does not measure RSS. Its
`reportViolation()` helper prints to stderr but does not fail the process,
compare a stored baseline, or enforce the documented ±10% regression tolerance.
Representative-hardware results and validated end-to-end gates remain pending.

## Declared gate targets and source behavior

| Gate | Declared target | Case | What the current case exercises |
|---|---|---|---|
| GATE-ACM-01 | L1→L2 ≤ 50 µs p99 | `GATE_ACM_01_L1ToL2Promotion` | `onStorageAccess(STORAGE_COLD, 5, ...)`; no policy or promotion. |
| GATE-ACM-02 | Eviction feedback ≤ 100 µs p99 | `GATE_ACM_02_CacheEvictionToStorageFeedback` | `onCacheEvicted(L1_WORKING, ..., 10, ...)`; no policy or demotion. |
| GATE-ACM-03 | Cold→warm ≤ 100 ms p99 | `GATE_ACM_03_ColdToWarmPromotion` | Three cold-storage access callbacks; no policy or adapter I/O. |
| GATE-ACM-04 | Throughput ≥ 10,000 events/sec | `GATE_ACM_04_EventProcessingThroughput` | Alternating callback calls on the no-policy path. |
| GATE-ACM-05 | ≤ 50 MB for 1 M events | `GATE_ACM_05_MemoryOverhead` | Event batches and a sleep; no memory measurement or bound assertion. |
| GATE-ACM-06 | Policy decision ≤ 10 µs/event | `GATE_ACM_06_PolicyDecisionOverhead` | Eviction callbacks on the no-policy path; no policy decision. |

These are source-defined targets, not observed performance results. In
particular, the cases named promotion or policy-decision do not currently
exercise those operations.

## Evidence needed to close the gate

- Update the fixture to set a valid policy and verify each case queues or
  performs the intended transition.
- Measure the intended percentile and include adapter I/O when a target claims
  transfer latency.
- Measure resident memory externally for GATE-ACM-05.
- Record a baseline and make threshold violations fail validation.
- Attach reproducible release-profile results from representative hardware.

Do not treat a clean benchmark invocation or the presence of target constants as
a gate pass. The current source is useful only as a callback-path scaffold until
the above validation is implemented and run.

## Suggested repeatable run

After building the benchmark target with benchmarks enabled:

```bash
./bench_access_coordinator_gates \
  --benchmark_filter="GATE_ACM_0[1-6]" \
  --benchmark_repetitions=5 \
  --benchmark_report_aggregates_only=true \
  --benchmark_out=gate_acm_results.json \
  --benchmark_out_format=json
```

For comparable future baselines, record the CPU, memory, OS/kernel, compiler,
build type, benchmark configuration, and output artifact. The former hardware
profile and acceptance checklist are not retained as passed evidence because no
representative-hardware result is included in this bundle.

## Related status

- [Performance expectations](./PERFORMANCE_EXPECTATIONS.md) describe the source
  limitations and outstanding validation.
- [Module roadmap](./ROADMAP.md) tracks local implementation and evidence status.
- The repository-level [`ROADMAP.md`](../../ROADMAP.md) remains canonical for
  release and wave readiness.

**AI provenance:** Exact model backend not verifiable from final document text alone.
