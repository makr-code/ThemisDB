> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Benchmark Baselines

This directory stores baseline benchmark results for performance regression detection.

## Structure

```
baselines/
├── main/              # Baselines from main branch
│   └── latest.json    # Latest baseline from main
├── releases/          # Baselines from tagged releases
│   ├── v1.4.0.json
│   └── v1.4.1.json
└── develop/           # Baselines from develop branch
    └── latest.json
```

## Baseline Format

Baselines are stored as JSON files with the following structure:

```json
{
  "version": "1.4.1",
  "branch": "main",
  "commit": "abc123def456",
  "timestamp": "2024-12-30T10:00:00Z",
  "benchmarks": {
    "BenchmarkName": {
      "real_time": 1000.0,
      "cpu_time": 950.0,
      "iterations": 10000,
      "items_per_second": 50000.0,
      "bytes_per_second": 100000.0
    }
  }
}
```

## Usage

Baselines are automatically created and updated by the CI pipeline:
- On main branch: Updates `main/latest.json`
- On release tags: Creates `releases/vX.Y.Z.json`
- On develop branch: Updates `develop/latest.json`

For manual baseline management, use:
```bash
python benchmarks/baseline_manager.py --help
```
