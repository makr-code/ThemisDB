---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Utils Research and Evaluation

## Purpose and scope

This note records the evidence and evaluation boundaries for method-heavy
utilities in `src/utils`. It is for maintainers comparing detection approaches
and deciding what evidence a change needs. It is not a research claim that a
particular method has achieved a measured accuracy or latency target.

The relevant methods are regex-based structured PII detection in
[`regex_detection_engine.cpp`](regex_detection_engine.cpp), rule/gazetteer
entity detection in [`ner_detection_engine.cpp`](ner_detection_engine.cpp),
stream-boundary handling in
[`pii_detection_engine.cpp`](pii_detection_engine.cpp), and numeric distance
helpers in [`simd_distance.cpp`](simd_distance.cpp).

## Evidence-backed assumptions

- The regex engine targets structured values such as email, phone, SSN,
  payment-card, IBAN, IP, and URL patterns. It validates configured patterns
  and bounds input size and matching time; a timeout can leave partial findings.
- `NERDetectionEngine` is implemented as rules and configured gazetteers
  (honorifics, organization suffixes, and location prepositions), not as a
  trained neural model. Configuration reload restores prior data after failure.
- `PIIStreamScanner` keeps a byte lookahead window so a finding crossing a
  chunk boundary can be emitted with document-relative offsets. Its result
  depends on the engine's declared maximum span.
- SIMD distance helpers and the privacy scanner have benchmark anchors listed
  in [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md). Those
  mappings identify measurements to run; they do not prove a current release
  result.

## Evaluation method

For detection changes, compare engine output against labeled fixtures and
exercise both expected matches and negatives. Preserve Unicode, malformed
UTF-8, large-input, timeout, malformed-pattern, configuration-reload, and
chunk-boundary cases. Existing focused evidence includes
[`test_regex_detection_engine.cpp`](../../tests/utils/test_regex_detection_engine.cpp),
[`test_ner_detection_engine.cpp`](../../tests/utils/test_ner_detection_engine.cpp),
[`test_utils_pii_unicode_edge_cases.cpp`](../../tests/utils/test_utils_pii_unicode_edge_cases.cpp),
and [`test_utils_privacy_hardening.cpp`](../../tests/utils/test_utils_privacy_hardening.cpp).

For throughput or latency changes, run the matching release-profile benchmark
listed in [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md) against
the same baseline and hardware. Report the benchmark case, input size,
configuration, and p95/p99 or throughput result. The current module docs do
not publish precision/recall measurements or a single cross-utility
performance score; do not infer one from test presence or source comments.

## Trade-offs and limits

Regex patterns are appropriate for structured formats but do not establish
semantic sensitivity. Gazetteer/rule detection can be language- and
configuration-dependent and can produce both misses and false positives.
Chunking lowers peak input retention but makes lookahead configuration part of
correctness. SIMD acceleration is hardware- and workload-dependent; scalar
parity and measured release baselines remain necessary.

No finding from these helpers alone is an authorization decision or proof that
text contains no sensitive information. See [`TRUST.md`](TRUST.md) for the
trust boundary and [`MODELING.md`](MODELING.md) for the data model.

## Embedding research boundary

The public PII headers mention an embedding engine, but the current factory
implementation creates only `regex` and `ner` engines; its embedding creation
branch is commented out. The implementation and evidence status is recorded
explicitly in [`EMBEDDING.md`](EMBEDDING.md). Do not treat header references or
an engine-name list as evidence of a working embedding detector.
