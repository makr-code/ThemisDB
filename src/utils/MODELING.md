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

# Utils Detection Data Model

## Purpose

This page describes the concrete data and rule model used by the utils
privacy-detection APIs. It is for developers changing an engine, consuming
findings, or reviewing detection limitations. It does not describe a trained
language model: the current NER implementation is rule/gazetteer based.

## Finding contract

[`PIIFinding`](../../include/utils/pii_detection_engine.h) is the common
result record. It carries a `PIIType`, matched `value`, `start_offset` and
`end_offset`, a confidence value, a pattern name, and the engine name. Offsets
are calculated against the input `std::string`; when slicing UTF-8 input,
treat them as string byte positions, not Unicode code-point indexes. A
confidence value is a per-engine score, not a globally calibrated
probability.

`PIIType` distinguishes structured types (`EMAIL`, `PHONE`, `SSN`,
`CREDIT_CARD`, `IBAN`, `IP_ADDRESS`, `URL`) from rule-detected entities
(`PERSON_NAME`, `LOCATION`, `ORGANIZATION`) and `UNKNOWN`. The orchestrator
merges enabled-engine findings, sorts by offset, and deduplicates them.

## Detection methods

| Method | Implemented behavior | Main constraints |
|---|---|---|
| Regex | Configurable compiled patterns, field-name hints, optional validation such as Luhn, and redaction recommendations | Header defaults include a 10 MiB input cap, 500-character pattern cap, and 5,000 ms pattern-batch budget; malformed/unsafe patterns can be rejected and timeouts can return partial results |
| NER | Token-based rules for names, organizations, and locations using honorifics, suffixes, contextual prepositions, and gazetteer sets | Gazetteer lists are bounded; initialized data must be available; language coverage and heuristic accuracy depend on configured lists |
| Streaming | `PIIStreamScanner` retains lookahead and tracks bytes processed across chunks | Configure lookahead to cover the longest entity span; finalize the last chunk with `is_last=true` |
| Pseudonymization | Detection findings map to encrypted original values and pseudonym identifiers in the configured store | Key/storage errors must not be treated as proof that plaintext is safe; inspect write results and recovery behavior |

The source contracts are in
[`pii_detection_engine.h`](../../include/utils/pii_detection_engine.h),
[`regex_detection_engine.h`](../../include/utils/regex_detection_engine.h),
[`ner_detection_engine.h`](../../include/utils/ner_detection_engine.h), and
[`pii_pseudonymizer.h`](../../include/utils/pii_pseudonymizer.h).

## Validation and edge cases

Tests should assert exact type, span, confidence policy where specified,
deduplication, and behavior for invalid UTF-8, empty input, repeated values,
overlapping findings, maximum-size input, timeout, unavailable gazetteers,
reload failure, and chunk boundaries. Test files are listed in
[`RESEARCH.md`](RESEARCH.md) and registered in
[`tests/utils/CMakeLists.txt`](../../tests/utils/CMakeLists.txt).

Do not silently convert “engine failed” into “no PII.” Direct
`PIIDetector::detectInText` logs engine exceptions and continues with other
engines, while the streaming scanner has explicit fail-closed exception
handling. Callers must select the contract appropriate to their data path and
must not interpret an empty result as an authorization decision.

## Model limitation

`EmbeddingDetectionEngine` is mentioned in interface comments, but no
instantiable embedding implementation is present in the current factory.
There are no embedding evaluation metrics in the module's benchmark mapping.
See [`EMBEDDING.md`](EMBEDDING.md); do not rely on semantic similarity
detection in production integrations.
