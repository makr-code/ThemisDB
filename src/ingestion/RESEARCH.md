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

# Ingestion Research and Evaluation

## Purpose and scope

This page records the evidence and evaluation method for ingestion behavior that
can be checked against this repository. It is for module developers and
reviewers; it is not a claim that every connector or deployment has been
benchmarked. Runtime contracts are defined in
[`ARCHITECTURE.md`](./ARCHITECTURE.md),
[`ingestion_api_contract.h`](../../include/ingestion/ingestion_api_contract.h),
and the public interfaces under [`include/ingestion/`](../../include/ingestion/).

## Questions and assumptions

The evaluation concerns four observable questions:

1. Does source orchestration report successful and failed documents separately?
2. Are validation, checkpoint, quarantine, and retry outcomes explicit?
3. Do workflow and connector failures stop or reject work rather than silently
   appearing as successful extraction?
4. Can performance changes be compared on the same benchmark case and baseline?

Evaluation inputs are connector-specific. A filesystem walk, a paginated HTTP
endpoint, a Kafka topic, and an object store have different completeness and
availability conditions; their results are not directly comparable without
holding source content, connector options, and downstream sinks constant.
`SourceConfig::options` also includes limits where `0` means unlimited for some
connectors, so test fixtures and production profiles must set deliberate bounds
for untrusted or unbounded sources.

## Evidence and methods

| Property | Evidence in this repository | Review method |
|---|---|---|
| Error classes | [`ingestion_error_contract.h`](../../include/ingestion/ingestion_error_contract.h), [`ingestion_api_contract.h`](../../include/ingestion/ingestion_api_contract.h) | Check classification and the exact error contract used by the caller. |
| Connector and lifecycle outcomes | [`tests/ingestion/`](../../tests/ingestion/) | Run focused connector, retry, checkpoint, schema, and quarantine regressions. |
| Reindex triggers | [`test_reindex_decision_engine.cpp`](../../tests/ingestion/test_reindex_decision_engine.cpp) | Verify each metadata-change trigger and unchanged-metadata behavior. |
| Hot-path targets | [`bench_ingestion_release_gates.cpp`](../../benchmarks/ingestion/bench_ingestion_release_gates.cpp) | Compare the six named cases against their release thresholds and baseline. |
| Extraction and quality paths | [`bench_ingestion_extraction.cpp`](../../benchmarks/ingestion/bench_ingestion_extraction.cpp), [`bench_ingestion_quality_judge.cpp`](../../benchmarks/ingestion/bench_ingestion_quality_judge.cpp) | Separate deterministic fallback/stub measurements from a configured live model. |

The release-gate benchmark explicitly uses an in-memory mock WAL and string
serialization for its write/batch cases. Its thresholds are useful regression
targets for those measured operations, but they do not establish end-to-end
database durability, connector throughput, or production capacity. Likewise, a
scripted quality-judge backend measures parsing and orchestration, not model
quality.

## Evaluation protocol

For a behavior change, keep a reproducible source fixture and connector options,
then verify the outcome at the narrowest applicable layer:

1. Run the focused `tests/ingestion/test_*.cpp` regression for that interface.
2. For error handling, assert the returned structured code and whether the
   document is processed, failed, or quarantined; do not infer success from a
   non-empty report alone.
3. For restart behavior, test checkpoint read/write and replay boundaries using
   the checkpoint implementation configured by the test.
4. For performance, run the mapped release-profile benchmark and compare the
   same case, hardware class, repetitions, and baseline. Do not compare a mock
   backend to a live remote connector as if they were equivalent.
5. Record whether an inference backend was available. An unavailable quality
   backend can produce an unevaluated, fail-open report, so its `passed` flag
   alone is not evidence that model-based quality criteria were evaluated.

## Limits and open evidence

- The quality dimensions and default thresholds are configuration values, not a
  published calibration against a labeled ingestion corpus.
- Benchmarks do not cover every connector, deployment topology, or sustained
  production load.
- `RESEARCH.md` does not promote the design examples in
  `EMBEDDING_VERSION_GOVERNANCE.md` into runtime behavior; see
  [`EMBEDDING.md`](./EMBEDDING.md) for what the current decision engine actually
  implements.
- There is no module-specific compiled evidence bundle under
  `ai_context/developer_llm_wiki/modules/ingestion/` in this checkout. The
  source, tests, and benchmarks linked above are the verification sources for
  this page.

## References

- [`ROADMAP.md`](./ROADMAP.md) — active work and acceptance state.
- [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md) — benchmark
  mapping and module-level regression expectations.
- [`MODULE_GUIDE.md`](./MODULE_GUIDE.md) — focused test and benchmark entry points.

> Exact model backend not verifiable from final document text alone.
