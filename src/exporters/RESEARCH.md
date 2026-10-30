---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Exporters Research Notes

Exact model backend not verifiable from final document text alone.

## Purpose and evidence boundary

This note records how maintainers should evaluate export behavior and what the
current code actually measures. The scope is `src/exporters` and its public
contracts in `include/exporters`; it does not claim that a particular
benchmark, production workload, or external dataset has passed evaluation.
For runtime ownership see [ARCHITECTURE.md](./ARCHITECTURE.md); for current
mapped performance cases see [PERFORMANCE_EXPECTATIONS.md](./PERFORMANCE_EXPECTATIONS.md).

## Questions and assumptions

Exporter evaluation should answer whether the same inputs and format version
produce repeatable artifacts, whether streaming preserves order and bounded
memory behavior, and whether filtering and safety controls act before data
leaves the process. The API contract defines byte-for-byte determinism for
repeated export calls on the same platform, ordered stream chunks, and
format-specific null handling. Cross-platform byte identity is not promised by
that contract.

The pipeline receives `BaseEntity` records and `ExportOptions`; source query
selection and the semantics of values are upstream responsibilities. Export
format, field selection, filters, schema hints, compression, encryption, and
execution mode change the artifact and must be fixed when comparing results.

## Evaluation method

1. Select the exporter, representative entity fixtures, exact configuration,
   format version, and platform. Keep test data deterministic and non-sensitive.
2. Verify output structure and values, including nulls, missing fields,
   escaping, selected/excluded columns, and filtering.
3. Repeat identical exports and compare bytes where the contract requires
   determinism. For streaming, check sequence/order and reconstruct the output
   by concatenating chunks in order.
4. Exercise failure boundaries: unsupported format before output, schema or
   template mismatch, write/size-limit failure, policy denial, checkpoint or
   watermark persistence failure, and Hub HTTP errors.
5. Measure throughput, bytes, latency, error counts, and peak memory using the
   established exporter tests and benchmark cases. Compare against the same
   release baseline and run configuration; do not infer a production SLO from
   a single test or a code default.

`ExporterMetrics` exposes export counts, bytes, latency summaries, errors,
checkpoint/delta counters, PII and encryption counters, policy denials, and
Hub upload failures. Its latency histogram uses coarse buckets, so it is useful
for regression signals, not a substitute for high-resolution external
latency distributions.

## Evidence and limits

- Contract source: `include/exporters/exporters_api_contract.h`.
- Public option and statistics surface: `include/exporters/exporter_interface.h`.
- Focused coverage: `tests/exporters/` and `tests/integration/test_exporters_soak.cpp`.
- Mapped benchmark entry points: `benchmarks/bench_exporters.cpp` and
  `src/exporters/PERFORMANCE_EXPECTATIONS.md`.
- Hugging Face uploads depend on external HTTP behavior; local unit tests do
  not establish Hub availability or remote service performance.
- PII detection is pattern-based and configurable. A clean detector result is
  not proof that an artifact contains no sensitive information.

## Review checklist

Record the exact exporter/configuration, fixture dimensions, build features
(especially Arrow availability), test command and result, benchmark baseline,
and artifact comparison method. Report failures and missing evidence instead
of describing unrun evaluation as passed.

Related: [MODELING.md](./MODELING.md), [EMBEDDING.md](./EMBEDDING.md),
[IMPLEMENTATION.md](./IMPLEMENTATION.md).
