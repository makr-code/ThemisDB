# Phase-0 Baseline Benchmark RUNBOOK

<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- Copyright (c) 2026 ThemisDB Contributors -->

## Quick Start

This runbook guides human operators through Phase-0 baseline measurement, comparison, and go/no-go decision.

### Prerequisites Checklist

- [ ] Linux/Windows machine with consistent hardware
- [ ] CMake 3.20+, Ninja, C++17 compiler
- [ ] RocksDB installed (vcpkg or librocksdb-dev)
- [ ] Google Benchmark linked
- [ ] Python 3.8+ for analysis scripts (upcoming)
- [ ] Thermal stability verified (no throttling expected)
- [ ] System load is minimal (idle or single-user)

## Phase 1: Initial Baseline Capture (First-Time Setup)

### Step 1.1: Build Phase-0 Benchmark

```bash
cd /path/to/ThemisDB

# Configure with benchmarks enabled
cmake --preset linux-release \
  -DTHEMIS_BUILD_BENCHMARKS=ON \
  -DCMAKE_BUILD_TYPE=Release

# Build the target
cmake --build --preset linux-release --target bench_p0_crud_baseline
```

**Expected outcome:** Binary at `build/Release/bin/benchmarks/bench_p0_crud_baseline`

### Step 1.2: Verify Binary Execution

```bash
# Test with --help to verify no runtime errors
./build/Release/bin/benchmarks/bench_p0_crud_baseline --help

# Dry-run to ensure no crashes
./build/Release/bin/benchmarks/bench_p0_crud_baseline --benchmark_list_tests
```

**Expected outcome:** Help text and list of tests (BM_P0_InsertHeavy, etc.)

### Step 1.3: Run Baseline (Uncontended Machine)

**⚠️ IMPORTANT:** Ensure machine is thermally stable and uncontended:

```bash
# Optional: disable CPU frequency scaling (Linux)
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Optional: pin process to single NUMA node (Linux)
numactl --preferred=0 ./build/Release/bin/benchmarks/bench_p0_crud_baseline \
  --benchmark_out=baseline_p0_v1_run1.json \
  --benchmark_out_format=json \
  --benchmark_repetitions=3 \
  --benchmark_enable_raw_timings=true
```

**Expected outcome:**
- JSON file with 4 benchmark results (insert_heavy, read_heavy, update_heavy, delete_heavy)
- Each result includes: iterations, time, cpu time, throughput
- No errors or warnings

### Step 1.4: Archive and Document

```bash
# Archive baseline and environment metadata
mkdir -p baselines/hardware_profiles/$(date +%Y%m%d_%H%M%S)
cp baseline_p0_v1_run1.json baselines/hardware_profiles/$(date +%Y%m%d_%H%M%S)/

# Record hardware info
cat > baselines/hardware_profiles/$(date +%Y%m%d_%H%M%S)/HARDWARE.md << 'EOF'
# Hardware Profile
- CPU: $(lscpu | grep "Model name")
- Cores: $(nproc)
- RAM: $(free -h | grep Mem | awk '{print $2}')
- Kernel: $(uname -r)
- Date: $(date)
EOF
```

## Phase 2: Regression Detection (Subsequent Runs)

### Step 2.1: Run New Benchmark

```bash
# Run with same conditions as original baseline
numactl --preferred=0 ./build/Release/bin/benchmarks/bench_p0_crud_baseline \
  --benchmark_out=baseline_p0_v1_run2.json \
  --benchmark_out_format=json \
  --benchmark_repetitions=3
```

### Step 2.2: Compare Against Baseline

*(Comparison script `compare_baseline.py` to be implemented in Phase 1)*

```bash
# Manual comparison (until script exists): extract ops/sec from JSON
python3 << 'PYTHON_EOF'
import json

with open('baselines/baseline_p0_v0.json') as f:
    baseline = json.load(f)

with open('baseline_p0_v1_run2.json') as f:
    current = json.load(f)

# Extract metrics for each benchmark
for bench in current['benchmarks']:
    name = bench['name']
    ops_per_sec_current = bench.get('items_per_second', 0)
    
    # Find baseline expectation
    workload = name.split('_')[-1].lower()  # extract workload type
    baseline_target = baseline['baseline_metrics'].get(workload, {}).get('target_ops_per_sec', 0)
    
    if baseline_target > 0:
        diff_pct = ((ops_per_sec_current - baseline_target) / baseline_target) * 100
        status = "✓ PASS" if abs(diff_pct) <= 10 else "✗ REGRESSION"
        print(f"{name}: {ops_per_sec_current:.0f} ops/s ({diff_pct:+.1f}%) [{status}]")
PYTHON_EOF
```

### Step 2.3: Decision Gate

**Go/No-Go Decision:**

- **GO:** All 4 workloads pass (≤10% regression on throughput & latency)
- **NO-GO:** Any workload regresses >10% → investigate and revert changes

## Phase 3: Hardware-Specific Baselines

For different hardware platforms (e.g., ARM, different CPU generation), capture separate baselines:

```bash
# ARM baseline (e.g., Apple Silicon)
./build/Release/bin/benchmarks/bench_p0_crud_baseline \
  --benchmark_out=baseline_p0_arm64_run1.json \
  --benchmark_out_format=json

# GPU baseline (e.g., NVIDIA H100)
./build/Release/bin/benchmarks/bench_p0_crud_baseline \
  --benchmark_out=baseline_p0_gpu_run1.json \
  --benchmark_out_format=json
```

Archive in `baselines/hardware_profiles/{platform}/`.

## Phase 4: CI Integration (Future)

Phase-0 baselines will be integrated into CI gates:

```yaml
# .github/workflows/benchmarks_p0.yml (future)
- name: Run Phase-0 Baseline
  run: |
    ./build/Release/bin/benchmarks/bench_p0_crud_baseline \
      --benchmark_out=p0_result.json \
      --benchmark_out_format=json

- name: Compare Against Baseline
  run: |
    python3 compare_baseline.py \
      --baseline baselines/baseline_p0_v0.json \
      --results p0_result.json \
      --threshold 10
    # Exit non-zero if regression detected
```

## Troubleshooting

### Binary Won't Link

**Error:** `undefined reference to 'benchmark::...'`

**Solution:** Ensure Google Benchmark is installed and linked:

```bash
# Ubuntu/Debian
sudo apt-get install libbenchmark-dev

# macOS
brew install google-benchmark

# Or via vcpkg
vcpkg install benchmark:x64-linux
```

### Thermal Throttling Detected

**Error:** Benchmark results show sudden latency spikes or high variance

**Solution:** Wait for thermal stability:

```bash
# Check CPU temp (Linux)
cat /sys/class/thermal/thermal_zone0/temp

# Power down and wait 10+ minutes, then rerun
```

### Inconsistent Results Across Runs

**Error:** CV (Coefficient of Variation) > 15% for same workload

**Solution:** Check system load and reduce background processes:

```bash
# Minimal run
systemctl stop docker
systemctl stop postgresql  # or other services

# Run again with --benchmark_repetitions=5
```

## Metrics Interpretation

### Throughput (ops/sec)

- **Higher is better**
- Compare as percentage difference: `(current - baseline) / baseline * 100%`
- Threshold: ±10% is acceptable, >10% is regression

### Latency (microseconds)

- **Lower is better**
- Report P50, P95, P99
- Threshold: >10% increase in P99 is regression

### Coefficient of Variation (CV)

- **Lower is better** (measures stability)
- CV = standard_deviation / mean
- Threshold: CV > 15% indicates flakiness

## Known Limitations

1. **Stub KV Store:** Current benchmark uses in-memory KV; production will use real backend
2. **Single-Machine:** No distributed/sharding scenarios
3. **No Network:** Local operations only
4. **Hardware-Dependent:** Baselines vary by CPU, RAM, storage type

## Next Steps

- [ ] Integrate comparison script into CI
- [ ] Capture baselines for multiple hardware platforms
- [ ] Automate baseline regression detection in GitHub Actions
- [ ] Extend to multi-node scenarios (Phase 1+)
- [ ] Link to real themis_core backend (Phase 1+)

## Contact & Support

For issues or questions about Phase-0 baselines:
- Check `MEASUREMENT_PROTOCOL.md` for detailed protocol
- Review baseline JSON in `baselines/baseline_p0_v0.json`
- Open GitHub issue with benchmark results and hardware profile
