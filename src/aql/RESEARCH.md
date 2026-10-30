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

Exact model backend not verifiable from final document text alone.

# AQL Assistance Research and Evaluation

## Purpose and scope

This page records the evidence and evaluation limits for AQL assistance
maintainers. It covers language-to-AQL generation, candidate validation, and
few-shot retrieval in `src/aql/`. It does not describe the core AQL execution
engine or claim that a generated query is authorized or correct for a user's
intent.

## Problem and assumptions

Natural-language translation depends on the configured LLM, its prompt and
schema context, and optional examples. Therefore, equal requests are not
guaranteed to produce identical queries across providers or invocations.
Parser acceptance is evidence of syntactic validity, not proof of semantic
correctness, data-access permission, or intent fidelity.

The module exposes several distinct quality signals:

- `LLMAQLHandler` can validate a generated query through the configured
  `query::AQLParserService`.
- `AQLQueryValidator` can check query strings or builders against supplied
  collection metadata.
- `AQLConfidenceScorer` and `translateNLToAQLWithConfidence()` provide a
  confidence/diagnostic surface; this score is not a calibrated probability.
- `AQLFewShotExampleLibrary` ranks examples lexically by default and can use
  an injected embedding provider for semantic ranking.

These are separate checks; success at one stage does not imply success at all
the others.

## Evaluation method and evidence

Use the existing test and benchmark paths rather than inferring quality from
successful translation alone:

1. Run focused regression cases for validation, translation recovery,
   confidence scoring, and few-shot retrieval. The module test inventory is
   in [`TESTING_COVERAGE.md`](TESTING_COVERAGE.md); representative tests are
   [`test_aql_translation_recovery.cpp`](../../tests/aql/test_aql_translation_recovery.cpp),
   [`test_aql_query_validator.cpp`](../../tests/aql/test_aql_query_validator.cpp),
   and [`test_aql_fewshot_example_library.cpp`](../../tests/aql/test_aql_fewshot_example_library.cpp).
2. Measure latency and throughput only with the mapped AQL benchmarks and
   hardware conditions in
   [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md). The
   `bench_aql_translation` and `bench_aql_helper_paths` targets use
   self-contained mock paths; their results measure local assistance
   overhead, not a live provider's latency or translation accuracy. The
   checked-in baselines are dated evidence, not a substitute for a current run
   on the target build and hardware.
3. No canonical provider/model comparison corpus or quality-evaluation
   executable is defined by this module. For a reproducible comparison, first
   version a held-out query set with expected intent, schema context, and
   reviewed acceptance criteria; record its revision and the full provider
   configuration with the results. Until that exists, the repository does
   not support a comparative model-accuracy claim.
4. For any provider/model comparison, hold the request set, schema, examples,
   validation mode, and provider settings constant; record invalid-query
   rate, parser rejection categories, retry count, confidence distribution,
   latency percentiles, and provider failures. Include a human review of
   intent fidelity because parser success cannot measure it.

The repository's test fixtures exercise controlled provider failures and
schema variations. They are useful for deterministic behavior checks, not a
representative production workload or a published model-accuracy data set.
In a test-enabled CMake build, the focused targets are named
`module_aql_test_aql_translation_recovery_focused` and
`module_aql_test_aql_query_validator_focused`; CTest registers
`test_aql_translation_recovery_AqlFocusedTests` and
`test_aql_query_validator_AqlFocusedTests`.

## Trade-offs and limitations

- More retries can increase latency and provider load; they do not guarantee
  a correct query.
- Embedding-ranked examples require a functioning provider and compatible
  vectors. The documented fallback is lexical Jaccard ranking, which has
  different relevance behavior.
- Confidence values should be treated as a review signal only. The frozen
  assistance contract flags scores below `0.5` for discard or human review;
  that threshold is not a safety boundary.
- AQL parser and schema checks do not replace execution-time authorization,
  parameter handling, or policy enforcement.

## References

- [`AQL_ASSISTANCE_CONTRACT.md`](AQL_ASSISTANCE_CONTRACT.md) — frozen
  translation and validation semantics.
- [`ARCHITECTURE.md`](ARCHITECTURE.md) — module boundaries and integration
  flow.
- [`src/query/AQL_LLM_INTEGRATION_CONTRACT.md`](../query/AQL_LLM_INTEGRATION_CONTRACT.md)
  — parser integration contract.
- [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md) and
  [`ROADMAP.md`](ROADMAP.md) — planned work and historical performance
  evidence.
