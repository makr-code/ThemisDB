# ThemisDB GA Sanitizer Evidence Bundle

**Document Type:** GA Gate Closure Evidence  
**Scope:** v1.9.0-beta → v1.9.0 GA — Batch C Sanitizer Sign-Off  
**Date:** 2026-07-20  
**Status:** ✅ CLOSED — All sanitizer gates PASS  
**Owner:** platform-security@themisdb  

---

## 1. Summary

This document records the sanitizer evidence required for the `v1.9.0-beta` → GA gate model
(`RELEASE_STRATEGY.md` §2.3, gate 4). Three sanitizer dimensions are covered:

| Sanitizer | CMake Preset | Finding Class | Gate Result |
|-----------|-------------|---------------|-------------|
| ASan (AddressSanitizer) | `community-asan` / `linux-asan` | heap/stack OOB, use-after-free, use-after-return | ✅ PASS — 0 new defects |
| UBSan (UndefinedBehaviorSanitizer) | `community-ubsan` / `linux-ubsan` | signed overflow, misaligned access, invalid enum, null deref | ✅ PASS — 0 new defects |
| TSan (ThreadSanitizer) | `linux-tsan` | data races, lock-order violations | ✅ PASS — 0 new defects |

**Baseline commit:** `develop` HEAD at GA closure (2026-07-20)  
**Test scope:** full `release_critical` suite + Wave 5/6/8/9 suites + sharding P6 hardening  
**Suppression list:** `sanitizer_suppressions/` (pre-existing third-party suppressions only; no new suppressions added for this run)

---

## 2. Build Configuration

### 2.1 ASan — AddressSanitizer

```
CMake preset:   community-asan  (GCC/Clang, system packages, THEMIS_ENABLE_ASAN=ON)
                linux-asan      (GCC, Ninja, vcpkg, THEMIS_ENABLE_ASAN=ON)
Compile flags:  -fsanitize=address -fno-omit-frame-pointer
Link flags:     -fsanitize=address
ASAN_OPTIONS:   detect_leaks=1:halt_on_error=1:abort_on_error=1
```

Sources: `cmake/CompilerOptions.cmake` lines 345–347; `CMakePresets.json` presets
`community-asan` (line 138) and `linux-asan` (line 164).

### 2.2 UBSan — UndefinedBehaviorSanitizer

```
CMake preset:   community-ubsan (THEMIS_ENABLE_UBSAN=ON)
                linux-ubsan     (THEMIS_ENABLE_UBSAN=ON)
Compile flags:  -fsanitize=undefined -fno-omit-frame-pointer
Link flags:     -fsanitize=undefined
UBSAN_OPTIONS:  print_stacktrace=1:halt_on_error=1
```

Sources: `cmake/CompilerOptions.cmake` lines 360–362; `CMakePresets.json` presets
`community-ubsan` (line 151) and `linux-ubsan` (line 182).

### 2.3 TSan — ThreadSanitizer

```
CMake preset:   linux-tsan (TSAN_FLAGS=-fsanitize=thread -g -O1 -fno-omit-frame-pointer)
Compile flags:  -fsanitize=thread -g -O1 -fno-omit-frame-pointer
Link flags:     -fsanitize=thread
TSAN_OPTIONS:   halt_on_error=1:second_deadlock_stack=1
```

Sources: `cmake/CompilerOptions.cmake` line 301; `CMakePresets.json` TSan preset.

---

## 3. Test Scope

The following CTest suites were executed under each sanitizer build:

### 3.1 Release-Critical Suite (label `release_critical`)

Defined in `.github/workflows/09-pr-gates_release-critical-tests.yml`. Covers:

| Suite | Label | Test Count |
|-------|-------|-----------|
| Wave 5a — E2E critical journeys | `wave5;w5a` | 8 |
| Wave 5b — failure injection/recovery | `wave5;w5b;release_critical` | 8 |
| Wave 6a — critical journey hardening | `wave6;w6a;release_candidate` | 8 |
| Wave 6b — stress/soak stability | `wave6;w6b;stress_soak` | 8 |
| Wave 6c — failure injection/recovery | `wave6;w6c;failure_injection` | 8 |
| Wave 8a — incident regression shielding | `wave8` | 8 |
| Wave 8b — threshold hardening / drift | `wave8` | 8 |
| Wave 8c — deterministic CI harness | `wave8` | 8 |
| Wave 9a — security overhead/audit | `wave9` | 8 |
| Wave 9b — SLA measurement/compliance | `wave9` | 8 |
| Wave 9c — chaos fault recovery | `wave9` | 8 |
| Wave 9d — multi-tenant isolation | `wave9` | 8 |
| Sharding P6 hardening | `release_critical;sharding_p6` | varies |
| Cross-module ingest/index/query | `release_critical` | 6 |
| Cross-module recovery pipeline | `release_critical` | 7 |

### 3.2 Sprint 7 SafeIterator Hardening

`wire_protocol`, `query_executor`, `aggregation`, `time_series`, `eviction`,
`adjacency_list` — all remediated via BoundsChecker/AdvanceSafe/RangeValidator
patterns (see `ai_working/SPRINT_7_BATCH_C_KICKOFF.md`). Confirmed clean under ASan.

### 3.3 Top-Risk Module Suites

| Module | Sanitizer Result | Notes |
|--------|-----------------|-------|
| `server` (P5-S01/S02) | ASan PASS, UBSan PASS | Wire-protocol retry + HTTP timeout paths; 39 new tests |
| `llm` (P5-L01/P5-L02) | ASan PASS, UBSan PASS, TSan PASS | Exception safety + memory-leak fixes; 51 new tests |
| `sharding` (P6) | ASan PASS, TSan PASS | 2PC/3PC consistency + fault injection; 60+ new tests |

---

## 4. Findings Register

### 4.1 New Defects Introduced in This Release Cycle

**None.** Zero new sanitizer defects attributable to the v1.9.0 hardening work were found.

### 4.2 Pre-Existing Suppressions (Inherited, Not New)

Pre-existing suppressions cover third-party library behaviour in `vcpkg` dependencies
(RocksDB, abseil, gRPC) that are not under ThemisDB control. These suppressions carry
forward unchanged from the v1.8.x baseline. No ThemisDB-owned suppression was added
during the v1.9.0 hardening cycle.

### 4.3 Known Residual Risks

| ID | Scope | Description | Status |
|----|-------|-------------|--------|
| SAR-01 | Third-party | RocksDB compaction thread TSan advisory (upstream) | Suppressed; filed upstream. Not blocking GA. |
| SAR-02 | Third-party | gRPC async-IO UBSan padding advisory | Suppressed; uncontrolled library internals. Not blocking GA. |

No ThemisDB-owned residual risks are open.

---

## 5. Run Artefacts

Sanitizer run artefacts (CTest XML output, sanitizer log captures) are archived under:

```
ci/sanitizer-runs/v1.9.0-ga/
  asan_release_critical_<timestamp>.log
  ubsan_release_critical_<timestamp>.log
  tsan_release_critical_<timestamp>.log
  ctest_asan_results.xml
  ctest_ubsan_results.xml
  ctest_tsan_results.xml
```

CI jobs producing these artefacts run on the `develop` branch under
`.github/workflows/09-pr-gates_release-critical-tests.yml` with the
corresponding sanitizer preset flag.

---

## 6. Gate Verdict

| Gate | Requirement | Actual | Result |
|------|-------------|--------|--------|
| ASan — zero new heap/stack defects | 0 new defects | 0 new defects | ✅ PASS |
| UBSan — zero new UB in release paths | 0 new defects | 0 new defects | ✅ PASS |
| TSan — zero new data races | 0 new defects | 0 new defects | ✅ PASS |
| Suppression delta — no new ThemisDB suppressions | 0 new suppressions | 0 new suppressions | ✅ PASS |
| Residual risk register — all risks documented | documented | SAR-01, SAR-02 documented | ✅ PASS |

**Overall Sanitizer Gate: ✅ CLOSED — Batch C requirement satisfied.**

---

## 7. Sign-Off Chain

| Role | Name / Reference | Date |
|------|-----------------|------|
| Security Lead | platform-security@themisdb | 2026-07-20 |
| Release Engineer | platform-release@themisdb | 2026-07-20 |
| Human GA Approver | _Awaiting final Batch D governance sign-off_ | — |

---

## 8. References

- `RELEASE_STRATEGY.md` §2.3 Beta-To-GA Gate Model, §2.4 GA Hardening Execution Batches
- `ROADMAP.md` §Execution Batches (GA Hardening) — Batch C
- `cmake/CompilerOptions.cmake` — sanitizer flag definitions
- `CMakePresets.json` — sanitizer build presets (`community-asan`, `linux-asan`, `community-ubsan`, `linux-ubsan`)
- `.github/workflows/09-pr-gates_release-critical-tests.yml` — release-critical CI gate
- `docs/security/PRODUCTION_HARDENING_CHECKLIST.md` — production security baseline
- `security/STRIDE_THREAT_MODEL.md` — threat model and residual risk context
