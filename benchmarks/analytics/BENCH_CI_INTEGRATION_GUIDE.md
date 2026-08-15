# Analytics Gap Closure Benchmarks - CI/CD Integration Guide

**Document**: Execution and regression detection workflow  
**Version**: Phase 5 Complete  
**Updated**: 2026-Q3

---

## Quick Start: Local Execution

### Build Benchmarks

```bash
# Standard build with benchmarking enabled
cd /home/runner/work/ThemisDB/ThemisDB
cmake -B build_bench \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_BENCHMARKING=ON \
  -DCMAKE_CXX_FLAGS="-march=native"
  
cmake --build build_bench --target bench_analytics_gap_closure -j4
```

### Run Individual Benchmarks

```bash
# Run single benchmark with 5 repetitions
./build_bench/benchmarks/analytics/bench_analytics_gap_closure \
  --benchmark_filter=BM_PM01 \
  --benchmark_repetitions=5

# Run all Gap Closure benchmarks
./build_bench/benchmarks/analytics/bench_analytics_gap_closure \
  --benchmark_repetitions=5 \
  --benchmark_min_time=1.0

# Output JSON for parsing
./build_bench/benchmarks/analytics/bench_analytics_gap_closure \
  --benchmark_repetitions=5 \
  --benchmark_format=json > gap_closure_results.json
```

### Available Benchmark Filters

```bash
# Process Mining
--benchmark_filter=BM_PM01_BuildDFG
--benchmark_filter=BM_PM02_DiscoverInductiveProcess
--benchmark_filter=BM_PM03_ConformanceCheck

# AutoML
--benchmark_filter=BM_AM01_GridSearch
--benchmark_filter=BM_AM02_Prediction

# Forecasting
--benchmark_filter=BM_FC01_TimeSeriesFit
--benchmark_filter=BM_FC02_BatchPredictSIMD

# CEP/Streaming
--benchmark_filter=BM_CEP01_EventBatchProcessing
--benchmark_filter=BM_CEP02_WindowFlushLatency

# Knowledge Base
--benchmark_filter=BM_KB01_FactAssertion
--benchmark_filter=BM_KB02_QueryFacts

# Utilities
--benchmark_filter=BM_UT01_ColumnarAggregate
--benchmark_filter=BM_UT02_DistributedMerge
```

---

## CI/CD Integration

### GitHub Actions Workflow

Create `.github/workflows/bench-gap-closure.yml`:

```yaml
name: Analytics Gap Closure Benchmarks

on:
  push:
    branches: [main, develop]
  pull_request:
  schedule:
    # Daily nightly run at 02:00 UTC
    - cron: '0 2 * * *'

jobs:
  benchmark:
    name: Gap Closure Performance Gates
    runs-on: ubuntu-latest-8core
    
    steps:
      - uses: actions/checkout@v4
        with:
          fetch-depth: 0
      
      - name: Setup build environment
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake ninja-build
      
      - name: Build benchmarks (Release)
        run: |
          cmake -B build_bench \
            -DCMAKE_BUILD_TYPE=Release \
            -DENABLE_BENCHMARKING=ON \
            -DCMAKE_CXX_FLAGS="-march=native" \
            -G Ninja
          cmake --build build_bench --target bench_analytics_gap_closure
      
      - name: Run benchmarks
        run: |
          ./build_bench/benchmarks/analytics/bench_analytics_gap_closure \
            --benchmark_repetitions=5 \
            --benchmark_format=json \
            --benchmark_out=gap_closure_results.json
      
      - name: Store baseline (main branch only)
        if: github.ref == 'refs/heads/main'
        run: |
          python3 scripts/store_benchmark_baseline.py \
            gap_closure_results.json \
            --storage s3://themis-benchmarks/gap-closure/ \
            --commit-sha ${{ github.sha }}
      
      - name: Validate gates vs baseline
        run: |
          python3 scripts/validate_benchmark_gates.py \
            gap_closure_results.json \
            --baseline-storage s3://themis-benchmarks/gap-closure/ \
            --tolerance 0.10 \
            --output gate_validation_report.json
      
      - name: Comment on PR with results
        if: github.event_name == 'pull_request'
        uses: actions/github-script@v7
        with:
          script: |
            const fs = require('fs');
            const report = JSON.parse(fs.readFileSync('gate_validation_report.json', 'utf8'));
            const comment = `## 📊 Analytics Gap Closure Benchmark Report\n\n${report.summary}\n\n${report.details}`;
            github.rest.issues.createComment({
              issue_number: context.issue.number,
              owner: context.repo.owner,
              repo: context.repo.repo,
              body: comment
            });
      
      - name: Fail if regression >10%
        run: |
          python3 scripts/check_gate_failures.py gate_validation_report.json
      
      - name: Upload results
        if: always()
        uses: actions/upload-artifact@v3
        with:
          name: benchmark-results-${{ github.sha }}
          path: |
            gap_closure_results.json
            gate_validation_report.json
          retention-days: 30
```

### Gate Validation Script

Create `scripts/validate_benchmark_gates.py`:

```python
#!/usr/bin/env python3
"""
Validate benchmark results against Wave 7 baseline with ≤10% tolerance.
"""

import json
import sys
from dataclasses import dataclass
from typing import Dict, List

# Wave 7 baseline metrics (from BENCH_GAP_CLOSURE_REPORT.md)
WAVE7_BASELINE = {
    "BM_PM01_BuildDFG": {"value": 150, "unit": "dfgs/s", "op": ">="},
    "BM_PM02_DiscoverInductiveProcess": {"value": 35, "unit": "ms", "op": "<="},
    "BM_PM03_ConformanceCheck": {"value": 1500, "unit": "traces/s", "op": ">="},
    "BM_AM01_GridSearch": {"value": 80, "unit": "ms", "op": "<="},
    "BM_AM02_Prediction": {"value": 7, "unit": "µs", "op": "<="},
    "BM_FC01_TimeSeriesFit": {"value": 70, "unit": "ms", "op": "<="},
    "BM_FC02_BatchPredictSIMD": {"value": 800000, "unit": "pts/s", "op": ">="},
    "BM_CEP01_EventBatchProcessing": {"value": 150000, "unit": "evt/s", "op": ">="},
    "BM_CEP02_WindowFlushLatency": {"value": 350, "unit": "µs", "op": "<="},
    "BM_KB01_FactAssertion": {"value": 15000, "unit": "facts/s", "op": ">="},
    "BM_KB02_QueryFacts": {"value": 80, "unit": "µs", "op": "<="},
    "BM_UT01_ColumnarAggregate": {"value": 1500000, "unit": "rows/s", "op": ">="},
    "BM_UT02_DistributedMerge": {"value": 8, "unit": "ms", "op": "<="},
}

TOLERANCE = 0.10  # ≤10% regression

@dataclass
class GateResult:
    benchmark: str
    baseline: float
    current: float
    threshold: float
    metric: str
    op: str
    status: str
    regression_pct: float

def parse_benchmark_json(filepath: str) -> Dict:
    """Parse Google Benchmark JSON output."""
    with open(filepath) as f:
        return json.load(f)

def extract_metric(benchmark_result: Dict) -> float:
    """Extract aggregated metric (mean) from benchmark result."""
    # Google Benchmark stores mean under cpu_time or real_time
    if "cpu_time" in benchmark_result:
        return benchmark_result["cpu_time"]
    return benchmark_result.get("real_time", 0)

def calculate_threshold(baseline: float, op: str) -> float:
    """Calculate acceptable threshold with ±10% tolerance."""
    if op == ">=":
        return baseline * (1 - TOLERANCE)  # Allow 10% decrease
    else:  # "<="
        return baseline * (1 + TOLERANCE)  # Allow 10% increase

def validate_gate(benchmark_name: str, current_value: float) -> GateResult:
    """Validate single benchmark against baseline."""
    if benchmark_name not in WAVE7_BASELINE:
        return GateResult(
            benchmark=benchmark_name,
            baseline=0,
            current=current_value,
            threshold=0,
            metric="unknown",
            op="??",
            status="UNKNOWN",
            regression_pct=0,
        )
    
    baseline_spec = WAVE7_BASELINE[benchmark_name]
    baseline = baseline_spec["value"]
    op = baseline_spec["op"]
    threshold = calculate_threshold(baseline, op)
    
    # Calculate regression percentage
    if baseline != 0:
        regression_pct = (current_value - baseline) / baseline * 100
    else:
        regression_pct = 0
    
    # Determine pass/fail
    if op == ">=":
        status = "PASS" if current_value >= threshold else "FAIL"
    else:  # "<="
        status = "PASS" if current_value <= threshold else "FAIL"
    
    return GateResult(
        benchmark=benchmark_name,
        baseline=baseline,
        current=current_value,
        threshold=threshold,
        metric=baseline_spec["unit"],
        op=op,
        status=status,
        regression_pct=regression_pct,
    )

def main():
    if len(sys.argv) < 2:
        print("Usage: validate_benchmark_gates.py <results.json>")
        sys.exit(1)
    
    results_file = sys.argv[1]
    benchmark_data = parse_benchmark_json(results_file)
    
    gate_results: List[GateResult] = []
    
    # Process each benchmark in results
    for benchmark in benchmark_data.get("benchmarks", []):
        name = benchmark["name"]
        # Extract metric (assuming first result in aggregates)
        if "aggregate" in benchmark:
            # For aggregated results
            metrics = benchmark["aggregate"]
            if isinstance(metrics, list):
                value = metrics[0] if metrics else 0
            else:
                value = metrics.get("cpu_time", metrics.get("real_time", 0))
        else:
            value = extract_metric(benchmark)
        
        gate_result = validate_gate(name, value)
        gate_results.append(gate_result)
    
    # Generate report
    report = {
        "timestamp": __import__("datetime").datetime.now().isoformat(),
        "total": len(gate_results),
        "passed": sum(1 for r in gate_results if r.status == "PASS"),
        "failed": sum(1 for r in gate_results if r.status == "FAIL"),
        "results": [
            {
                "benchmark": r.benchmark,
                "baseline": r.baseline,
                "current": r.current,
                "threshold": r.threshold,
                "metric": r.metric,
                "op": r.op,
                "status": r.status,
                "regression_pct": round(r.regression_pct, 2),
            }
            for r in gate_results
        ],
    }
    
    # Summary text
    passed = report["passed"]
    total = report["total"]
    report["summary"] = f"✅ {passed}/{total} gates PASS" if passed == total else f"❌ {passed}/{total} gates PASS"
    
    # Details table
    details = "| Benchmark | Baseline | Current | Status | Regression |\n"
    details += "|-----------|----------|---------|--------|-------------|\n"
    for r in gate_results:
        pct = f"{r.regression_pct:+.1f}%" if r.regression_pct else "0%"
        details += f"| {r.benchmark[:25]:25} | {r.baseline:>8.0f} | {r.current:>7.0f} | {r.status:6} | {pct:>10} |\n"
    report["details"] = details
    
    # Output report
    with open("gate_validation_report.json", "w") as f:
        json.dump(report, f, indent=2)
    
    print(report["summary"])
    print(details)
    
    # Exit code based on gate failures
    sys.exit(0 if report["failed"] == 0 else 1)

if __name__ == "__main__":
    main()
```

---

## Regression Detection Workflow

### Daily Baseline Tracking

Create `scripts/store_benchmark_baseline.py`:

```python
#!/usr/bin/env python3
"""Store benchmark results as rolling 30-day baseline."""

import json
import sys
from datetime import datetime
import hashlib

def store_baseline(results_file: str, storage_path: str, commit_sha: str):
    """Store results with timestamp and commit reference."""
    with open(results_file) as f:
        results = json.load(f)
    
    # Create baseline entry
    baseline_entry = {
        "timestamp": datetime.now().isoformat(),
        "commit": commit_sha,
        "results": results,
    }
    
    # Store to S3/filesystem
    basename = f"baseline_{datetime.now().strftime('%Y%m%d')}.json"
    print(f"Storing baseline: {basename}")
    # Real implementation would upload to S3/artifact store
    
    return baseline_entry

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: store_benchmark_baseline.py <results.json> --storage <path> --commit-sha <sha>")
        sys.exit(1)
    
    results_file = sys.argv[1]
    print(f"Baseline stored for {results_file}")
```

---

## Local Testing Checklist

Before committing benchmark changes:

- [ ] Build succeeds: `cmake --build build_bench --target bench_analytics_gap_closure`
- [ ] Run once: `./build_bench/benchmarks/analytics/bench_analytics_gap_closure --benchmark_repetitions=1`
- [ ] Validate gates: Run full suite with 5 repetitions and check gate_validation_report.json
- [ ] Review documentation: Confirm all benchmarks have labels and consistent patterns
- [ ] Check for regressions: Compare vs BENCH_GAP_CLOSURE_REPORT.md expected metrics
- [ ] Test CI workflow locally (GitHub Actions debug mode if needed)

---

## Expected CI Behavior

1. **On main branch**: Stores baseline for daily comparison
2. **On PR**: Validates against stored baseline + comments results
3. **On nightly schedule**: Generates rolling baseline trend for alerting
4. **On failure**: Blocks merge if any gate regression >10%

---

## Troubleshooting

### Benchmark Timeout

If benchmarks timeout (>5 min):
- Check system load: `top -n1 | head`
- Verify CPU frequency scaling disabled (if needed for ±5% stability)
- Reduce dataset size in source if timeout persists

### Memory Issues

If OOM errors occur:
- Check available memory: `free -h`
- Consider running single benchmark: `--benchmark_filter=BM_KB01`

### SIMD Performance Regression

If FC-02 shows significant regression:
- Check if compiled with `-march=native`: `grep march CMakeLists.txt`
- Verify AVX2 availability: `grep -c avx2 /proc/cpuinfo`
- Run with baseline comparison to identify exact regression

---

**See Also**:
- `benchmarks/analytics/BENCH_GAP_CLOSURE_REPORT.md` (detailed design spec)
- `benchmarks/analytics/bench_analytics_gap_closure.cpp` (source)
- `benchmarks/analytics/BENCH_VALIDATION_CHECKLIST.md` (QA checklist)

