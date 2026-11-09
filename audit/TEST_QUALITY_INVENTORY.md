# Test Quality Inventory — Baseline v1

**Author:** ThemisDB Contributors
**Created:** 2026-11-09
**Last Updated:** 2026-11-09
**Status:** review
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot
**Human_Review:** false
**Human_Author:** ThemisDB Contributors

<!--
Exact model backend not verifiable from final document text alone.
Source level: Level 1 — source-derived audit evidence.
SOT domain: module-behavior (test inventory and module test evidence).
Canonical sources: test_quality_baseline_v1.json; tools/ci/test_quality_inventory.py;
  tests/<module>/CMakeLists.txt and linked focused-test sources.
Related issue: #7820. Related milestone: DOC-WEEKLY-2026-W46.
-->

> **Scope and authority:** This is the current audit inventory for issue [#7820](https://github.com/makr-code/ThemisDB/issues/7820). The machine-readable source is [`test_quality_baseline_v1.json`](test_quality_baseline_v1.json), generated from `develop` at source commit `c21f7bba0cb44d58e5ad49f64781a76703b3ee56` on 2026-11-09. The baseline JSON records the generation time and inventory tool hash. This snapshot reports static inventory results; it does not prove that tests build, run, pass, or meet coverage gates.

## Baseline

| Measure | Baseline v1 |
|---|---:|
| `GTEST_SKIP` calls | 2,871 |
| Skip calls with a same-statement string-literal reason | 1,895 |
| Skip calls without an inline literal reason | 976 |
| Conservative lexical vacuous-assertion candidates | 346 (manual review required) |
| Test source files | 4,258 |
| Static test macro declarations | 88,906 |
| Legacy C++ source files under `tests/legacy/` | 1,018; all pending per-file disposition |
| Coverage | Unavailable |

The legacy inventory includes **all C++ source files under `tests/legacy/`**, including helper/support files and files whose names do not begin with `test_`; all 1,018 remain pending per-file disposition. Examples of included non-`test_` sources are [`mock_signing_service.cpp`](../tests/legacy/binary/mock_signing_service.cpp) and [`runtime_test_plugin.cpp`](../tests/legacy/plugin/runtime_test_plugin.cpp). Every skip record in the JSON has a module or aggregate owner issue link. Ownership is not a justification: the 976 skips without an inline literal reason remain unresolved, and no reason is inferred here. The assertion count is a conservative lexical candidate count, not a finding that all candidates are vacuous.

### Measurement and comparability

- Source LOC means physical C/C++ source/header lines under `src/<module>/`, excluding nested `src/<module>/tests/` trees and including blank and comment lines.
- Case count means declarations of `TEST`, `TEST_F`, `TEST_P`, `TYPED_TEST`, and `TYPED_TEST_P`; parameterized runtime instances are not expanded.
- The refined scanner masks C++ comments before counting `TEST*`, `GTEST_SKIP`, and assertion tokens.
- The prior audit LOC/case figures use non-comparable counting methods. Do not compare them as a trend against this baseline.
- Static declarations and CTest registration do not establish successful build or test execution.

## M1 skip target

For v2.5.0 M1, target date 2027-03-31, each module’s ceiling is **floor(current module skip count / 2)**. Thus a module with one baseline skip has a zero target unless a maintainer-approved exception is added. Every approved skip exception must record an **accountable owner** and an **expiry date**; without both, the exception does not change the module cap. The sum of those per-module ceilings is **1,418**, as recorded by `m1_skip_budget_maximum` in the refreshed JSON; the aggregate maximum is **1,435** (`floor(2,871 / 2)`). Module remediation remains with its existing owner issue; this inventory does not transfer or close module work.

## High-risk module density and current focused-test evidence

Hotspot tuples are source LOC / test files / static case declarations; ratios are source LOC per test file and source LOC per case. CTest names below are those declared by the module CMake registration rules. Their presence is concrete source/registration evidence only—not a claim that the tests were built or executed.

| Module | Baseline tuple; LOC/file; LOC/case | Skip baseline → M1 ceiling | Owner issue | Existing focused CTest registration → source |
|---|---|---|---|---|
| `server` | 101,838 / 24 / 509; 4,243.25; 200.07 | 4 → 2 | [#7543](https://github.com/makr-code/ThemisDB/issues/7543) (also SEC-13) | `test_server_contract_hardening_focused_server_FocusedTests` → [`tests/server/test_server_contract_hardening_focused.cpp`](../tests/server/test_server_contract_hardening_focused.cpp) |
| `distributed_tensor` | 11,676 / 4 / 29; 2,919.00; 402.62 | 0 → 0 | [#7512](https://github.com/makr-code/ThemisDB/issues/7512) | `test_tensor_operations_distributed_tensor_FocusedTests` → [`tests/distributed_tensor/test_tensor_operations.cpp`](../tests/distributed_tensor/test_tensor_operations.cpp) |
| `prompt_engineering` | 16,390 / 7 / 81; 2,341.43; 202.35 | 0 → 0 | [#7538](https://github.com/makr-code/ThemisDB/issues/7538) | `test_prompt_engineering_adversarial_focused_prompt_engineering_FocusedTests` → [`tests/prompt_engineering/test_prompt_engineering_adversarial_focused.cpp`](../tests/prompt_engineering/test_prompt_engineering_adversarial_focused.cpp) |
| `timeseries` | 9,876 / 7 / 142; 1,410.86; 69.55 | 0 → 0 | [#7549](https://github.com/makr-code/ThemisDB/issues/7549) | `test_timeseries_error_path_phase3_TimeseriesFocusedTests` → [`tests/timeseries/test_timeseries_error_path_phase3.cpp`](../tests/timeseries/test_timeseries_error_path_phase3.cpp) |
| `scheduler` | 9,664 / 6 / 60; 1,610.67; 161.07 | 2 → 1 | [#7541](https://github.com/makr-code/ThemisDB/issues/7541) | `test_scheduler_concurrency_edge_cases_scheduler_FocusedTests` → [`tests/scheduler/test_scheduler_concurrency_edge_cases.cpp`](../tests/scheduler/test_scheduler_concurrency_edge_cases.cpp) |
| `whisper` | 1,474 / 4 / 87; 368.50; 16.94 | 0 → 0 | [#7558](https://github.com/makr-code/ThemisDB/issues/7558) | `test_whisper_stub_transcribe_bridge_whisper_FocusedTests` → [`tests/whisper/test_whisper_stub_transcribe_bridge.cpp`](../tests/whisper/test_whisper_stub_transcribe_bridge.cpp) |
| `llama_cpp` | 1,506 / 9 / 132; 167.33; 11.41 | 2 → 1 | [#7526](https://github.com/makr-code/ThemisDB/issues/7526) | `test_inference_execution_llamacpp_FocusedTests` → [`tests/llama_cpp/test_inference_execution.cpp`](../tests/llama_cpp/test_inference_execution.cpp) |
| `analytics` | 34,656 / 41 / 1,189; 845.27; 29.15 | 16 → 8 | [#7500](https://github.com/makr-code/ThemisDB/issues/7500) | `test_analytics_concurrency_safety_focused_AnalyticsFocusedTests` → [`tests/analytics/test_analytics_concurrency_safety_focused.cpp`](../tests/analytics/test_analytics_concurrency_safety_focused.cpp) |

The `distributed_tensor` high-cardinality source [`test_distributed_tensor_highcardinality_stress.cpp`](../tests/distributed_tensor/test_distributed_tensor_highcardinality_stress.cpp) has **no focused CTest registration**; it is not listed as registered evidence above.

### Retained gaps and M1 follow-up by hotspot

The shared TST-02 criterion is a focused error-path and concurrency test for each public API. The actions below identify the module-specific retained gap and the existing focused source to extend or use as a baseline; they do not claim a test has been added or passed. Each action remains with the linked owner issue and targets **v2.5.0 M1 by 2027-03-31**.

| Module | Retained gap / testable follow-up | Owner | Target |
|---|---|---|---|
| `server` | SEC-13 remains open: exercise every registered HTTP/gRPC/WebSocket/GraphQL route with anonymous, wrong-tenant, and missing-scope requests and assert rejection; add the TST-02 per-public-API error/concurrency cases. The existing contract-focused CTest is not the route-wide matrix. | [#7543](https://github.com/makr-code/ThemisDB/issues/7543) (also SEC-13) | v2.5.0 M1 · 2027-03-31 |
| `distributed_tensor` | Test persistence across restart, restart/recovery behavior, and concurrent reads/writes for public APIs. The high-cardinality source is currently unregistered; establish its focused CTest registration before counting it as focused CI evidence. | [#7512](https://github.com/makr-code/ThemisDB/issues/7512) | v2.5.0 M1 · 2027-03-31 |
| `prompt_engineering` | Test input-validation failures, safety-policy rejection, and degraded/fallback paths, including concurrent calls across public APIs. | [#7538](https://github.com/makr-code/ThemisDB/issues/7538) | v2.5.0 M1 · 2027-03-31 |
| `timeseries` | Test retention boundaries and compaction behavior, concurrent ingestion/operations, persistence after restart, and recovery after interruption. | [#7549](https://github.com/makr-code/ThemisDB/issues/7549) | v2.5.0 M1 · 2027-03-31 |
| `scheduler` | Test fairness under competing work, capacity limits, timeout/cancellation races, distributed failure handling, and recovery after failure/restart. | [#7541](https://github.com/makr-code/ThemisDB/issues/7541) | v2.5.0 M1 · 2027-03-31 |
| `whisper` | Test backend/model lifecycle and error paths, preview-mode behavior, and concurrent public-API calls; registration alone does not establish real-backend execution. | [#7558](https://github.com/makr-code/ThemisDB/issues/7558) | v2.5.0 M1 · 2027-03-31 |
| `llama_cpp` | Test model/runtime lifecycle and error paths, and validate real-backend behavior separately from stub/focused registration evidence. | [#7526](https://github.com/makr-code/ThemisDB/issues/7526) | v2.5.0 M1 · 2027-03-31 |
| `analytics` | Manually review the 346 lexical assertion candidates by module, add assertion-quality regressions, and collect line/branch coverage evidence. Coverage remains unavailable until measured. | [#7500](https://github.com/makr-code/ThemisDB/issues/7500) | v2.5.0 M1 · 2027-03-31 |

These registrations are current source/CTest declarations only, not execution evidence. The exact public-API-to-case mapping remains owner follow-up. Unresolved module coverage gates remain open for v3.0.0 M3 sign-off.

## Coverage and release gates

Coverage measurement is unavailable in baseline v1. The proposed thresholds of **at least 70% line coverage and at least 60% branch coverage** are proposals only, pending maintainer approval; they are **not currently enforced**. Unresolved module coverage gates remain open for v3.0.0 M3 sign-off.

The source correction in `tests/transaction/test_transaction_distributed_2pc.cpp` has been applied (`wal.flush()` is called without asserting on its void return); nightly C++ build validation is still pending. The workflow failure is not resolved.

## Canonical references

- [`test_quality_baseline_v1.json`](test_quality_baseline_v1.json) — immutable input for this documentation update; generated by [`tools/ci/test_quality_inventory.py`](../tools/ci/test_quality_inventory.py).
- [`ROADMAP.md`](ROADMAP.md) — audit-derived TST-01, TST-02, and coverage tracking.
- [`AUDIT.md`](AUDIT.md) and [`README.md`](README.md) — canonical audit domain and scope rules.
- [`../ROADMAP.md`](../ROADMAP.md) — canonical root release/milestone status.

## Reproducing baseline v1

The recorded `develop` SHA predates the scanner. Archive only `src/` and `tests/` from that commit, then invoke the scanner from the current PR checkout against the archived source root. The output path below is absolute and points to this repository's baseline artifact:

```sh
repo_root="$(git rev-parse --show-toplevel)"
archive_dir="$(mktemp -d)"
git -C "$repo_root" archive --format=tar \
  c21f7bba0cb44d58e5ad49f64781a76703b3ee56 src tests | tar -x -C "$archive_dir"
/usr/bin/python3 "$repo_root/tools/ci/test_quality_inventory.py" \
  --root "$archive_dir" \
  --source-commit c21f7bba0cb44d58e5ad49f64781a76703b3ee56 \
  --source-branch develop \
  --output "$repo_root/audit/test_quality_baseline_v1.json"
```

The generated JSON records the scanner SHA-256 and generation metadata.
