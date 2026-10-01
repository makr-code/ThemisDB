# Wave B Option B1: LLM Wiki Phase B — Representative-Hardware Validation & Benchmark Sign-Off
## Status: ✅ READY FOR EXECUTION

---

## Executive Summary

**Module:** LLM Wiki (RAG retrieval orchestration with persistence)  
**Phase:** Phase B hardening (representative-hardware validation + benchmark sign-off)  
**Timeline:** Sept 16-30, 2026 (15 days)  
**Gate:** Wave B Exit Criteria item 3 — "Release decisions based on representative hardware baselines"  
**Target Hardware:** A100 / RTX 4090 / H100 (GPU acceleration for vector search + reranking)  
**Success Metric:** Locked p95/p99 latency + memory guarantees across full 4-layer retrieval chain

---

## Current State (2026-09-02 Baseline)

### Codebase Status
- ✅ **Phase A Complete:** BM25+HNSW+RRF all implemented (2026-08-26)
- ✅ **Phase B Codebase Complete:** RocksDB backend wiring + persistence round-trip verified (2026-08-26)
- ✅ **In-memory Fallback:** Retained for test environments
- ⚠️ **Representative-Hardware Baselines:** PENDING (marked Q4 2026 in ROADMAP)

### Test Coverage
- 40+ persistence/roundtrip tests passing locally (CPU-only)
- Benchmark suite exists: `benchmarks/llm_wiki/`
- Coverage: BM25 scoring, HNSW index rebuild, RRF fusion, persistence I/O

### Known Limitations
- Current benchmarks run on CPU/local disk (scaffolding environment)
- No GPU-accelerated vector search baselines captured
- No memory profile data under sustained 1M+ vector load
- No p95/p99 data on representative hardware

---

## Wave B Exit Criteria & Blockers

**Exit Criteria (from ROADMAP line 113):**
- [x] Full 4-layer retrieval chain has stable p95/p99 and bounded memory on representative hardware
- [~] Release decisions based on representative hardware baselines, not module-local-only scaffolding benchmarks

**Blocker Status:**
- ⚠️ **Hardware Access:** A100 / H100 / RTX-class GPU required (not available in Ubuntu build environment)
  - **Mitigation:** Coordinate with infrastructure team to provision hardware CI runner by Sept 7
  - **Alternative:** If hardware unavailable Sept 16, defer to Oct 1 with hybrid CPU mock baseline

---

## Execution Plan (Sept 16-30)

### Phase 1: Baseline Measurement Protocol Design (Sept 16-17, 2 days)

**Objective:** Define measurement methodology + baseline capture procedures

**Tasks:**
1. **Hardware Target Specification** (Sept 16, 09:00 UTC)
   - Confirm available GPU hardware (A100, H100, or RTX 4090)
   - Document GPU memory capacity, compute capability, thermal limits
   - Establish warm-up + cooldown procedures (avoid thermal throttling)

2. **Benchmark Scenario Design** (Sept 16, 14:00 UTC)
   - Scenario 1: Small-scale (10K vectors, d=768)
     - BM25 query latency ≤50ms
     - HNSW neighbor search ≤20ms
     - RRF fusion ≤10ms
     - Persistence I/O (write) ≤100ms
   
   - Scenario 2: Medium-scale (1M vectors, d=768)
     - BM25 batch (1K tokens) ≤500ms
     - HNSW approximate NN (k=10) ≤150ms
     - RRF fusion ≤50ms
     - Cache hit rate ≥95% after 100K queries
   
   - Scenario 3: Large-scale (10M vectors, d=1536, half-precision)
     - Memory footprint ≤16GB GPU + ≤32GB system RAM
     - BM25 throughput ≥5K queries/sec
     - Persistence write throughput ≥1K index updates/sec
     - p99 latency ≤500ms under concurrent load (32 threads)

3. **Measurement Tools Setup** (Sept 17, 09:00 UTC)
   - Perf counters: NVIDIA NVML (GPU utilization, memory, thermal)
   - Latency sampling: Chrono high-resolution timers + percentile histograms
   - Memory profiling: GPU compute-sanitizer + system malloc hooks
   - Output format: JSON baseline manifest (reproducible, versioned)

### Phase 2: Benchmark Execution (Sept 18-24, 7 days)

**Objective:** Execute benchmarks on representative hardware + capture baseline

**Tasks:**

1. **Setup CI Environment** (Sept 18, 09:00 UTC, 4 hours)
   - Provision hardware CI runner (GitHub self-hosted or cloud GPU)
   - Install CUDA toolkit + cuDNN (if needed for vector search acceleration)
   - Compile LLM Wiki tests + benchmarks with GPU optimization flags
   - Smoke test: 1 scenario run (verify no crashes, baseline output format)

2. **Scenario 1: Small-scale Baseline** (Sept 18-19, 2 days)
   - Execute 10 trial runs (BM25 + HNSW + RRF + persistence)
   - Capture p50, p95, p99 latencies per operation
   - Measure GPU memory peak + sustained average
   - Generate baseline JSON: `benchmarks/llm_wiki/BASELINE_SMALL_2026_09_18.json`
   - Expected output: Scenario 1 targets locked ✅ (or flagged if exceeds)

3. **Scenario 2: Medium-scale Baseline** (Sept 20-22, 3 days)
   - Execute 5-10 trial runs (1M vector index)
   - Cold start: Clean cache, rebuild index from RocksDB (~30 sec load)
   - Warm cache: 100K query warm-up, then 50K measurement queries
   - Cache hit rate validation (target ≥95%)
   - Generate baseline JSON: `benchmarks/llm_wiki/BASELINE_MEDIUM_2026_09_20.json`

4. **Scenario 3: Large-scale Stability** (Sept 23-24, 2 days)
   - Execute 3-5 trial runs (10M vectors, half-precision)
   - Sustained concurrent load test (32 threads for 60 min)
   - Memory leak detection (valgrind or CUDA compute-sanitizer)
   - Thermal stability (GPU core temp < 80°C, no throttling)
   - Generate baseline JSON: `benchmarks/llm_wiki/BASELINE_LARGE_2026_09_23.json`

5. **Fallback Option: CPU Mock (if hardware unavailable Sept 16)**
   - Use pre-recorded GPU timings (from Wave 5 archive if available)
   - Simulate GPU memory constraints via malloc hooks
   - Document deviation from hardware baseline in report
   - Flag as "CPU-mock baseline; defer hardware validation to Oct 1"

### Phase 3: Analysis & Evidence Documentation (Sept 25-28, 4 days)

**Objective:** Consolidate baseline data + validate against Wave B exit criteria

**Tasks:**

1. **Baseline Analysis** (Sept 25, 09:00 UTC)
   - Read all 3 baseline JSON files
   - Compute aggregate statistics (mean, median, p95, p99 per scenario)
   - Identify outliers (thermal throttling, OS jitter, GC pauses)
   - Cross-check against expected targets from Phase 1 design

2. **Memory Profile Validation** (Sept 25, 14:00 UTC)
   - Verify memory footprint within 16GB GPU + 32GB system for Scenario 3
   - Identify memory growth (leak detection)
   - Compare in-memory cache size vs. persistent RocksDB backend ratio
   - Document any memory pressure events

3. **Regression Detection** (Sept 26, 09:00 UTC)
   - Compare against Wave A baseline (if exists): `benchmarks/wave7/...`
   - Flag any >10% performance regression (investigate + fix)
   - Document deviations (expected tuning, expected hardware differences)

4. **Wave B Exit Criteria Validation** (Sept 26-27)
   - ✅ Exit Criteria item 1: "Full 4-layer retrieval chain has stable p95/p99 and bounded memory on representative hardware"
     - BM25 ✅ p95 ≤500ms (target scenario 2)
     - HNSW ✅ p95 ≤150ms (target scenario 2)
     - RRF ✅ p95 ≤50ms
     - Persistence I/O ✅ write p95 ≤100ms
     - Memory ✅ bounded (16GB GPU / 32GB system max)
   
   - ✅ Exit Criteria item 3: "Release decisions based on representative hardware baselines"
     - All 3 scenarios locked with hardware evidence
     - JSON baselines versioned + committed
     - CI integration verified (benchmarks run green)

5. **Documentation** (Sept 27-28)
   - Generate final report: `LLM_WIKI_PHASE_B_HARDWARE_VALIDATION_2026_09_28.md`
     - Section 1: Executive summary (baseline status, pass/fail per scenario)
     - Section 2: Hardware profile (GPU model, memory, thermal limits)
     - Section 3: Measurement methodology (procedure, tools, calibration)
     - Section 4: Baseline results (latency/memory per scenario, p50/p95/p99)
     - Section 5: Analysis (regressions, tuning recommendations, deviations)
     - Section 6: Wave B exit criteria validation (PASS / NEEDS WORK)
   - Generate evidence bundle: `src/llm_wiki/WAVE_B_HARDWARE_VALIDATION_EVIDENCE_2026_09_28.md`

### Phase 4: CI Integration & Sign-Off (Sept 29-30, 2 days)

**Objective:** Integrate benchmarks into CI + Wave B exit criteria sign-off

**Tasks:**

1. **CI Workflow Integration** (Sept 29, 09:00 UTC)
   - Create GitHub Actions workflow: `bench-llm-wiki-hardware.yml`
     - Trigger: Manual (on-demand) or scheduled weekly (after Sept 28)
     - Runner: self-hosted GPU (or cloud GPU provider)
     - Baseline comparison: Read stored baseline JSON, fail if >10% regression
     - Output artifact: Benchmark results JSON + latency histogram graphs
   
   - Add benchmark gate: `.github/workflows/09-pr-gates_release-critical-tests.yml`
     - Condition: If LLM Wiki code changed + GPU available, run baseline comparison
     - Pass condition: All scenarios ≤ baseline + 5% margin

2. **Documentation Sync** (Sept 29, 14:00 UTC)
   - Update ROADMAP.md line 108: Change `[~]` to `[x]` (Phase B complete)
   - Update Wave B Exit Criteria line 113: Mark criteria 1 as ✅ (hardware baselines locked)
   - Add link to hardware validation evidence bundle
   - Commit with message: "Wave B Option B1: LLM Wiki representative-hardware validation complete"

3. **Final Sign-Off** (Sept 30, 14:00 UTC)
   - GPU module owner reviews baseline report + CI workflow
   - Approval: All scenarios within expected range, CI gates operational
   - Final artifact: `docs/governance/WAVE_B_LLM_WIKI_SIGN_OFF_2026_09_30.md`
   - Status: ✅ Wave B Exit Criteria #1 COMPLETE

---

## Resource Allocation

**Team:** 1.5 FTE (Sept 16-30)
- **Benchmark Engineer** (1 FTE): Measurement design, execution, analysis
- **DevOps/CI** (0.5 FTE): Hardware provisioning, CI workflow setup, artifact management

**Hardware Requirements:**
- A100 / H100 / RTX 4090 GPU (8-24GB VRAM)
- Self-hosted CI runner or cloud GPU provider (AWS EC2 g4dn, GCP a100, Azure NC)
- 32GB system RAM, 500GB SSD for index data
- Estimated cost: $200-500 (if using cloud provider)

**Time Breakdown:**
- Protocol design: 8 hours (Sept 16-17)
- Benchmark execution: 56 hours (Sept 18-24, ~8 hours/day)
- Analysis + documentation: 32 hours (Sept 25-28)
- CI integration + sign-off: 16 hours (Sept 29-30)
- **Total:** 112 hours (14 FTE-days)

---

## Success Criteria (Sept 30, 18:00 UTC)

✅ **All must be true:**
- [ ] Hardware provisioned: A100 / H100 / RTX-class GPU available Sept 16
- [ ] Baseline protocol defined: 3 scenarios (small/medium/large) with measurement tooling locked
- [ ] Scenario 1 baseline captured: ≤50ms (BM25), ≤20ms (HNSW), write p95 ≤100ms
- [ ] Scenario 2 baseline captured: ≤500ms (BM25 batch), ≤150ms (HNSW), cache ≥95% hit
- [ ] Scenario 3 baseline captured: ≤16GB GPU + 32GB RAM, ≥5K queries/sec throughput
- [ ] Wave B Exit Criteria #1 validated: p95/p99 stable, memory bounded ✅
- [ ] Evidence bundle generated: `WAVE_B_HARDWARE_VALIDATION_EVIDENCE_2026_09_28.md`
- [ ] CI workflow operational: GPU baseline comparison gates configured + green
- [ ] ROADMAP.md updated: Wave B LLM Wiki marked complete [x]

---

## Known Risks & Mitigations

| Risk | Probability | Impact | Mitigation |
|---|---|---|---|
| GPU hardware unavailable Sept 16 | MEDIUM | HIGH | Defer to Oct 1; use CPU mock baseline if needed |
| Thermal throttling invalidates measurements | LOW | MEDIUM | Establish cooldown procedures; re-run if throttling detected |
| Benchmark tools not available on GPU | LOW | MEDIUM | Pre-test tools on Aug 31; install NVIDIA profiler toolchain Sept 7 |
| Performance regression >10% | MEDIUM | HIGH | Investigate tuning opportunities; document expected deviations |
| RocksDB persistence shows unexpected latency | MEDIUM | MEDIUM | Profile I/O path; consider compression/caching tuning Sept 28 |
| CI runner resource contention | LOW | MEDIUM | Schedule benchmarks off-peak (weekends); use dedicated runner |

---

## Next Steps

**Pre-Execution (Sept 2-15):**
- [ ] Request GPU hardware provisioning from infrastructure team (target: ready by Sept 7)
- [ ] Verify benchmark suite compiles with GPU optimization flags
- [ ] Pre-test NVIDIA profiler tools + latency measurement infrastructure
- [ ] Create GitHub self-hosted runner configuration (or select cloud provider)

**Execution (Sept 16-30):**
- [ ] Sept 16-17: Protocol design complete, 3 scenarios locked
- [ ] Sept 18-24: All 3 baseline scenarios executed
- [ ] Sept 25-28: Analysis + evidence bundle generated
- [ ] Sept 29-30: CI integration + sign-off

**Post-Execution (Oct 1+):**
- [ ] Weekly performance validation (compare new runs to baseline, alert if >5% regression)
- [ ] Repeat hardware validation on release branches before v2.4.0 GA (target: Oct 15)

---

## Document Status

**Prepared By:** Copilot Coding Agent  
**Date:** Sept 2, 2026, 14:35 UTC  
**Session:** Wave B Planning — Option B1 (LLM Wiki Hardware Validation)  
**Gate:** Wave B Exit Criteria #1 (Representative-hardware p95/p99 baseline + memory bounds)

**Status:** ✅ READY FOR EXECUTION (Sept 16, 2026, 09:00 UTC)

