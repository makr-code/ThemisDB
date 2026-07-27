# AdaLoRA↔TT Bridge — Experiments

This directory contains the reproducible benchmark artifacts for the
AdaLoRA↔Tensor-Train (TT) bridge publication claims.

## Structure

```
adalora_tt_bridge/
├── RESULT_SCHEMA.md          — Human-readable result schema specification
├── result_schema.json        — JSON Schema (validates summary.json files)
├── run_config_template.json  — Canonical run configuration template
├── collect_results.py        — Result collection and statistical reporting tool
└── results/
    └── <YYYY-MM-DD>_<track-id>_<variant>_<env-hash>/
        ├── env.json           — Environment descriptor
        ├── raw/               — Raw Google Benchmark JSON output files
        ├── summary.json       — Computed statistics (validated by result_schema.json)
        └── README.md          — Run notes
```

## Quick start

```bash
# 1. Capture environment
mkdir -p results
python3 -c "
import json, subprocess, sys
env = {
    'env_schema_version': '1.0',
    'hardware': {'cpu_model': 'fill-in', 'ram_gb': 'fill-in'},
    'software': {
        'os': 'fill-in', 'compiler': 'fill-in',
        'themisdb_git_sha': subprocess.check_output(['git','rev-parse','HEAD']).decode().strip(),
        'themisdb_branch': subprocess.check_output(['git','rev-parse','--abbrev-ref','HEAD']).decode().strip()
    }
}
print(json.dumps(env, indent=2))
" > results/env.json

# 2. Build
cmake --preset linux-release
cmake --build --preset linux-release --parallel 8 \
  --target bench_adalora_tt_bridge_latency

# 3. Run
./build-linux-release/benchmarks/bench_adalora_tt_bridge_latency \
  --benchmark_repetitions=30 \
  --benchmark_out=results/bt1_raw.json \
  --benchmark_out_format=json

# 4. Collect and validate
python3 collect_results.py \
  --input results/bt1_raw.json \
  --track bt1 \
  --variant cold_load_rank8 \
  --env results/env.json \
  --git-sha $(git rev-parse HEAD) \
  --warmup-protocol-followed \
  --output results/$(date +%F)_bt1_cold_load_rank8_$(sha256sum results/env.json | cut -c1-8)/summary.json
```

## Protocol reference

See `research/ADALORA_TT_BRIDGE_BENCHMARK_PROTOCOL.md` for the full protocol,
including experiment matrix, validity requirements, and reporting contract.

`collect_results.py` currently supports `bt1` and `bt4` collection flows only.
`bt2`/`bt3` require track-specific counters not derivable from latency samples alone.

## Status

| Track | Runnable | Blocked on |
|-------|----------|-----------|
| BT-1 (load latency) | ✅ | — |
| BT-2 (dedup efficiency) | ✅ | — |
| BT-3 (rank/quality) | ✅ / partial | BT-3-C: external downstream task |
| BT-4 (FLARE switching) | ❌ | GGML bridge Stub #271 + FLARE integration |

<!-- Status: scaffolding-complete | awaiting first runs | validated: 2026-07-27 -->
