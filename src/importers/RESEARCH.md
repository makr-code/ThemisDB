---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "include/importers/importers_api_contract.h; tests/importers/test_importers_contract_hardening_focused.cpp; tests/importers/CMakeLists.txt; benchmarks/importers/bench_importers_release_gates.cpp; benchmarks/importers/CMakeLists.txt; src/importers/PERFORMANCE_EXPECTATIONS.md"
Related_Reference: "makr-code/ThemisDB#7353"
---

> Exact model backend not verifiable from final document text alone.

# Importers Research and Evaluation

This page describes how importer behavior is evaluated and what the available
evidence can—and cannot—establish. It is for maintainers comparing connector,
schema, conflict, and validation changes. Runtime contracts remain defined by
the public headers, especially
[`importers_api_contract.h`](../../include/importers/importers_api_contract.h).

## Questions and assumptions

Evaluation should answer whether an importer:

- preserves source row order and reports failures with stable locations;
- handles unchanged, additive, and breaking schema changes according to the
  contract;
- applies configured bad-row and conflict dispositions without silently
  changing the result;
- remains bounded when row size, quotas, connector capability, or cancellation
  limits are reached.

The shared contract states that row order is preserved, breaking schema
changes require migration, and bad rows can be skipped or fail the import.
Connector support and runtime flags vary, so a result for one importer does not
establish parity across all connectors.

## Evaluation evidence

[`test_importers_contract_hardening_focused.cpp`](../../tests/importers/test_importers_contract_hardening_focused.cpp)
covers the IMCH-01..16 contract cases: idempotency, schema evolution, bad-row
handling, commit visibility, row order, quota, and retry classification. Its
header states that external interactions are mocked and the suite is
self-contained; it is contract-regression evidence, not a live connector test.

[`bench_importers_release_gates.cpp`](../../benchmarks/importers/bench_importers_release_gates.cpp)
declares six hot-path cases with target labels: CSV parsing at 5M rows/s, schema validation at
p99 50 µs, duplicate checks at p99 100 µs, mock row-buffer commit at p99 5 ms,
quota checks at p99 50 µs, and schema evolution checks at p99 200 µs. The
benchmark source uses in-memory/mock work and sets five repetitions with 200
explicit warmup calls in each benchmark body. The source does not implement a
threshold assertion or compute p99. Treat these labels as intended targets,
not passing CI gates, measured p99 values, or end-to-end connector results.

The broader
[`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md) maps additional
connector benchmark families. Use benchmark output and the matching run
configuration to claim a measured result; source thresholds alone are not
proof that a current build passed them.

## Reproducible review method

1. Select the affected connector and failure mode; do not infer one connector's
   behavior from another's tests.
2. Run the focused contract or connector regression relevant to the change.
3. For performance claims, run the matching benchmark in the intended build
   profile and retain its output, seed, repetitions, and machine context.
4. Compare result counters and structured errors as well as elapsed time;
   skipped or quarantined records are not successful imports.
5. Record whether the scenario used a live source, mock, fixture, or synthetic
   in-memory data.

## Trade-offs and limits

Schema sampling is a bounded view of a source, not a proof that later rows
conform. Strict validation can reject data that permissive import would
otherwise accept; sampling depth and row limits affect both cost and coverage.
Batch size trades memory and per-batch overhead against throughput. Conflict
strategies change record semantics and must be part of the test case, not an
unstated benchmark default.

The contract tests intentionally model the coordinator and do not replace
connector integration, failure-injection, or recovery tests. Their mock rejects
a repeated `import_id`, whereas the contract header describes idempotent replay.
This API/test discrepancy is unresolved (TODO: reconcile outside this
documentation-only change). Consult the
[focused-test registration](../../tests/importers/CMakeLists.txt), the
[module roadmap](./ROADMAP.md), and the connector-specific suites before
making a release or compatibility claim.
