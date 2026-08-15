# Analytics Phase 5 - Benchmark CI/CD Integration Guide

**Purpose**: Enable automated performance testing in GitHub Actions CI/CD pipeline

**Date**: August 15, 2026

---

## Quick Start

### Build & Run Benchmarks Locally
```bash
# Build release binary
cd /path/to/ThemisDB
mkdir -p build && cd build
cmake -DTHEMIS_BUILD_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS="-O3 -march=native -DNDEBUG" ..
make bench_analytics_gap_closure

# Run all benchmarks
./bin/bench_analytics_gap_closure \
  --benchmark_out=analytics_results.json \
  --benchmark_out_format=json \
  --benchmark_repetitions=5 \
  --benchmark_min_time=0.1

# Run specific cluster
./bin/bench_analytics_gap_closure --benchmark_filter="^BM_PM.*"  # Process Mining
./bin/bench_analytics_gap_closure --benchmark_filter="^BM_AM.*"  # AutoML
./bin/bench_analytics_gap_closure --benchmark_filter="^BM_FC.*"  # Forecasting
./bin/bench_analytics_gap_closure --benchmark_filter="^BM_CEP.*" # CEP/Streaming
./bin/bench_analytics_gap_closure --benchmark_filter="^BM_KB.*"  # Knowledge Base
./bin/bench_analytics_gap_closure --benchmark_filter="^BM_UT.*"  # Utilities

# Run with memory sanitizer
ASAN_OPTIONS=verbosity=2 ./bin/bench_analytics_gap_closure
```

---

## CI/CD Pipeline Configuration

### GitHub Actions Workflow

Create `.github/workflows/analytics-benchmarks.yml`:

```yaml
name: Analytics Phase 5 Benchmarks

on:
  push:
    branches: [ main, develop, release/* ]
    paths:
      - 'src/analytics/**'
      - 'benchmarks/analytics/**'
      - 'include/analytics/**'
  pull_request:
    branches: [ main, develop ]
    paths:
      - 'src/analytics/**'
      - 'benchmarks/analytics/**'
      - 'include/analytics/**'
  schedule:
    # Run nightly to track performance trends
    - cron: '0 2 * * *'

jobs:
  benchmark:
    name: Performance Benchmarks
    runs-on: ubuntu-latest
    permissions:
      checks: write
      contents: read
      pull-requests: write
    
    steps:
      - uses: actions/checkout@v4
        with:
          fetch-depth: 0  # Full history for regression detection
      
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y \
            build-essential cmake \
            libssl-dev zlib1g-dev \
            rocksdb-dev librocksdb-dev
      
      - name: Configure CMake
        run: |
          mkdir -p build
          cd build
          cmake -DTHEMIS_BUILD_BENCHMARKS=ON \
                 -DCMAKE_BUILD_TYPE=Release \
                 -DCMAKE_CXX_FLAGS="-O3 -march=native -DNDEBUG" \
                 -DSANITIZE=address \
                 ..
      
      - name: Build benchmarks
        run: |
          cd build
          make bench_analytics_gap_closure -j$(nproc)
      
      - name: Run benchmarks
        id: run_benchmarks
        run: |
          cd build
          ./bin/bench_analytics_gap_closure \
            --benchmark_out=analytics_phase5_results.json \
            --benchmark_out_format=json \
            --benchmark_repetitions=5 \
            --benchmark_min_time=0.1 \
            --benchmark_time_unit=ms \
            2>&1 | tee benchmark_output.txt
      
      - name: Download baseline
        run: |
          # Store baseline in repo or artifact storage
          if [ -f "benchmarks/baselines/analytics_wave7.json" ]; then
            cp benchmarks/baselines/analytics_wave7.json baseline.json
          else
            echo "No baseline found - establishing fresh baseline"
            cp build/analytics_phase5_results.json baseline.json
          fi
      
      - name: Detect regressions
        id: detect_regressions
        continue-on-error: true
        run: |
          python3 scripts/detect_benchmark_regression.py \
            --baseline baseline.json \
            --current build/analytics_phase5_results.json \
            --output regression_report.json \
            --config benchmarks/analytics/regression_thresholds.toml
      
      - name: Generate benchmark report
        run: |
          python3 scripts/generate_benchmark_report.py \
            --results build/analytics_phase5_results.json \
            --regression regression_report.json \
            --output ANALYTICS_BENCHMARK_REPORT.md
      
      - name: Comment on PR
        if: github.event_name == 'pull_request'
        uses: actions/github-script@v7
        with:
          script: |
            const fs = require('fs');
            const report = fs.readFileSync('ANALYTICS_BENCHMARK_REPORT.md', 'utf8');
            github.rest.issues.createComment({
              issue_number: context.issue.number,
              owner: context.repo.owner,
              repo: context.repo.repo,
              body: report
            });
      
      - name: Upload results
        if: always()
        uses: actions/upload-artifact@v3
        with:
          name: benchmark-results
          path: |
            build/analytics_phase5_results.json
            regression_report.json
            ANALYTICS_BENCHMARK_REPORT.md
            benchmark_output.txt
      
      - name: Upload to Benchmark Tracker
        if: success()
        run: |
          # Optional: Push results to external benchmark tracking service
          # e.g., Buildkite, Dashboard, S3, etc.
          curl -X POST \
            -H "Authorization: ****** secrets.BENCHMARK_API_TOKEN }}" \
            -H "Content-Type: application/json" \
            -d @build/analytics_phase5_results.json \
            https://benchmark-api.example.com/v1/analytics/results
      
      - name: Fail on critical regression
        if: failure() && steps.detect_regressions.outcome == 'failure'
        run: |
          echo "Critical performance regression detected!"
          echo "See regression_report.json for details"
          exit 1
```

---

## Benchmark Regression Detection

### Regression Thresholds Configuration

Create `benchmarks/analytics/regression_thresholds.toml`:

```toml
# Analytics Phase 5 Benchmark Regression Thresholds
# Format: metric_threshold = percentage

[process_mining]
"BM_PM01_BuildDFG" = 10
"BM_PM02_DiscoverInductiveProcess" = 10
"BM_PM03_ConformanceCheck" = 10

[automl]
"BM_AM01_GridSearch" = 15
"BM_AM02_Prediction" = 15

[forecasting]
"BM_FC01_TimeSeriesFit" = 10
"BM_FC02_BatchPredictSIMD" = 10

[cep_streaming]
"BM_CEP01_EventBatchProcessing" = 15
"BM_CEP02_WindowFlushLatency" = 15

[knowledge_base]
"BM_KB01_FactAssertion" = 10
"BM_KB02_QueryFacts" = 10

[utilities]
"BM_UT01_ColumnarAggregate" = 15
"BM_UT02_DistributedMerge" = 15

# Global settings
[settings]
min_samples = 3  # Minimum benchmark iterations to consider
warn_threshold = 5  # Warn if regression > this % (before failing)
fail_threshold = 15  # Fail CI if regression > this %
```

### Regression Detection Script

Create `scripts/detect_benchmark_regression.py`:

```python
#!/usr/bin/env python3
"""
Detect performance regressions between benchmark runs.

Usage:
    python detect_benchmark_regression.py \
        --baseline baseline.json \
        --current current.json \
        --output regression_report.json \
        --config benchmarks/analytics/regression_thresholds.toml
"""

import json
import sys
import argparse
import statistics
from pathlib import Path
from typing import Dict, List, Optional, Tuple
import toml


def load_benchmark_json(path: str) -> Dict:
    """Load Google Benchmark JSON output."""
    with open(path) as f:
        return json.load(f)


def extract_benchmark_stats(benchmarks: List[Dict]) -> Dict[str, Dict]:
    """Extract mean, stddev, min, max from benchmark runs."""
    stats = {}
    
    for bench in benchmarks:
        name = bench.get('name', '')
        # Filter out aggregate runs (they end with _mean, _stddev, etc.)
        if any(name.endswith(suffix) for suffix in ['_mean', '_stddev', '_median', '_stddev', '_min', '_max']):
            continue
        
        times = [run.get('real_time', 0) for run in bench.get('runs', [])]
        
        if times:
            stats[name] = {
                'mean': statistics.mean(times),
                'stddev': statistics.stdev(times) if len(times) > 1 else 0,
                'min': min(times),
                'max': max(times),
                'count': len(times),
                'unit': bench.get('unit', 'ms'),
                'time_unit': bench.get('time_unit', 'ms'),
            }
    
    return stats


def calculate_regression(baseline: float, current: float) -> float:
    """Calculate regression percentage: (current - baseline) / baseline * 100"""
    if baseline == 0:
        return 0
    return ((current - baseline) / baseline) * 100


def detect_regressions(
    baseline_stats: Dict[str, Dict],
    current_stats: Dict[str, Dict],
    thresholds: Dict[str, float]
) -> Tuple[List[Dict], int]:
    """
    Detect regressions between baseline and current runs.
    
    Returns:
        (regression_details, total_regressions)
    """
    regressions = []
    regression_count = 0
    
    for bench_name, current in current_stats.items():
        if bench_name not in baseline_stats:
            continue
        
        baseline = baseline_stats[bench_name]
        
        # Extract threshold for this benchmark
        threshold = None
        for prefix, thresh in thresholds.items():
            if bench_name.lower().startswith('bm_' + prefix.lower()):
                threshold = thresh
                break
        
        if threshold is None:
            threshold = 10  # Default threshold
        
        # Calculate regression
        regression_pct = calculate_regression(
            baseline['mean'],
            current['mean']
        )
        
        # Determine status
        status = 'PASS'
        if abs(regression_pct) > threshold:
            status = 'FAIL'
            regression_count += 1
        elif abs(regression_pct) > threshold * 0.5:
            status = 'WARN'
        
        regressions.append({
            'benchmark': bench_name,
            'baseline_mean': baseline['mean'],
            'baseline_stddev': baseline['stddev'],
            'current_mean': current['mean'],
            'current_stddev': current['stddev'],
            'regression_pct': round(regression_pct, 2),
            'threshold_pct': threshold,
            'status': status,
            'unit': current.get('unit', 'ms'),
        })
    
    return regressions, regression_count


def generate_report(
    regressions: List[Dict],
    baseline_path: str,
    current_path: str
) -> str:
    """Generate markdown report of regressions."""
    report = []
    report.append('# Analytics Benchmark Regression Report\n')
    
    # Summary
    total = len(regressions)
    passes = sum(1 for r in regressions if r['status'] == 'PASS')
    warnings = sum(1 for r in regressions if r['status'] == 'WARN')
    failures = sum(1 for r in regressions if r['status'] == 'FAIL')
    
    report.append(f'## Summary\n')
    report.append(f'- **Total Benchmarks**: {total}\n')
    report.append(f'- **✅ PASS**: {passes}\n')
    report.append(f'- **⚠️  WARN**: {warnings}\n')
    report.append(f'- **❌ FAIL**: {failures}\n\n')
    
    # Detailed table
    report.append('## Regression Details\n\n')
    report.append('| Benchmark | Baseline | Current | Regression | Status |\n')
    report.append('|-----------|----------|---------|------------|--------|\n')
    
    for r in sorted(regressions, key=lambda x: abs(x['regression_pct']), reverse=True):
        baseline = f"{r['baseline_mean']:.2f}±{r['baseline_stddev']:.2f}"
        current = f"{r['current_mean']:.2f}±{r['current_stddev']:.2f}"
        regression = f"{r['regression_pct']:+.2f}% ({r['threshold_pct']}% gate)"
        emoji = '✅' if r['status'] == 'PASS' else ('⚠️' if r['status'] == 'WARN' else '❌')
        
        report.append(
            f'| {r["benchmark"]} | {baseline} {r["unit"]} | '
            f'{current} {r["unit"]} | {regression} | {emoji} {r["status"]} |\n'
        )
    
    return ''.join(report)


def main():
    parser = argparse.ArgumentParser(description='Detect benchmark regressions')
    parser.add_argument('--baseline', required=True, help='Baseline benchmark JSON')
    parser.add_argument('--current', required=True, help='Current benchmark JSON')
    parser.add_argument('--output', required=True, help='Output regression report JSON')
    parser.add_argument('--config', required=False, help='Regression thresholds TOML')
    
    args = parser.parse_args()
    
    # Load benchmarks
    baseline_data = load_benchmark_json(args.baseline)
    current_data = load_benchmark_json(args.current)
    
    baseline_stats = extract_benchmark_stats(baseline_data.get('benchmarks', []))
    current_stats = extract_benchmark_stats(current_data.get('benchmarks', []))
    
    # Load thresholds
    thresholds = {}
    if args.config and Path(args.config).exists():
        config = toml.load(args.config)
        for cluster, benchmarks in config.items():
            if cluster != 'settings' and isinstance(benchmarks, dict):
                for bench_name, threshold in benchmarks.items():
                    thresholds[cluster] = threshold
    
    # Detect regressions
    regressions, regression_count = detect_regressions(
        baseline_stats,
        current_stats,
        thresholds
    )
    
    # Output JSON report
    report_data = {
        'baseline_file': args.baseline,
        'current_file': args.current,
        'total_benchmarks': len(regressions),
        'pass_count': sum(1 for r in regressions if r['status'] == 'PASS'),
        'warn_count': sum(1 for r in regressions if r['status'] == 'WARN'),
        'fail_count': regression_count,
        'regressions': regressions,
    }
    
    with open(args.output, 'w') as f:
        json.dump(report_data, f, indent=2)
    
    # Print summary to console
    print(generate_report(regressions, args.baseline, args.current))
    
    # Exit with error if regressions found
    sys.exit(1 if regression_count > 0 else 0)


if __name__ == '__main__':
    main()
```

---

## Local Development Workflow

### Running Benchmarks During Development

```bash
# 1. Before you start
./bin/bench_analytics_gap_closure \
  --benchmark_out=baseline.json \
  --benchmark_out_format=json

# 2. Make your changes to src/analytics/...

# 3. Rebuild
make bench_analytics_gap_closure

# 4. Check performance
./bin/bench_analytics_gap_closure \
  --benchmark_out=current.json \
  --benchmark_out_format=json

# 5. Detect regressions
python3 scripts/detect_benchmark_regression.py \
  --baseline baseline.json \
  --current current.json \
  --output regression_report.json

# 6. Review results
cat regression_report.json | jq '.fail_count'  # Should be 0
```

### Memory Profiling During Development

```bash
# Build with AddressSanitizer
cmake -DTHEMIS_BUILD_BENCHMARKS=ON \
      -DCMAKE_CXX_FLAGS="-fsanitize=address -O1 -g" ..
make bench_analytics_gap_closure

# Run with ASAN output
ASAN_OPTIONS=verbosity=2:halt_on_error=1 \
  ./bin/bench_analytics_gap_closure \
  --benchmark_filter=PM01
```

---

## Benchmark Results Storage & Trending

### Store Baseline in Repository

```bash
# After establishing Wave 7 baseline, commit to repo
mkdir -p benchmarks/baselines
cp build/analytics_phase5_results.json benchmarks/baselines/analytics_wave7.json
git add benchmarks/baselines/analytics_wave7.json
git commit -m "Wave 7 Analytics Benchmark Baseline"
```

### Trend Analysis (Optional)

Store historical results in external database:

```bash
# Example: Push to InfluxDB for trending
curl -X POST \
  -H "Authorization: Token $INFLUXDB_TOKEN" \
  "https://influxdb.example.com/write?db=benchmarks" \
  --data-binary @/dev/stdin << EOF
analytics_phase5,benchmark=PM01 time=10.5,stddev=0.3 $(date +%s)000000000
analytics_phase5,benchmark=PM02 time=48.2,stddev=1.1 $(date +%s)000000000
EOF
```

---

## Troubleshooting

### Benchmark Won't Build
```bash
# Check dependencies
cmake --system-information | grep -i benchmark

# Verify google-benchmark is installed via vcpkg
ls vcpkg_installed/*/include/benchmark/benchmark.h

# Rebuild vcpkg dependencies
./vcpkg/vcpkg install benchmark --clean-after-build
```

### High Variability in Results
```bash
# Increase time per benchmark iteration
./bin/bench_analytics_gap_closure \
  --benchmark_min_time=1.0  # Run each at least 1 second

# Reduce system noise
sudo cpupower frequency-set -g performance  # (Linux)
```

### False Regression Detections
```bash
# Check if baseline is stale
git log --oneline benchmarks/baselines/analytics_wave7.json | head -5

# Re-establish baseline if needed
./bin/bench_analytics_gap_closure \
  --benchmark_out=benchmarks/baselines/analytics_wave7.json \
  --benchmark_out_format=json \
  --benchmark_repetitions=10  # More iterations for stability
```

---

## Performance Optimization Checklist

If benchmarks show regression, follow this checklist:

- [ ] Run with profiler: `perf stat ./bin/bench_analytics_gap_closure`
- [ ] Check for new memory allocations: `valgrind --leak-check=full`
- [ ] Verify compiler flags: `-O3 -march=native` applied
- [ ] Check for unexpected branches: Use `benchmark::DoNotOptimize()` correctly
- [ ] Profile hot paths with: `perf record -g` then `perf report`
- [ ] Compare before/after object code: `objdump -d` on hot functions
- [ ] Ensure inlining: Check `-flto` and function sizes

---

## References

- **Google Benchmark Docs**: https://github.com/google/benchmark
- **Regression Detection**: `scripts/detect_benchmark_regression.py`
- **Benchmark Code**: `benchmarks/analytics/bench_analytics_gap_closure.cpp`
- **CI Configuration**: `.github/workflows/analytics-benchmarks.yml`
- **Performance Report**: `ANALYTICS_PHASE5_BENCHMARK_RESULTS.md`

---

**Last Updated**: August 15, 2026  
**Status**: Ready for CI/CD Integration
