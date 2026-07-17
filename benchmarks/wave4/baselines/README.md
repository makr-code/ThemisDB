# Wave 4 Baselines

This directory stores versioned Google Benchmark JSON output files used as
performance baselines for Wave 4 release gate comparison.

## Naming Convention

```
baseline_<version>.json
```

Examples:
- `baseline_v1.5.0.json` — W4A release gates baseline for v1.5.0

## Recording a New Baseline

```bash
# Build in release mode
cmake --preset linux-release
cmake --build --preset linux-release --parallel 16

# Run W4A with high repetitions for a stable baseline
./build/linux-release/bin/benchmarks/bench_w4a_release_gates \
  --benchmark_repetitions=10 \
  --benchmark_out=benchmarks/wave4/baselines/baseline_v1.5.0.json \
  --benchmark_out_format=json

git add benchmarks/wave4/baselines/baseline_v1.5.0.json
git commit -m "chore(bench): record W4A baseline for v1.5.0"
```

## Policy

- One baseline per release version.
- Baseline is updated only when a deliberate performance change is approved
  or the CI runner hardware is upgraded.
- The active baseline for `develop` is referenced in `release_gate_manifest.json`.

See `benchmarks/wave4/RUNBOOK.md` for the full baseline strategy.
