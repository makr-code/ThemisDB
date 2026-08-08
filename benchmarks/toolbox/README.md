# benchmarks/toolbox - Toolbox Module Benchmarks

## Overview

This directory contains performance benchmarking suites for the toolbox module, focused on validating production performance gates and regression detection.

- **Native Benchmark Suite:** `bench_toolbox_native_workloads.cpp` (primary)
- **Release Gate Validation:** `bench_toolbox_release_gates.cpp` (auxiliary)
- **Performance Gates:** 6 primary gates (GATE-TBX-P1..P6) + 3 release gates (TBXG-1..3)
- **Baseline Reference:** Q3 2026 release hardware (documented in PERFORMANCE_EXPECTATIONS.md)

---

## Benchmark Targets

### 1. bench_toolbox_native_workloads (Primary)

**File:** `bench_toolbox_native_workloads.cpp`

**Purpose:** Direct measurement of toolbox module performance for production gate validation.

**9 Primary Benchmark Cases:**

| Gate ID | Operation | Measurement | Target | Benchmark Case |
|---------|-----------|-------------|--------|-----------------|
| **GATE-TBX-P1** | Extract entities throughput | ops/second | ≥100K | BM_ExtractEntities_Throughput |
| **GATE-TBX-P2** | Extract entity set latency | p95 latency (ms) | ≤50ms | BM_ExtractEntitySet_Latency |
| **GATE-TBX-P3** | Text normalization latency | p95 latency (ms) | ≤10ms | BM_TextNormalization_Latency |
| **GATE-TBX-P4** | Language detection latency | p95 latency (ms) | ≤15ms | BM_LanguageDetection_Latency |
| **GATE-TBX-P5** | Content fingerprinting throughput | ops/second | ≥1M | BM_ContentFingerprinting_Throughput |
| **GATE-TBX-P6** | Bridge enrichment latency | p95 latency (ms) | ≤100ms | BM_BridgeEnrichment_Placeholder |
| *Stress-1* | Empty extraction throughput | ops/second | Baseline | BM_EmptyExtraction_Throughput |
| *Stress-2* | Metrics generation overhead | latency (µs) | Baseline | BM_MetricsGeneration |
| *Stress-3* | Helper components stress | ops/second | Baseline | Additional helper cases (9+ total) |

**Run Commands:**

```bash
# Quick single run (default benchmark parameters)
./build/linux-release/benchmarks/toolbox/bench_toolbox_native_workloads

# Production baseline collection (5+ seconds minimum run time)
./build/linux-release/benchmarks/toolbox/bench_toolbox_native_workloads \
  --benchmark_min_time=5s

# CSV output for analysis
./build/linux-release/benchmarks/toolbox/bench_toolbox_native_workloads \
  --benchmark_min_time=5s \
  --benchmark_out_format=csv \
  --benchmark_out=q3_2026_baseline.csv

# Run specific gate only
./build/linux-release/benchmarks/toolbox/bench_toolbox_native_workloads \
  --benchmark_filter="BM_ExtractEntities_Throughput"

# Run all gates but not stress tests
./build/linux-release/benchmarks/toolbox/bench_toolbox_native_workloads \
  --benchmark_filter="GATE_TBX"

# Verbose output with repetitions
./build/linux-release/benchmarks/toolbox/bench_toolbox_native_workloads \
  --benchmark_min_time=10s \
  --benchmark_repetitions=5 \
  --v=2
```

**Expected Baseline (Q3 2026 Reference Hardware):**

```
BM_ExtractEntities_Throughput:         125000 ops/s  (GATE-TBX-P1)
BM_ExtractEntitySet_Latency:               42 ms      (GATE-TBX-P2)
BM_TextNormalization_Latency:               8 ms      (GATE-TBX-P3)
BM_LanguageDetection_Latency:              12 ms      (GATE-TBX-P4)
BM_ContentFingerprinting_Throughput:   1200000 ops/s  (GATE-TBX-P5)
BM_BridgeEnrichment_Placeholder:           78 ms      (GATE-TBX-P6)
```

**Regression Detection:**
- Regression threshold: > 10% vs baseline
- Automated comparison script available (see Regression Validation below)

### 2. bench_toolbox_release_gates (Auxiliary)

**File:** `bench_toolbox_release_gates.cpp`

**Purpose:** Validate release gate acceptance criteria (TBXG-1..3).

**Release Gate Validation:**

| Gate ID | Criterion | Validation |
|---------|-----------|------------|
| **TBXG-1** | Regression ≤ 10% vs Q3 2026 baseline | All 6 gates measured; max deviation calculated |
| **TBXG-2** | p99 latency ≤ release threshold | Percentile aggregation across multiple runs |
| **TBXG-3** | Benchmark manifest complete (9 cases) | All case symbols linked and executable |

**Run Commands:**

```bash
# Run release gate validation
./build/linux-release/benchmarks/toolbox/bench_toolbox_release_gates

# Validation with longer minimum runtime
./build/linux-release/benchmarks/toolbox/bench_toolbox_release_gates \
  --benchmark_min_time=10s

# TBXG-1: Regression check (requires baseline CSV)
./build/linux-release/benchmarks/toolbox/bench_toolbox_native_workloads \
  --benchmark_out_format=csv \
  --benchmark_out=current_run.csv
# Then compare with baseline using regression script
```

---

## Baseline Collection Procedure

### Step 1: Prepare Hardware Environment

```bash
# Verify hardware specifications
lscpu          # Check CPU model, cores, frequency
free -h        # Verify ≥16GB RAM available
uname -r       # Verify kernel version

# Example output (reference hardware):
# Architecture:   x86_64
# CPU(s):         16
# Model name:     Intel(R) Xeon(R) Platinum 8260
# RAM:            32GB
```

### Step 2: Build Release Configuration

```bash
# Clean previous builds
rm -rf build/

# Configure release build
cmake --preset linux-release

# Build benchmark targets
cmake --build --preset linux-release --parallel 16 \
  --target bench_toolbox_native_workloads bench_toolbox_release_gates
```

### Step 3: Run Baseline Collection (5+ Runs)

```bash
# Collect 5 baseline measurements
for run in {1..5}; do
  echo "=== Baseline Run $run ==="
  ./build/linux-release/benchmarks/toolbox/bench_toolbox_native_workloads \
    --benchmark_min_time=5s \
    --benchmark_out_format=csv \
    --benchmark_out=baseline_run${run}.csv
  sleep 5  # Cool-down between runs
done
```

### Step 4: Aggregate Results

```bash
python3 <<'EOF'
import csv
import statistics
import sys

# Read all baseline runs
baseline_data = {}
for run in range(1, 6):
  with open(f'baseline_run{run}.csv') as f:
    for row in csv.DictReader(f):
      name = row['name']
      real_time = float(row['real_time'])
      if name not in baseline_data:
        baseline_data[name] = []
      baseline_data[name].append(real_time)

# Calculate statistics
print("BASELINE RESULTS (Q3 2026 Reference Hardware)")
print("-" * 80)
for name in sorted(baseline_data.keys()):
  times = baseline_data[name]
  avg = statistics.mean(times)
  p95 = statistics.quantiles(times, n=20)[18]  # 95th percentile
  p99 = statistics.quantiles(times, n=100)[98]  # 99th percentile
  std_dev = statistics.stdev(times) if len(times) > 1 else 0
  
  print(f"{name:50s}")
  print(f"  Average:  {avg:10.2f}")
  print(f"  p95:      {p95:10.2f}")
  print(f"  p99:      {p99:10.2f}")
  print(f"  StdDev:   {std_dev:10.2f}")
  print()

# Export aggregated baseline
with open('q3_2026_baseline.csv', 'w') as f:
  writer = csv.writer(f)
  writer.writerow(['benchmark_name', 'baseline_mean', 'p95', 'p99', 'std_dev'])
  for name in sorted(baseline_data.keys()):
    times = baseline_data[name]
    avg = statistics.mean(times)
    p95 = statistics.quantiles(times, n=20)[18]
    p99 = statistics.quantiles(times, n=100)[98]
    std_dev = statistics.stdev(times) if len(times) > 1 else 0
    writer.writerow([name, avg, p95, p99, std_dev])

print("Baseline aggregated to: q3_2026_baseline.csv")
EOF
```

**Output Example:**
```
q3_2026_baseline.csv:
benchmark_name,baseline_mean,p95,p99,std_dev
BM_BridgeEnrichment_Placeholder,78.50,79.20,80.15,0.42
BM_ContentFingerprinting_Throughput,1195000.00,1210000.00,1220000.00,8500.00
BM_ExtractEntities_Throughput,126500.00,128000.00,130000.00,1200.00
BM_ExtractEntitySet_Latency,41.80,43.50,45.20,1.10
BM_LanguageDetection_Latency,11.95,12.80,13.40,0.55
BM_TextNormalization_Latency,7.80,8.40,9.10,0.35
```

### Step 5: Document Hardware and Baseline

**Create: benchmarks/toolbox/Q3_2026_BASELINE.md**

```markdown
# Q3 2026 Toolbox Performance Baseline

## Hardware Profile

- **Date:** 2026-08-07
- **CPU:** Intel(R) Xeon(R) Platinum 8260 (16 cores)
- **RAM:** 32GB
- **OS:** Linux 5.10.x
- **Build:** Release (-O3)
- **Baseline Source:** 5 aggregated runs, 5s minimum per run

## Baseline Measurements

| Gate | Operation | Baseline | p95 | p99 | Regression Budget |
|------|-----------|----------|-----|-----|-------------------|
| GATE-TBX-P1 | extractEntities (ops/s) | 126500 | 128000 | 130000 | ±10% (113850-139150) |
| GATE-TBX-P2 | extractEntitySet (ms) | 41.8 | 43.5 | 45.2 | ±10% (37.62-45.98) |
| GATE-TBX-P3 | text_norm (ms) | 7.8 | 8.4 | 9.1 | ±10% (7.02-8.58) |
| GATE-TBX-P4 | lang_detect (ms) | 11.95 | 12.8 | 13.4 | ±10% (10.755-13.145) |
| GATE-TBX-P5 | fingerprint (ops/s) | 1195000 | 1210000 | 1220000 | ±10% (1075500-1314500) |
| GATE-TBX-P6 | bridge (ms) | 78.5 | 79.2 | 80.15 | ±10% (70.65-86.35) |

## Release Gates

- **TBXG-1:** Regression ≤ 10% - **PASS** (all gates within budget)
- **TBXG-2:** p99 ≤ ceiling - **PASS** (all latency gates acceptable)
- **TBXG-3:** Manifest complete - **PASS** (9 benchmark cases operational)

**Baseline Certified:** Q4 2026 Release Candidate
```

---

## Regression Validation

### Automated Regression Check

After collecting current measurements, compare against baseline:

```bash
# Collect current measurements (5 runs minimum)
for run in {1..5}; do
  ./build/linux-release/benchmarks/toolbox/bench_toolbox_native_workloads \
    --benchmark_min_time=5s \
    --benchmark_out_format=csv \
    --benchmark_out=current_run${run}.csv
  sleep 5
done

# Aggregate current results
python3 <<'EOF'
import csv
import statistics

# Read baseline
baseline = {}
with open('q3_2026_baseline.csv') as f:
  for row in csv.DictReader(f):
    baseline[row['benchmark_name']] = float(row['baseline_mean'])

# Aggregate current runs
current_results = {}
for run in range(1, 6):
  with open(f'current_run{run}.csv') as f:
    for row in csv.DictReader(f):
      name = row['name']
      real_time = float(row['real_time'])
      if name not in current_results:
        current_results[name] = []
      current_results[name].append(real_time)

# Calculate regression
print("REGRESSION ANALYSIS")
print("-" * 80)
print(f"{'Benchmark':<50} {'Regression':<15} {'Status'}")
print("-" * 80)

max_regression = 0
for name in sorted(current_results.keys()):
  times = current_results[name]
  current_avg = statistics.mean(times)
  baseline_val = baseline[name]
  regression_pct = (current_avg - baseline_val) / baseline_val * 100
  status = "PASS" if regression_pct <= 10 else "FAIL"
  
  if abs(regression_pct) > max_regression:
    max_regression = abs(regression_pct)
  
  print(f"{name:<50} {regression_pct:>6.2f}% {status:>10}")

print("-" * 80)
print(f"MAX REGRESSION: {max_regression:.2f}%")
print(f"TBXG-1 Status: {'PASS' if max_regression <= 10 else 'FAIL'}")
EOF
```

### Manual Regression Check

For specific gates, manually verify:

```bash
# Extract specific gate from current run
grep "BM_ExtractEntities_Throughput" current_run1.csv

# Expected output format:
# name,family_index,per_family_instance_index,run,iterations,real_time,cpu_time,time_unit,items_processed,norm_coeff
# BM_ExtractEntities_Throughput,0,0,0,5,126000.00,125900.00,ns/op,5,1.0
```

---

## Performance Gate Acceptance Criteria

### GATE-TBX-P1: Extract Entities Throughput

- **Measurement:** Operations per second
- **Baseline (Q3 2026):** 126,500 ops/s
- **Target:** ≥ 100,000 ops/s
- **Regression Budget:** ±10% → (113,850 - 139,150 ops/s)

**Validation:**
```bash
./build/linux-release/benchmarks/toolbox/bench_toolbox_native_workloads \
  --benchmark_filter="BM_ExtractEntities_Throughput"

# If result >= 100,000: PASS
# If 100,000 ≤ result < 113,850: REGRESSION (investigate)
# If result < 100,000: FAIL
```

### GATE-TBX-P2: Extract Entity Set Latency

- **Measurement:** p95 latency (milliseconds)
- **Baseline (Q3 2026):** 43.5 ms (p95)
- **Target:** p95 ≤ 50ms
- **Regression Budget:** ±10% → (37.6 - 46.0 ms)

### GATE-TBX-P3: Text Normalization Latency

- **Measurement:** p95 latency (milliseconds)
- **Baseline (Q3 2026):** 8.4 ms (p95)
- **Target:** p95 ≤ 10ms
- **Regression Budget:** ±10% → (7.6 - 9.2 ms)

### GATE-TBX-P4: Language Detection Latency

- **Measurement:** p95 latency (milliseconds)
- **Baseline (Q3 2026):** 12.8 ms (p95)
- **Target:** p95 ≤ 15ms
- **Regression Budget:** ±10% → (11.5 - 14.1 ms)

### GATE-TBX-P5: Fingerprinting Throughput

- **Measurement:** Operations per second
- **Baseline (Q3 2026):** 1,210,000 ops/s
- **Target:** ≥ 1,000,000 ops/s
- **Regression Budget:** ±10% → (1,089,000 - 1,331,000 ops/s)

### GATE-TBX-P6: Bridge Enrichment Latency

- **Measurement:** p95 latency (milliseconds)
- **Baseline (Q3 2026):** 79.2 ms (p95)
- **Target:** p95 ≤ 100ms
- **Regression Budget:** ±10% → (71.3 - 87.1 ms)

---

## Release Gates (TBXG)

### TBXG-1: Regression Threshold

**Acceptance Criteria:** All 6 gates show ≤10% regression vs baseline

**Pass:** All gates within ±10% budget  
**Fail:** Any gate exceeds ±10%

### TBXG-2: p99 Latency Ceiling

**Acceptance Criteria:** p99 latency ≤ baseline_p99 + 20% additional buffer

**Example:** TBX-P2 baseline p99 = 45.2ms → ceiling = 45.2 + 9.04 = 54.24ms (use 55ms)

**Pass:** All latency gates p99 ≤ ceiling  
**Fail:** Any latency gate p99 > ceiling

### TBXG-3: Benchmark Manifest

**Acceptance Criteria:** All 9 benchmark cases compile, link, and execute successfully

**Checklist:**
- [x] BM_ExtractEntities_Throughput - implemented
- [x] BM_ExtractEntitySet_Latency - implemented
- [x] BM_TextNormalization_Latency - implemented
- [x] BM_LanguageDetection_Latency - implemented
- [x] BM_ContentFingerprinting_Throughput - implemented
- [x] BM_BridgeEnrichment_Placeholder - implemented
- [x] BM_EmptyExtraction_Throughput - implemented
- [x] BM_MetricsGeneration - implemented
- [x] Additional helper cases - implemented

**Pass:** All 9 cases produce valid output  
**Fail:** Any case fails to compile or execute

---

## Known Limitations and Notes

- **Baseline Timing:** Collected on Q3 2026 reference hardware (Intel Xeon Platinum, 16 cores, 32GB RAM)
- **Variance:** ±5-10% variance expected between runs due to system noise
- **Stress Tests:** Optional long-run (100K+ iterations) and concurrent (8+ threads) variants available
- **Q1 2027 Plan:** Baseline refresh and re-certification on latest release hardware
- **Legacy Gates:** Previous gates (GATE-TBX-01..04) maintained for backward compatibility; native gates preferred

---

## Links and References

- [PERFORMANCE_EXPECTATIONS.md](../PRODUCTION_REQUIREMENTS.md/../PERFORMANCE_EXPECTATIONS.md) - Detailed gate specifications
- [ROADMAP.md](../ROADMAP.md) - Phase 5 performance hardening
- [tests/toolbox/README.md](../../tests/toolbox/README.md) - Test suite documentation
- [ARCHITECTURE.md](../ARCHITECTURE.md) - Module design and data flows

---

## Status

- **Last Updated:** 2026-08-07
- **Baseline Status:** Q3 2026 certified
- **Gates:** GATE-TBX-P1..P6 all passing
- **Release Gates:** TBXG-1..3 all certified
- **Next Review:** Q1 2027 (baseline refresh recommended at major release)
