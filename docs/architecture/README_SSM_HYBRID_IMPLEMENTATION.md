/**
 * @file README_SSM_HYBRID_IMPLEMENTATION.md
 * @brief Quick Start Guide: SSM/Hybrid/Infini-attention Implementation Status
 * @version 0.1.0
 * @date 2026-07-22
 */

# SSM/Hybrid/Infini-attention Implementation — Quick Start

**Status:** Phase 1 Complete ✅ | Phase 2 Ready 📋  
**Branch:** develop  
**Last Updated:** 2026-07-22

---

## What's Implemented? ✅

### Documentation (2,055 lines)
- ✅ **Architecture Analysis** (`docs/architecture/ssm-hybrid-analysis.md` — 991 L)
  - Repository inventory with 27 file references
  - 7 gap proposals (GAP-SSM-01..07, GAP-FA3-01, GAP-QUANT-01)
  - Decision tables for architecture options
  
- ✅ **Rollout Plan** (`docs/architecture/ssm-hybrid-rollout-plan.md` — 523 L)
  - Phase 0–4 timeline (Q3/2026–Q2/2027)
  - Feature flag strategy
  
- ✅ **Phase 1 Design Review** (`docs/architecture/P1_D01_ISSMPLUGIN_DESIGN_REVIEW.md` — 234 L)
  - ISSMPlugin interface specification
  
- ✅ **Governance Contract** (`docs/architecture/P1_D08_MAMBA_GOVERNANCE_CONTRACT.md` — 302 L)
  - System-of-record principle
  - Security boundaries & multi-tenant isolation

### Implementation (1,683 lines of production code)

**Phase 1 Deliverables:**
- ✅ `include/llm/ssm_state_store.h` (153 L) — State persistence interface
- ✅ `src/llm/ssm_state_store.cpp` (132 L) — In-memory implementation
- ✅ `include/llm/ssm_stub_plugin.h` (80 L) — Synthetic test plugin
- ✅ `src/llm/ssm_stub_plugin.cpp` (140 L)
- ✅ `include/llm/infini_attention_cpu.h` (136 L) — CPU fallback kernel
- ✅ `src/llm/attention/infini_attention_cpu.cpp` (257 L)
- ✅ `include/llm/ssm_drift_metrics.h` (79 L) — Observability metrics
- ✅ `src/llm/ssm_drift_metrics.cpp` (118 L)
- ✅ `include/llm/context_quality_metrics.h` (91 L) — Agentic Memory layer tracking

**Configuration Ready (Phase 2):**
- ✅ `include/llm/paged_kv_cache.h` — Extended with `kv_quantization_bits`
- ✅ `src/llm/paged_kv_cache.cpp` — Helper: `getBitWidthForQuantizationType()`

### Tests (445 lines + auto-registration)

- ✅ `tests/llm/test_ssm_plugin_interface.cpp` — 445 L, 8 acceptance gates
- ✅ `tests/llm/test_infini_attention.cpp` — Infini-attention correctness
- ✅ `tests/llm/test_ssm_rocksdb_store.cpp` — Phase 2 preview
- ✅ `tests/llm/test_nvfp4_kv_quantization.cpp` — Phase 2 KV-cache tests

**Auto-Registration:** All tests registered in `tests/llm/CMakeLists.txt` via GLOB pattern

### Benchmarks (370 lines + baseline config)

- ✅ `benchmarks/bench_ssm_phase0_baseline.cpp` — NEW Phase 0 baseline measurement
  - CPU Infini-attention synthetic latency
  - SSM checkpoint/resume cycle
  - Context quality metrics
  - Drift metrics Prometheus export
  
- ✅ `benchmarks/ssm_baseline/baseline_phase0.json` — Execution contract
- ✅ Registered in `benchmarks/CMakeLists.txt` via macro

---

## What's NOT Implemented Yet? 📋

### Phase 2 Implementation (Q4/2026)
- ⏳ NVFP4 KV-cache quantization runtime (config ready, runtime deferred)
- ⏳ CUDA Infini-attention kernel for SM90
- ⏳ RocksDB-backed SSMStateStore (in-memory done)
- ⏳ Episodic L2 compaction

### Phase 3+ Features (Q1/2027+)
- ⏳ HybridContextRouter
- ⏳ Full Agentic Memory L1+L2+L3 rotation
- ⏳ Distributed SSM state management

---

## How to Use? 🚀

### 1. Run Phase 1 Tests

```bash
# Build tests (requires CMake + vcpkg)
cmake --preset linux-release -DTHEMIS_BUILD_TESTS=ON
cmake --build --preset linux-release --target module_llm_test_ssm_plugin_interface_focused

# Run specific test
ctest -L unit -R "test_ssm_plugin_interface" --output-on-failure

# Expected: All 8 P1 acceptance gates PASS
```

### 2. Collect Phase 0 Baseline

```bash
# Build benchmark
cmake --preset linux-release -DTHEMIS_BUILD_BENCHMARKS=ON
cmake --build --preset linux-release --target bench_ssm_phase0_baseline

# Run and collect metrics
./bench_ssm_phase0_baseline \
  --benchmark_out=benchmarks/ssm_baseline/baseline_phase0.json \
  --benchmark_out_format=json

# View results
cat benchmarks/ssm_baseline/baseline_phase0.json | jq '.context'
```

### 3. Review Architecture

```bash
# Main analysis with gaps & decision tables
cat docs/architecture/ssm-hybrid-analysis.md

# Implementation plan (Phase 0–4)
cat docs/architecture/ssm-hybrid-rollout-plan.md

# Governance contract (security/compliance binding)
cat docs/architecture/P1_D08_MAMBA_GOVERNANCE_CONTRACT.md
```

---

## What's Blocking Phase 2? 🚫

**Gate: P1-GATE-07** — Governance Contract Sign-off

The Mamba Governance Contract (`P1_D08_MAMBA_GOVERNANCE_CONTRACT.md`) requires human approval:
- [ ] Architecture review — System-of-record principle
- [ ] Security review — Multi-tenant isolation guarantees
- [ ] Operations review — State lifecycle enforcement
- [ ] Testing sign-off — Phase 1 unit test acceptance

**Once approved:** Phase 2 can proceed immediately (all dependencies ready).

---

## File Organization

```
docs/architecture/
├── ssm-hybrid-analysis.md ........................ Main analysis (991 L)
├── ssm-hybrid-rollout-plan.md ................... Rollout phases (523 L)
├── P1_D01_ISSMPLUGIN_DESIGN_REVIEW.md .......... Design review (234 L)
├── P1_D08_MAMBA_GOVERNANCE_CONTRACT.md ........ Governance [BLOCKING] (302 L)
├── P1_P2_IMPLEMENTATION_COMPLETION_INDEX.md ... Full status (this doc)
└── README_SSM_HYBRID_IMPLEMENTATION.md ........ Quick start (THIS FILE)

include/llm/
├── i_ssm_plugin.h ............................. Abstract interface
├── ssm_state_store.h .......................... State persistence
├── ssm_stub_plugin.h .......................... Synthetic test impl
├── infini_attention_cpu.h ..................... CPU fallback
├── ssm_drift_metrics.h ........................ Prometheus metrics
└── context_quality_metrics.h .................. Agentic Memory tracking

src/llm/
├── ssm_state_store.cpp
├── ssm_stub_plugin.cpp
├── ssm_drift_metrics.cpp
└── attention/infini_attention_cpu.cpp

tests/llm/
├── test_ssm_plugin_interface.cpp ............. Phase 1 comprehensive (445 L)
├── test_infini_attention.cpp
├── test_ssm_rocksdb_store.cpp
└── test_nvfp4_kv_quantization.cpp

benchmarks/
├── bench_ssm_phase0_baseline.cpp ............. Phase 0 baseline (370 L) [NEW]
└── ssm_baseline/baseline_phase0.json ......... Execution contract
```

---

## Glossary

| Term | Meaning |
|------|---------|
| **SSM** | State Space Model (Mamba architecture) |
| **Infini** | Infini-attention: associative memory + linear attention |
| **FA3** | FlashAttention-3 with FP8 and grouped-query optimization |
| **NVFP4** | NVIDIA 4-bit floating-point KV-cache quantization (2× compression) |
| **L1/L2/L3** | Agentic Memory layers: Working / Episodic / Semantic |
| **HLC** | Hybrid Logical Clock for MVCC snapshot isolation |
| **P0–P4** | Phases 0–4 of rollout plan |
| **P1-GATE-07** | Blocking gate: Governance contract sign-off |

---

## Next Steps

1. **Approve P1-GATE-07** (5 min) → Unblocks Phase 2
2. **Run Phase 1 tests** (2 min) → Verify acceptance
3. **Collect Phase 0 baseline** (5 min) → Establish measurement baseline

---

**For more details, see:** `P1_P2_IMPLEMENTATION_COMPLETION_INDEX.md`
