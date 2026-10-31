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

# Embedding Detection Status

## Purpose and current status

This page prevents consumers from mistaking references to semantic or
embedding-based PII detection for an available implementation. It is for
developers evaluating PII detection options and integrating the utils
factory.

The current source does **not** provide an instantiable
`EmbeddingDetectionEngine`. In
[`pii_detection_engine.cpp`](pii_detection_engine.cpp),
`PIIDetectionEngineFactory::createUnsigned` constructs only `regex` and
`ner`; the embedding branch is commented as a future engine. The same file
conditionally adds the string `"embedding"` to `getAvailableEngines()` when
`THEMIS_ENABLE_EMBEDDING` is defined. That listing is not sufficient evidence
that factory creation, initialization, detection, or evaluation works.

The base interface and `PIIDetector` comments mention embedding-oriented
concepts in
[`pii_detection_engine.h`](../../include/utils/pii_detection_engine.h) and
[`pii_detector.h`](../../include/utils/pii_detector.h), but those comments do
not establish implementation behavior. The concrete available methods are
summarized in [`MODELING.md`](MODELING.md).

## Integration guidance

- Do not configure `"embedding"` as a production engine or rely on it for
  privacy coverage.
- Check factory creation and engine initialization results; do not assume an
  advertised engine name implies it can be instantiated.
- Use the implemented regex or rule/gazetteer NER paths only with their
  documented limits and caller-side failure handling.
- Do not interpret absence of findings as proof that content is non-sensitive.

## Evidence required before claiming support

A future implementation would need a concrete engine class and factory
registration, defined model/configuration loading and trust checks, explicit
input/output and failure contracts, resource and timeout bounds, and tests for
unavailable or malformed model data. Evaluation would need a documented,
labeled dataset and measured precision/recall, false-negative analysis,
latency, memory, and reproducibility details. No such embedding-specific
implementation, test suite, or benchmark is currently identified in the
utils source map.

This is a current implementation-status note, not a roadmap commitment.
Changes to that status must be based on the engine source, tests, and
benchmark evidence rather than interface comments alone.
