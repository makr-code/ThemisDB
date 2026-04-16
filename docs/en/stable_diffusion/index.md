[docs](../INDEX.md) > [en](../INDEX.md) > [stable_diffusion](./index.md) > [index](./index.md)
**Date:** 2026-04-16
**Status:** review
**Primary Source:**
- `src/stable_diffusion/README.md`
- `src/stable_diffusion/ARCHITECTURE.md`
- `src/stable_diffusion/ROADMAP.md`
- `src/stable_diffusion/FUTURE_ENHANCEMENTS.md`
- `src/stable_diffusion/CHANGELOG.md`
- `src/stable_diffusion/sd_plugin.cpp`
- `include/stable_diffusion/sd_plugin.h`

**Reference:**
- Issue: `[MODULE] stable_diffusion`
- Context: Module-level reality check and documentation migration (Primary → Secondary)

---

# stable_diffusion — Reality Check & Documentation Sync

## TL;DR

Primary docs were checked against implementation reality. This page records:
- concrete doc-vs-code deltas,
- ROADMAP/FUTURE verification outcomes,
- research constraints and decisions,
- linkage to the DE missing-implementations report.

## Task 1 — Reality check against source code

| Topic | Primary claim | Code reality | Evidence |
|---|---|---|---|
| Test coverage | Older README/ARCHITECTURE counts | `SDPluginFocusedTests` now has 51 tests | `src/stable_diffusion/README.md`, `src/stable_diffusion/ARCHITECTURE.md`, `src/stable_diffusion/tests/test_sd_plugin.cpp`, `tests/CMakeLists.txt` |
| PNG encoding | Older architecture text described stub PNG path | `encodeMinimalPng()` writes IHDR+IDAT+IEND with CRC/Adler | `src/stable_diffusion/ARCHITECTURE.md`, `src/stable_diffusion/sd_plugin.cpp` |
| SDCppGenerator status | Future/Audit had stale "not implemented" notes | `SDCppGenerator` is implemented and CMake-gated | `src/stable_diffusion/FUTURE_ENHANCEMENTS.md`, `src/stable_diffusion/AUDIT.md`, `include/stable_diffusion/sd_generator.h`, `src/stable_diffusion/CMakeLists.txt` |

## Task 2 — ROADMAP/FUTURE_ENHANCEMENTS verification

- ROADMAP phases 1–4 are implemented and code-backed.
- Remaining roadmap gaps are benchmark + SDCpp parallel audit + real-model E2E test.
- FUTURE_ENHANCEMENTS now focuses on still-open, actionable items.

## Task 3 — Research notes / constraints

- Real backend path depends on `THEMIS_ENABLE_STABLE_DIFFUSION=ON` and linked library availability.
- `SDPlugin` enforces content-policy checks for `prompt` and `negative_prompt` in all generate paths.
- `SDCppGenerator` concurrency behavior still needs dedicated audit due to downstream runtime constraints.

## Related Secondary Docs

- [PRIMARY_SOURCES.md](./PRIMARY_SOURCES.md)
- [DE Missing Implementations](../../de/stable_diffusion/missing-implementations.md)
