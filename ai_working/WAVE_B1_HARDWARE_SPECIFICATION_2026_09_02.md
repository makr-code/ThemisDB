# Wave B Option B1: Hardware Specification for LLM Wiki Phase B Validation
## Representative-Hardware Validation & Benchmark Protocol Design

**Document ID:** WAVE_B1_HARDWARE_SPECIFICATION_2026_09_02  
**Phase:** Phase 1 (Baseline Protocol Design)  
**Milestone:** Sept 16 Readiness  
**Date:** 2026-09-02  
**Owner:** Platform Performance & Infrastructure  
**Status:** Draft Specification

---

## Executive Summary

This document specifies the representative hardware configuration, procurement strategy, measurement tooling, and validation methodology for Wave B Option B1 (Sept 16–30, 2026). The goal is to establish reproducible, hardware-representative performance baselines for LLM Wiki Phase B (RocksDB backend, BM25+, HNSW, RRF) and related Wave B modules (GPU, Transaction, Voice, Auth).

### Success Criteria (Gate: Sept 5)
- ✅ **Hardware specification signed off** — Target GPU models, fallback CPU baseline, thermal/power limits defined
- ✅ **Procurement request filed** — External GPU hardware (if needed) ordered with delivery target Sept 7
- ✅ **CPU mock baseline agreed** — Fallback measurement approach approved if hardware unavailable
- ✅ **NVML profiler integration plan** — Measurement tooling selections locked

### Primary Deliverables by Sept 16
1. Hardware procurement confirmation + delivery SLA
2. CUDA 12.x + NVIDIA NVML profiler environment validated
3. Latency histogram baseline + regression detection calibration
4. Memory profiling runbook

---

## 1. Target Hardware Specification

### 1.1 Primary GPU Targets

| Model | Specifications | Purpose | Priority |
|-------|----------------|---------|----------|
| **NVIDIA A100 40GB** | 40GB HBM2e, 432 SMs, 1410 MHz boost, 2TB/s bandwidth | Standard production baseline | P0 |
| **NVIDIA A100 80GB** | 80GB HBM2e, dual-rank HBM, 1410 MHz boost | Large-scale retrieval validation | P1 |
| **NVIDIA H100 80GB** | 80GB HBM3, 528 SMs, 2.2TB/s bandwidth, sparsity support | Hyperscaler target (optional) | P2 |
| **NVIDIA RTX 4090 24GB** | 24GB GDDR6X, consumer-grade FP32, no sparsity | Budget alternative (if available) | P3 |

### 1.2 CPU Fallback Baseline

**When GPU Hardware is Unavailable (After Sept 7):**
- **CPU:** Intel Xeon W9-3495X (60-core, 4.8 GHz all-core, 420W TDP) or equivalent
- **Purpose:** Single-threaded and multi-threaded CPU-only retrieval baseline
- **Scope:** Small (10K vectors) and Medium (1M vectors) scenarios only
- **Measurement:** Wall-clock latency; no CUDA/GPU metrics
- **Regression Detection:** CPU p95 +20% = RED (larger variance expected on CPU)

**Mock CPU Baseline Strategy:**
If even CPU hardware is unavailable, use CI runner hardware with documented configuration:
- **Fall-Back:** GitHub Actions standard runner (2 vCPU, ~7GB RAM)
- **Scope:** Micro-benchmark only (10K vectors, single query)
- **Validation Purpose:** Regression detection only, NOT performance targets
- **Documentation:** All results tagged `cpu_mock_baseline`

### 1.3 System Requirements (All Configurations)

| Component | Specification |
|-----------|---|
| **RAM** | Minimum 32GB; 64GB+ recommended |
| **Storage** | 500GB SSD (NVMe) for RocksDB benchmark data |
| **NVIDIA CUDA Toolkit** | 12.0+ (CUDA Compute Capability 7.0+) |
| **cuBLAS / cuDNN** | Latest stable (verify A100/H100/RTX 4090 support) |
| **Linux OS** | Ubuntu 22.04 LTS (matching CI standard) |
| **NVIDIA Driver** | 535+, validated for CUDA 12.x |
| **Thermal Management** | <80°C sustained operation; throttling detection enabled |

---

## 2. Measurement Methodology

### 2.1 Baseline Metrics (Wave 7 Reference)

From `benchmarks/wave7/release_gate_manifest_w7.json`:

| Metric | Unit | Purpose | Baseline |
|--------|------|---------|----------|
| **p50 latency** | µs / ms | Median response time | Tracked |
| **p95 latency** | µs / ms | Tail latency (production SLA) | Hard gate |
| **p99 latency** | µs / ms | Extreme tail (reliability) | Hard gate |
| **Throughput (ops/sec)** | items/sec | Sustained retrieval rate | Hard gate |
| **Memory Peak** | GB | Peak heap usage under load | Soft gate |
| **Cache Hit Ratio** | % | RocksDB page cache effectiveness | Tracked |

### 2.2 Profiling Tools & Integration

#### NVIDIA NVML (Primary GPU Profiler)
```
Tool: NVIDIA GPU Monitor Library (python3-pynvml or C++ cuda_runtime_api.h)
Metrics:
  - SM utilization (%)
  - GPU memory used / total (GB)
  - GPU power consumption (W)
  - Temperature (°C)
  - Clock throttling events (count)
Sampling Rate: 100ms (10 Hz)
Integration: Embedded in benchmark harness via `benchmarks/cmake/GpuProfiler.h`
```

#### CPU Profiler (perf + PAPI)
```
Tool: Linux perf (performance counters) + PAPI (Performance API)
Metrics:
  - CPU cycles
  - Cache L3 misses
  - Branch misses
  - CPU frequency scaling events
  - Context switches
Sampling: Per-benchmark, recorded to JSON alongside latency data
Integration: Via CMake target property `BENCHMARK_ENABLE_PERF_COUNTERS`
```

#### Memory Profiler
```
Tool: jemalloc (malloc hook) + custom allocation tracker
Purpose: Track RocksDB memory growth, peak heap, fragmentation
Integration: Link benchmark binaries with jemalloc; enable malloc profiling
Output: Heap dumps at 5-minute intervals during sustained load phase
```

### 2.3 Latency Histogram Methodology

**Histogram Collection:**
- **Bin Resolution:** 1µs (for latencies <1ms), 0.1ms (for latencies 1–100ms), 1ms (>100ms)
- **Percentile Extraction:** p50, p95, p99, p99.9, max
- **Outlier Detection:** Values > 3σ flagged separately (thermal throttle / GC events)

**Warmup & Steady-State Protocol (Defined in §3):**
- 5-minute warmup (cache priming, JIT compilation if applicable)
- 20-minute steady-state measurement (core benchmark window)
- Final 1-minute cooldown (capture cleanup overhead)

**Output Format (JSON):**
```json
{
  "benchmark_id": "LLM_WIKI_P0_SMALL_10K_VECTORS",
  "timestamp": "2026-09-16T14:30:00Z",
  "hardware_profile": "A100_40GB_ubuntu_22_04",
  "metrics": {
    "latency_ms": {
      "p50": 42.3,
      "p95": 156.7,
      "p99": 203.4,
      "p99_9": 248.1,
      "max": 312.5
    },
    "throughput_ops_sec": 5230,
    "gpu_memory_gb": 18.4,
    "gpu_power_avg_w": 215,
    "gpu_temp_max_c": 74,
    "throttle_events": 0
  }
}
```

### 2.4 Regression Detection Thresholds

| Metric | Yellow Threshold | Red Threshold | Rationale |
|--------|------------------|---------------|-----------|
| **p95 latency** | +5% vs baseline | +10% vs baseline | Catch early degradation |
| **p99 latency** | +7% vs baseline | +15% vs baseline | Tail-latency regressions |
| **Throughput** | -3% vs baseline | -10% vs baseline | Performance cliffs |
| **Memory Peak** | +10% vs baseline | >80% of system RAM | Avoid OOM in production |
| **Thermal** | Throttle detected | Continuous throttle | Thermal instability |

---

## 3. Validation Phases

### Phase A: Hardware Procurement & Setup (Sept 2–7)

| Milestone | Deliverable | Owner | Status |
|-----------|---|---|---|
| **Sept 2 (Today)** | Procurement request filed for GPU hardware (if external) | Infrastructure | [ ] To-Do |
| **Sept 4** | CI environment setup: CUDA 12.x, NVIDIA NVML, driver validation | CI/DevOps | [ ] To-Do |
| **Sept 5** | Hardware access confirmed (GPU or CPU mock baseline) | Infrastructure | [ ] Blocker if unmet |
| **Sept 7** | Fallback CPU baseline configured if GPU not available | Platform | [ ] Contingency |

### Phase B: Profiler Calibration (Sept 8–12)

| Milestone | Activity | Status |
|-----------|----------|--------|
| **Sept 8–9** | NVML / perf integration test; verify histogram collection | [ ] To-Do |
| **Sept 10** | Baseline reference run: capture p50/p95/p99 for Small/Medium scenarios | [ ] To-Do |
| **Sept 11** | Thermal stress test: validate <80°C operation, zero throttle events | [ ] To-Do |
| **Sept 12** | Memory profiler calibration: RocksDB peak heap validation | [ ] To-Do |

### Phase C: Benchmark Execution Readiness (Sept 13–16)

| Milestone | Activity | Status |
|-----------|----------|--------|
| **Sept 13** | CI workflow syntax check; manual-dispatch trigger operational | [ ] To-Do |
| **Sept 14** | Dry-run: Small scenario (10K vectors) end-to-end on representative hardware | [ ] To-Do |
| **Sept 15** | Regression detection gates calibrated; thresholds locked | [ ] To-Do |
| **Sept 16** | Baseline evidence bundle signed off; ready for Wave B execution (Sept 17–30) | [ ] Blocker for Phase 2 |

---

## 4. Risk Assessment & Mitigation

### Critical Risks

| Risk | Impact | Probability | Mitigation |
|------|--------|-------------|-----------|
| **GPU Hardware Unavailable by Sept 7** | Blocks representative-hardware validation | Medium (40%) | CPU mock baseline approved; fall-back CI runner as last resort |
| **Thermal Throttling Under Load** | p99 latency artificially inflated; invalid baseline | Low–Medium (20%) | Thermal management plan: throttle detection + adaptive load reduction |
| **CUDA 12.x Driver Incompatibility** | Build/runtime failures; delays milestone | Low (10%) | Early CI smoke test (Sept 4); driver update SLA defined |
| **RocksDB I/O Bottleneck** | SSD performance limit discovered late | Medium (35%) | Sept 10: benchmark SSD I/O independently; NVMe validation mandatory |
| **Regression Gate Thresholds Too Tight** | False positives; gate flakiness | Medium (30%) | Calibration run (Sept 10) + statistical validation (Sept 11–12) |

### Mitigation Checklist

- [x] **CPU Fallback Approved** — Contingency baseline signed off
- [ ] **Driver Update SLA** — Sept 4 deadline for CUDA 12.x validation
- [ ] **Thermal Stress Protocol** — Document max sustained temp + throttle reset procedure
- [ ] **RocksDB SSD Validation** — Separate I/O benchmark to isolate storage overhead
- [ ] **Gate Calibration Sign-Off** — Thresholds locked after Phase B completion (Sept 12)

---

## 5. Hardware Procurement & Access Plan

### 5.1 Procurement Options

#### Option A: External Cloud GPU (Recommended)
- **Provider:** AWS (p3.8xlarge with 4x A100), GCP (a100-80gb), or on-prem partnership
- **SLA:** Delivery by Sept 7 EOD
- **Cost:** Budget TBD by infrastructure
- **Upside:** Reproducible in any region; documented configuration

#### Option B: Internal Lab Hardware
- **Assumption:** Organization has on-premises GPU lab
- **Action:** Reserve A100/H100 slots for Sept 7–30 (non-blocking for Phase 1)
- **Coordination:** Schedule access via infrastructure team

#### Option C: Fallback CPU Baseline
- **Configuration:** Xeon W9-3495X or equivalent
- **Timeline:** Activate if GPU unavailable after Sept 5
- **Scope:** Small + Medium scenarios only; Large scenario deferred
- **Documentation:** Baseline tagged `cpu_baseline_20260916`

### 5.2 Acceptance Criteria

**Hardware Procurement Accepted When:**
1. ✅ Device accessible via CI/CD or secured lab environment
2. ✅ CUDA 12.x + driver installed and validated
3. ✅ NVML profiler operational (temperature, power, utilization reporting)
4. ✅ Sustained operation <80°C (no throttle events) for 30-minute baseline run
5. ✅ 500GB+ SSD available for RocksDB artifacts

---

## 6. Documentation & Runbooks

### 6.1 Hardware Validation Runbook

**File:** `docs/operability/WAVE_B_HARDWARE_VALIDATION_RUNBOOK.md`

**Sections:**
1. CUDA/Driver setup & validation
2. NVML profiler check
3. Thermal baseline validation
4. RocksDB SSD I/O calibration
5. Regression gate threshold tuning

### 6.2 CI Workflow Integration

**File:** `.github/workflows/13-wave-b-llm-wiki-benchmarks.yml`

**Responsibilities:**
- Hardware detection (GPU / CPU fallback)
- Profiler activation & verification
- Benchmark execution with result collection
- Regression gate evaluation
- Artifact storage (latency histograms, memory profiles)

---

## 7. Execution Checklist (Sept 2–16)

### By Sept 5 (Sign-Off Gate)
- [ ] Hardware specification locked (this document approved)
- [ ] Procurement request filed (GPU or CPU baseline confirmed)
- [ ] Fallback CPU strategy signed off by leadership
- [ ] NVML profiler integration plan documented

### By Sept 7 (Hardware Delivery)
- [ ] GPU hardware accessible OR CPU fallback ready
- [ ] CUDA 12.x + drivers installed & validated
- [ ] Thermal management baseline established

### By Sept 12 (Profiler Calibration)
- [ ] NVML / perf / jemalloc integration operational
- [ ] p50/p95/p99 baseline captured for Small scenario
- [ ] Regression detection gates calibrated

### By Sept 16 (Phase 1 Completion)
- [ ] CI workflow operational & syntax validated
- [ ] Dry-run (Small scenario) completed successfully
- [ ] Hardware readiness evidence bundle finalized
- [ ] Ready to proceed with Phase 2 (Sept 17–30 benchmark execution)

---

## 8. Success Criteria & Sign-Off

**Phase 1 Complete When:**
✅ Hardware specification approved  
✅ Procurement/access confirmed  
✅ Measurement tools operational  
✅ Regression gate thresholds locked  
✅ Ready for Phase 2 execution (Sept 17–30)

**Sign-Off Authority:** Platform Performance Lead + Infrastructure Owner

**Current Status:** Draft for Sept 2 Review

---

## Appendix A: Hardware Reference Specifications

### NVIDIA A100 40GB Specifications
```
GPU Memory:        40GB HBM2e
Compute Capability: 8.0
SMs:               108 (432 CUDA cores/SM, 46,080 total)
Boost Clock:       1.410 GHz
Memory Bandwidth:  1.555 TB/s
PCIe:              Gen 4 x16 (64 GB/s)
Max Power:         250W
Thermal:           < 80°C recommended
```

### System Configuration Template
```bash
# CUDA toolkit verification
nvcc --version  # >= CUDA 12.0

# Driver check
nvidia-smi  # >= 535 version

# NVML binding test
python3 -c "import pynvml; pynvml.nvmlInit(); print(pynvml.nvmlDeviceGetCount())"

# Thermal baseline
watch -n 1 nvidia-smi --query-gpu=temperature.gpu --format=csv

# Memory baseline (RocksDB prep)
df -h /mnt/ssd  # Verify 500GB+ available
```

---

**Document Version:** 1.0  
**Next Review:** Sept 5, 2026  
**Last Updated:** 2026-09-02
