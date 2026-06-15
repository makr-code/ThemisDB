# CI Gate Specification — Benchmark Pipeline

**Scope:** Integration of `benchmarks/scripts/scientific_evaluation_framework.py` and
`benchmarks/scripts/audit_benchmark_registration.py` as mandatory CI gate jobs.

**Status:** Reference specification. Active GitHub workflow definitions require
`.github/workflows/` entries; the repository currently stores workflow definitions
outside the active path (`.github/no_workflows`). This document specifies the
intended gate contract so downstream pipelines and future CI activation can
implement it without ambiguity.

---

## Gate 1 — Benchmark Registration Integrity

**Tool:** `benchmarks/scripts/audit_benchmark_registration.py`

**Purpose:** Ensure every C++ benchmark source file is registered in CMake.
Prevents silent dead-code that builds locally but is never measured.

**Trigger:** On every PR that touches `benchmarks/**/*.cpp` or `benchmarks/**/CMakeLists.txt`.

**Command:**

```bash
python3 benchmarks/scripts/audit_benchmark_registration.py
```

**Pass/Fail contract:**

| Exit code | Meaning | CI action |
|-----------|---------|-----------|
| `0` | All sources registered (or intentionally excluded) | ✅ Pass |
| `1` | Unregistered source(s) found | ❌ Fail — block merge |

**Expected output (clean):**

```
Benchmark sources discovered: N
Registered in CMake: N-1
Intentionally excluded: 1
Unregistered: 0
```

**Intentional exclusions** are declared in the `INTENTIONAL_EXCLUSIONS` set inside
the script. Any new exclusion must be explicitly added there with a comment explaining
the reason for exclusion.

---

## Gate 2 — Scientific Evaluation Framework

**Tool:** `benchmarks/scripts/scientific_evaluation_framework.py`

**Purpose:** Validate that benchmark experiments satisfy the defined performance
budget and no regressions are introduced. Auto-generates regression tickets on
budget violation.

**Trigger:** On benchmark CI runs (nightly or explicitly triggered on performance-
sensitive PRs).

**Command:**

```bash
python3 benchmarks/scripts/scientific_evaluation_framework.py \
  --input benchmarks/ci_experiments.json \
  --output benchmarks/results/ci_report.json \
  --tickets-output benchmarks/results/ci_regression_tickets.json
```

**Input contract (`ci_experiments.json`):**

```json
{
  "baseline_freeze": {
    "compiler": "clang-17",
    "compiler_flags": "-O3 -march=native -DNDEBUG",
    "preset": "linux-release",
    "hardware_profile": "ci-runner-standard",
    "os_image": "ubuntu-22.04"
  },
  "metric": "latency_p99_ms",
  "hypothesis": {
    "direction": "lower_is_better",
    "effect_description": "P99 latency does not increase vs baseline",
    "risk": "regression",
    "stop_criteria": "budget_violation"
  },
  "scenario": {
    "workload_family": "oltp",
    "dataset": "themis-tpc-c-10k",
    "query_profile": "mixed-rw",
    "concurrency": 16,
    "warmup_iterations": 100,
    "measurement_iterations": 1000,
    "interference_factors": {
      "caching": "warm",
      "numa": "single-socket",
      "io": "local-nvme",
      "gpu": "none"
    }
  },
  "experiments": [
    {
      "name": "baseline",
      "samples": [/* n >= 30 measurements */],
      "performance_budget_percent": 5.0
    },
    {
      "name": "treatment",
      "samples": [/* n >= 30 measurements */]
    }
  ]
}
```

**Output contract (`ci_report.json`) — key fields:**

```json
{
  "summary": {
    "classification": "neutral | regressiv | signifikant_positiv",
    "gate_violations": 0,
    "p_value": 0.42,
    "cohens_d": 0.05,
    "cliffs_delta": 0.03
  },
  "results": [ ... ]
}
```

**Pass/Fail contract:**

| `summary.gate_violations` | `summary.classification` | CI action |
|--------------------------|--------------------------|-----------|
| `0` | any | ✅ Pass |
| `> 0` | `regressiv` | ❌ Fail — block merge, attach regression tickets |
| `> 0` | `neutral` | ⚠️ Warning — gate depends on policy |

**Regression ticket output** (`ci_regression_tickets.json`) is uploaded as a
CI artifact for triage; it does not automatically create GitHub Issues (planned
enhancement, see `benchmarks/scripts/ROADMAP.md`).

---

## Gate Composition

For a standard PR touching performance-sensitive C++ code, both gates run:

```
[Gate 1: Registration Integrity]  →  pass/fail (fast, < 5s)
[Gate 2: Scientific Evaluation]   →  pass/fail (nightly or on-demand, 2-5 min)
```

Gate 1 is always required. Gate 2 may be skipped for non-performance PRs (e.g.,
doc-only changes) by labelling the PR `skip-bench-gate`.

---

## Reproducibility Requirements

Per the framework contract, CI gate runs MUST:

1. Fix the random seed (`--seed` or `BENCHMARK_SEED` env var — see framework internals).
2. Pin `baseline_freeze` fields to the exact CI runner image and compiler.
3. Archive the full `ci_report.json` and `ci_regression_tickets.json` as build artifacts.
4. Not mix warm-up measurements into the `samples` array.

Minimum sample size: **n ≥ 30** per baseline and treatment group.
Any run below this threshold is rejected by the framework with an explicit error.

---

## Activation Instructions

When GitHub Actions workflows become available in this repository:

1. Create `.github/workflows/benchmark-gate.yml` referencing the commands above.
2. Set `on: pull_request` with `paths` filter for `benchmarks/**`.
3. Add `on: schedule` (nightly) for Gate 2.
4. Store `ci_experiments.json` in `benchmarks/ci/` and version it with the codebase.
5. Upload `ci_report.json` and `ci_regression_tickets.json` via `actions/upload-artifact`.

See `benchmarks/scripts/FUTURE_ENHANCEMENTS.md` for planned Ticket-Dispatcher integration.
