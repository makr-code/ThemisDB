[docs](../../index.md) > [en](../index.md) > [llama_cpp](./index.md) > [roadmap](./roadmap-future-verification.md)
**Date:** 2026-04-16
**Status:** review
**Primary Source:**
- `src/llama_cpp/ROADMAP.md`
- `src/llama_cpp/FUTURE_ENHANCEMENTS.md`
- `src/llama_cpp/llama_cpp_plugin.cpp`
- `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`

**Reference:**
- Issue: `[MODULE] llama_cpp`
- Context: Task 2/3 — verification of ROADMAP/FUTURE_ENHANCEMENTS and research notes.

---

# ROADMAP/FUTURE Verification — llama_cpp

## ROADMAP Phase Validation

| Phase | ROADMAP status | Verification |
|---|---|---|
| Phase 1–4 | marked complete | Plausible: API, core implementation, error handling, and test suite A–N exist in code |
| Phase 5 | open (benchmark/concurrency) | Plausible open: no dedicated concurrency tests in `src/llama_cpp/tests/test_llama_cpp_plugin.cpp` |
| Phase 6 | marked complete | Primary docs exist, but parts are outdated (see Reality Check) |

## FUTURE_ENHANCEMENTS Quality

- File follows a clear structure (`Scope`, `Design Constraints`, `Required Interfaces`, `Security / Reliability`) and is generally actionable.
- Correction needed: some entries are already delivered or reference outdated compile-flag naming.

## Research Notes / Constraints

1. **Compile-flag consistency**
   Documentation should consistently use `THEMIS_LLM_ENABLED`, as this is what CMake and C++ code actually use.
2. **Dual operating mode**
   Module has two real modes (wrapper-enabled vs. stub fallback). Secondary docs should explicitly document both paths.
3. **Open hardening gap**
   ROADMAP phase-5 concurrency items remain relevant, since current focused tests do not cover race/parallel scenarios.
