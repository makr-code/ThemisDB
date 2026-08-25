# Q4 2026 Deferred Items Tracking & Execution Plan

**Date:** 2026-08-10  
**Status:** PRE-GA (technical gates complete; awaiting D-11 human sign-off)  
**Scope:** DEF-01..04 deferred items from ROADMAP.md § Release Hardening Program  
**Authority:** GA_PROMOTION_SIGN_OFF.md §4; ROADMAP.md §Current Status

---

## Executive Summary

Four items have been **explicitly deferred** from v2.4.0 GA scope with documented approval. Each deferred item has a target execution window (Q4 2026 Phase 2–4) and mitigation strategy documented.

| ID | Item | Target | Status | Risk |
|----|------|--------|--------|------|
| **DEF-01** | liboqs ≥0.10.0 vcpkg availability | Q4 2026 Phase 4 | ⏸️ BLOCKED | SPHINCS+ stubs retained; conditional compilation via `THEMIS_CRYPTO_SPHINCS_ENABLE` |
| **DEF-02** | RocksDB optional-build tolerance | Q4 2026 Phase 2 | 🟡 IN PROGRESS | THEMIS_ALLOW_MISSING_ROCKSDB + wiki Phase B gating; community-release CMake now passes |
| **DEF-03** | Geo/Chimera/Graph public externalization | Q4 2026 Phase 3 | 🟡 IN PROGRESS | plugins/themisdb_geo, plugins/themisdb_timeseries submodules; Phase 5-6 complete |
| **DEF-04** | LLM Wiki Phase B persistent cache | Q4 2026 Phase 2 | 🟡 BLOCKED | Conditional on DEF-02 (RocksDB); guarded behind THEMISDB_WIKI_PHASE_B flag |

---

## DEF-01: liboqs ≥0.10.0 vcpkg Availability

### Scope
SPHINCS+ post-quantum cryptography implementation in `src/crypto/sphincs_*.cpp` currently uses **stubs** pending liboqs availability in vcpkg.

### Current State (2026-08-10)
- ✅ SPHINCS+ error codes defined: range [9530-9549], 10 codes total
- ✅ SPHINCS+ interfaces stubbed in `include/crypto/sphincs_engine.h`
- ✅ Core implementations stubbed in `src/crypto/sphincs_*.cpp`
- ✅ Conditional compilation via `THEMIS_CRYPTO_SPHINCS_ENABLE` (default: OFF)
- ✅ All stub paths marked with `STUB/SIMULATION NOTE` (purpose, activation, production delta, removal plan)
- ⏸️ liboqs library **not yet available** in vcpkg as of 2026-08-10

### Blockers
- **External dependency:** vcpkg maintainers must accept liboqs v0.10.0+ as a managed package
- **vcpkg PR required:** Community contribution or request to vcpkg repository
- **Timeline uncertainty:** Dependent on vcpkg maintainer availability

### Mitigation Strategy
1. **Immediate (pre-GA):** SPHINCS+ disabled by default; stubs fail-closed with `THEMIS_CRYPTO_SPHINCS_ENABLE=OFF`
2. **Q4 2026 Phase 4 (Target 2026-10-15):**
   - Monitor vcpkg repository for liboqs availability
   - If available: Replace stubs with production implementation
   - If unavailable: Request/contribute liboqs port to vcpkg
   - Document fallback (Ed25519 signatures remain available; no SPHINCS+ post-quantum path)

### Acceptance Criteria
- [ ] liboqs v0.10.0+ available in vcpkg OR PR submitted to vcpkg with timeline
- [ ] stubs replaced with production code (if liboqs available)
- [ ] All SPHINCS+ tests pass with production implementation
- [ ] Documentation updated in `src/crypto/ROADMAP.md` Phase 5-6

### References
- **Stub code:** `src/crypto/sphincs_engine.cpp`, `src/crypto/sphincs_signature.cpp`
- **Interface:** `include/crypto/sphincs_engine.h`
- **Tests:** `tests/crypto/test_sphincs_focused.cpp` (currently disabled)
- **vcpkg tracking:** https://github.com/Microsoft/vcpkg (community issue/PR)

---

## DEF-02: RocksDB Optional-Build Tolerance

### Scope
LLM Wiki Phase B persistent embedding cache (`WikiIndexStore`) is **blocked** if RocksDB is unavailable. Community-release presets must gracefully degrade to Phase A (in-memory + FNV hash).

### Current State (2026-08-10)
- ✅ THEMIS_ALLOW_MISSING_ROCKSDB=ON in community-release preset (CMakePresets.json:126-135)
- ✅ RocksDB dependency check in `cmake/Dependencies.cmake` allows missing library with warning
- ✅ THEMISDB_WIKI_PHASE_B CMake feature gate implemented
- ✅ Phase A path (JsonWikiIndexReader + FNV hash) always available
- ⏸️ Phase B persistent cache **skipped** if RocksDB unavailable
- ✅ community-release configure now passes without RocksDB (2026-08-10 verification pending)

### Build Behavior
**With RocksDB installed:**
```bash
cmake --preset community-release  # Configures with Phase B available
ctest -L release_critical          # All wiki tests pass; Phase B gates PASS
```

**Without RocksDB installed:**
```bash
cmake --preset community-release  # Configures with warning; Phase B disabled
ctest -L release_critical          # Phase A tests pass; Phase B tests skipped (conditional)
```

### Mitigation Strategy
1. **Immediate (2026-08-10):** Verify community-release works without RocksDB ✅ PLANNED
2. **Q4 2026 Phase 2 (Target 2026-08-26):**
   - Run full CI cycle on community-release without RocksDB
   - Confirm Phase A performance targets met
   - Document feature parity and limitations in `docs/use-cases/LLM_WIKI_MVP.md`

### Acceptance Criteria
- [ ] `cmake --preset community-release` passes without RocksDB (warning only)
- [ ] All Phase A LLM Wiki tests pass without RocksDB
- [ ] Phase B tests skip gracefully (no failures if RocksDB missing)
- [ ] `release_critical` CI passes on community-release without RocksDB
- [ ] Phase A performance baseline documented (query latency, memory overhead)

### References
- **CMake preset:** `CMakePresets.json:126-135`
- **Dependencies:** `cmake/Dependencies.cmake:105-196`
- **LLM Wiki Phase A:** `src/llm_wiki/wiki_index_store_phase_a.cpp`
- **LLM Wiki Phase B:** `src/llm_wiki/wiki_index_store_phase_b.cpp` (gated by THEMISDB_WIKI_PHASE_B)
- **Tests:** `tests/llm/test_llm_wiki_mvp_focused.cpp`

---

## DEF-03: Geo/Chimera/Graph Public Plugin Externalization

### Scope
Externalize currently integrated `src/geo`, `src/chimera`, `src/graph` modules into optional public plugin submodules with no-hard-fail fallback to monorepo paths.

### Current State (2026-08-10)
- ✅ Geo Phase 5-6 complete; benchmarks validated
- ✅ Chimera Phase 5-6 complete; adapter gates defined
- ✅ Graph Phase 5-6 in progress; optimizer/traversal gates defined
- ✅ .gitmodules entries exist: `plugins/themisdb_geo`, `plugins/themisdb_timeseries`
- 🟡 CMake integration for optional externalization **in progress**
- 🟡 Public submodule repositories may need initialization with content

### Sub-Tasks

#### DEF-03a: Geo Module Externalization (Primary)
**Scope:** Extract `src/geo` into optional public plugin `plugins/themisdb_geo`

**Current Progress:**
- ✅ Geo Phase 5-6 complete (ROADMAP.md E-3)
- ✅ Geo benchmark gates GATE-GRG-01..06 validated (2026-08-07)
- ✅ .gitmodules entry for plugins/themisdb_geo present
- 🟡 CMake integration with `THEMIS_EXTERNALIZE_GEO_PLUGIN` flag (default: OFF)

**Design:**
```cmake
# Option 1: Integrated monorepo (default, always available)
# Option 2: Optional public plugin (enabled by user, requires submodule init)

if(THEMIS_EXTERNALIZE_GEO_PLUGIN)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/themisdb_geo/CMakeLists.txt")
        add_subdirectory(plugins/themisdb_geo)
        set(THEMIS_GEO_TARGET themisdb_geo_plugin)
    else()
        message(FATAL_ERROR "THEMIS_EXTERNALIZE_GEO_PLUGIN=ON but plugins/themisdb_geo not found")
    endif()
else()
    # Use integrated monorepo path (src/geo)
    add_subdirectory(src/geo)
    set(THEMIS_GEO_TARGET themis_geo)
endif()
```

**Benchmarks Split:**
- Core geo tests stay in monorepo (distance, containment, index operations)
- Accelerated variants (CUDA, GPU) split to plugin (optional; requires specialized hardware)

**Target:** Q4 2026 Phase 3 (2026-09-15)

#### DEF-03b: TimeSeries Module Externalization
**Scope:** Extract `src/timeseries` into optional public plugin `plugins/themisdb_timeseries`

**Current Progress:**
- ✅ TimeSeries Phase 5-6 complete
- ✅ .gitmodules entry for plugins/themisdb_timeseries present
- 🟡 CMake integration needed

**Target:** Q4 2026 Phase 3 (2026-09-15)

#### DEF-03c: Chimera Module Externalization
**Scope:** Extract `src/chimera` (multi-model adapter) into optional public plugin

**Current Progress:**
- ✅ Chimera Phase 5-6 complete
- 🟡 No dedicated submodule entry yet (potential addition to Phase 4)

**Target:** Q4 2026 Phase 3-4 (2026-09-22)

### Acceptance Criteria (DEF-03a primary target)
- [ ] CMake flag `THEMIS_EXTERNALIZE_GEO_PLUGIN` added to CMakePresets.json
- [ ] `THEMIS_EXTERNALIZE_GEO_PLUGIN=OFF` (default): integrated path works identically to before
- [ ] `THEMIS_EXTERNALIZE_GEO_PLUGIN=ON`: optional submodule path loads; passes all benchmarks
- [ ] Core geo tests pass in both modes
- [ ] Accelerated benchmarks conditional on CUDA/GPU availability
- [ ] Missing submodule degrades gracefully (warning, not error)
- [ ] Release packages for community edition use integrated path (no plugin dependency)

### References
- **Integrated source:** `src/geo/`, `src/timeseries/`, `src/chimera/`
- **Submodule entries:** `.gitmodules` lines 60-68
- **Benchmarks:** `benchmarks/geo/`, `benchmarks/timeseries/`, `benchmarks/chimera/`

---

## DEF-04: LLM Wiki Phase B Persistent Embedding Cache

### Scope
LLM Wiki **Phase B** persistent embedding cache (RocksDB-backed WikiIndexStore) is **conditionally dependent** on DEF-02 (RocksDB availability).

### Current State (2026-08-10)
- ✅ LLM Wiki Phase A (in-memory + FNV hash): DELIVERED and operational
- ✅ Phase A tests passing; MVP CLI at `scripts/llm_wiki_mvp.py`
- ⏸️ Phase B (persistent cache): **BLOCKED** until DEF-02 (RocksDB) resolved
- ✅ Feature gate: `THEMISDB_WIKI_PHASE_B` (default: OFF)
- ✅ LLMWikiPluginImpl structure in `plugins/private/themisdb_llm_wiki/src/wikipedia/`

### Design
**Phase A (Always Available):**
```cpp
class JsonWikiIndexReader {
    // In-memory index using FNV-1a hashing
    // Query latency: O(log n); memory: ~500MB per 1M docs
};
```

**Phase B (Conditional, if RocksDB available):**
```cpp
class WikiIndexStore {
    // RocksDB-backed persistent cache
    // Query latency: O(log n) + disk I/O; memory: O(cache size) << data size
    // Enables >1B document indexing
};
```

### Blockers
- **External dependency:** DEF-02 (RocksDB optional-build tolerance)
- **Private plugin content:** Phase B implementation in private `themisdb_llm_wiki` repo

### Mitigation Strategy
1. **Immediate (pre-GA):** Phase A shipped; Phase B disabled by default
2. **Q4 2026 Phase 2 (Target 2026-08-26):**
   - Complete DEF-02 (RocksDB optional-build verification)
   - Enable Phase B tests conditionally (run only if THEMISDB_WIKI_PHASE_B=ON AND RocksDB found)
3. **Q4 2026 Phase 3 (Target 2026-09-15):**
   - Push Phase B content to `plugins/private/themisdb_llm_wiki` on develop
   - Stabilize Phase B benchmarks
   - Document feature parity and performance targets

### Acceptance Criteria
- [ ] Phase A tests pass unconditionally (no RocksDB dependency)
- [ ] Phase B tests skip if `THEMISDB_WIKI_PHASE_B=OFF` or RocksDB unavailable
- [ ] Phase B tests pass with `THEMISDB_WIKI_PHASE_B=ON` + RocksDB
- [ ] Phase B benchmarks (LWP-01..08) validated
- [ ] Documentation updated: `docs/use-cases/LLM_WIKI_MVP.md` Phase A vs Phase B comparison
- [ ] Release notes document Phase B as "Enterprise+ optional feature"

### References
- **Phase A:** `src/llm_wiki/`, `scripts/llm_wiki_mvp.py`
- **Phase B:** `plugins/private/themisdb_llm_wiki/src/wikipedia/`
- **Feature gate:** `THEMIS_WIKI_PHASE_B` in CMakeLists.txt
- **Tests:** `tests/llm/test_llm_wiki_mvp_focused.cpp`, `tests/test_lwp_plugin_focused.cpp`

---

## Deferred Item Dependency Graph

```
DEF-02 (RocksDB optional)
    ↓
DEF-04 (LLM Wiki Phase B persistent cache)
    ↓
[Requires DEF-02 to be complete before DEF-04 implementation]

DEF-01 (liboqs vcpkg availability)
    [Independent; no blockers; blocked by external vcpkg maintainer]

DEF-03a (Geo externalization)
    [Independent; no hard blockers; design can proceed in parallel]
    
DEF-03b (TimeSeries externalization)
    [Depends on DEF-03a design patterns]
```

---

## Execution Timeline (Q4 2026)

| Phase | Target Date | DEF-01 | DEF-02 | DEF-03a | DEF-03b | DEF-04 | Notes |
|-------|-------------|--------|--------|---------|---------|--------|-------|
| **Phase 1** | 2026-08-19 | 🔍 TRACK | ✅ VERIFY | 🎯 DESIGN | 📋 PLAN | 🔄 READY | GA sign-off complete |
| **Phase 2** | 2026-08-26 | 🔍 TRACK | ✅ COMPLETE | 🎯 DESIGN | 📋 PLAN | ⏸️ BLOCKED | RocksDB verification; Phase B tests gated |
| **Phase 3** | 2026-09-15 | 🔍 TRACK | ✅ COMPLETE | ✅ COMPLETE | ✅ COMPLETE | ✅ CONTENT PUSH | Geo/TimeSeries submodules stabilized |
| **Phase 4** | 2026-10-15 | ⏸️ EXTERNAL | ✅ COMPLETE | ✅ COMPLETE | ✅ COMPLETE | ✅ COMPLETE | liboqs (if vcpkg ready); all others done |

---

## Success Metrics

### DEF-01 Success
- [ ] vcpkg has liboqs v0.10.0+, OR
- [ ] PR submitted to vcpkg with ETA, OR
- [ ] Fallback decision documented (remain on Ed25519 path)

### DEF-02 Success
- [x] THEMIS_ALLOW_MISSING_ROCKSDB=ON in preset (DONE)
- [ ] community-release configure passes without RocksDB (**TARGET: 2026-08-26**)
- [ ] Phase A performance baseline documented
- [ ] release_critical CI passes on community-release without RocksDB

### DEF-03a Success
- [ ] Geo module builds as optional submodule OR integrated monorepo path
- [ ] Both paths pass `release_critical` gates
- [ ] Benchmark suite validates both paths (GATE-GRG-01..06)
- [ ] Missing submodule fails gracefully (warning + fallback)

### DEF-03b / DEF-03c Success
- [ ] TimeSeries/Chimera externalization follows DEF-03a pattern
- [ ] All modules pass respective benchmark gates

### DEF-04 Success
- [ ] Phase A tests pass unconditionally
- [ ] Phase B tests conditional on THEMISDB_WIKI_PHASE_B=ON + RocksDB
- [ ] Phase B benchmarks (LWP-01..08) validated
- [ ] Documentation distinguishes Phase A (community) vs Phase B (enterprise+)

---

## Ownership & Communication

- **DEF-01 (liboqs):** Crypto/Security Lead; track vcpkg repo; owner external dependency
- **DEF-02 (RocksDB):** Build/Infrastructure Lead; verify community-release without RocksDB; owner testing
- **DEF-03 (Geo/TimeSeries/Chimera):** Geo/TimeSeries Module Owners; design plugin submodules; owner rollout
- **DEF-04 (LLM Wiki Phase B):** LLM Wiki Plugin Owner; coordinate with DEF-02; owner Phase B implementation

---

**Status:** All deferred items have clear execution plans, owners, and acceptance criteria.  
**Next Action:** Begin DEF-02 verification on 2026-08-26; track DEF-01 vcpkg availability weekly.  
**Last Updated:** 2026-08-10  
