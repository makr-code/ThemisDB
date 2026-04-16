[docs](../../README.md) > [en](../README.md) > [chaos](./index.md) > [README](./README.md)
**Datum:** 2026-04-16
**Status:** review
**Primary (Quelle der Wahrheit):**
- `src/chaos/README.md`
- `src/chaos/ARCHITECTURE.md`
- `src/chaos/ROADMAP.md`
- `src/chaos/FUTURE_ENHANCEMENTS.md`
- `include/chaos/README.md`
- `include/chaos/ROADMAP.md`

**Bezug / Reference:**
- Issue: #4685
- Context: Secondary documentation for the module-level reality check and documentation migration of `chaos`.

---

# Chaos Module — Overview and Verification Status

## TL;DR

The `chaos` module is implemented as an **in-process chaos/fault-injection component**.  
Core classes (`FaultInjector`, `ChaosScheduler`) are implemented and covered by unit/stress tests and benchmarks.

## Reality-check scope

- **Implementation:** `src/chaos/chaos_framework.cpp`
- **Public API:** `include/chaos/chaos_framework.h`
- **Tests:** `tests/test_chaos_framework.cpp`, `tests/test_chaos_scheduler.cpp`, `tests/test_chaos_stress.cpp`
- **Benchmark:** `benchmarks/bench_chaos_stress.cpp`

## Verified statements

1. **Primary docs ↔ code are aligned for core functionality**
   - Fault types, fault lifecycle, and scheduler behavior in `README`/`ARCHITECTURE` match implementation.
2. **ROADMAP status checked against evidence**
   - `include/chaos/ROADMAP.md` Phase 4 is now marked `[x]` with concrete test evidence.
3. **FUTURE_ENHANCEMENTS remains actionable**
   - Required sections (`Scope`, `Design Constraints`, `Required Interfaces`, `Test Strategy`, `Performance Targets`, `Security / Reliability`) are present and implementable.

## Open items / known gaps

- Cluster-wide distributed chaos coordination is still open (`src/chaos/ROADMAP.md`, In Progress).
- ABI compatibility matrix for external include consumers is still open (`include/chaos/ROADMAP.md`, Production Readiness Checklist).

German detailed gap report:
- `docs/de/chaos/missing-implementations.md`
