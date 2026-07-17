# EPIC 3 Phase 5 Benchmark Repro/Triage

## When to use

Use this checklist when an EPIC 3 Phase 5 benchmark gate fails or a profile is
missing from the aggregated result bundle.

## Triage order

1. Confirm the executed benchmark binary matches the planned profile source.
2. Confirm canonical seed `42` and intended topology parameters were used.
3. Check whether the failure is isolated to latency, throughput, integrity, recovery, or availability.
4. Re-run the failing profile in isolation before changing thresholds.
5. Record the failing gate, raw metric, and observed topology delta.

## Failure classes

- **Latency regression:** planner or control-plane p99 exceeds budget
- **Throughput regression:** placement throughput falls below the minimum
- **Integrity regression:** verification success ratio drops below the gate
- **Recovery regression:** rebuild time or retry budget exceeds the bound
- **Determinism regression:** coefficient of variation exceeds the allowed drift
- **Coverage regression:** one or more expected profile results are missing

## Escalation notes

- Do not relax thresholds without updating `src/distributed_tensor/PERFORMANCE_EXPECTATIONS.md`.
- Do not promote Phase 6 docs with partial results.
- Link every failed gate to its raw evidence bundle and follow-up issue.
