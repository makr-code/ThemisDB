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

# Toolbox heuristics and data models

**Purpose and audience:** Describes the local helper models and their assumptions for maintainers integrating or evaluating toolbox text-processing behavior. These are deterministic heuristics, not trained models.

## Language detector

`DefaultLanguageDetector` tokenizes with whitespace extraction, lowercases each token, and checks stopword membership for English (`en`) and German (`de`). The default minimum ratio is `0.05`; a language is selected only when its hit ratio is strictly greater than the current best ratio. Inputs with fewer than three tokens return `"und"` before scoring. The candidate with the higher qualifying ratio wins; unsupported, mixed, or technical text can remain `"und"` or be misclassified. See [`language_detector.cpp`](./language_detector.cpp) and [`language_detector.h`](../../include/toolbox/language_detector.h).

The configurable ratio is documented as `[0.0, 1.0]`, but the constructor does not validate it. Callers should use a value in that range and should not rely on an exception for invalid values.

## Text quality score

`TextQualityScore` contains:

- `char_count`: input byte length, not Unicode code points.
- `token_count`: count of whitespace-delimited words after umlaut normalization.
- `language`: the detector result (`"en"`, `"de"`, or `"und"`).
- `is_empty`: true for an empty string or a string consisting only of ASCII space, tab, line feed, or carriage return.
- `has_boilerplate`: true when fewer than five tokens exist, average token length exceeds 25 characters, or repeated-token ratio exceeds 0.50.

The scorer has no calibrated scalar quality value. Its flags are triage signals only; applications must decide whether they justify rejecting or retaining a document. Implementation: [`text_quality_scorer.cpp`](./text_quality_scorer.cpp); regression cases: [`test_toolbox_primitives.cpp`](../../tests/toolbox/test_toolbox_primitives.cpp).

## Chunk and token model

`TextChunker` is a façade over `rag::DocumentSplitter`, not an independent segmentation algorithm. Its default configuration is chunk size 512, overlap 64, and sentence strategy; invalid configurations such as zero chunk size or overlap greater than or equal to chunk size are rejected by the splitter. `estimateTokens()` follows the splitter's configured estimation factor. The `chunkText()` convenience function selects sentence splitting and defaults to the same size and overlap. See [`text_chunker.h`](../../include/toolbox/text_chunker.h).

## Fingerprint model

For non-empty input, the fingerprinter computes SHA-256 over the exact supplied bytes and renders a lowercase 64-character hexadecimal string. `byte_len` is the input byte count; `token_estimate` is rounded `byte_len / 4`. Empty input produces an empty digest and zero estimate. Equality of `ContentFingerprint` compares the digest only; latency and size fields do not affect equality. See [`content_fingerprinter.cpp`](./content_fingerprinter.cpp).

## Evaluation and limitations

Use the primitive tests to protect defined examples, then add representative domain samples before changing thresholds or normalization rules. Evaluation should report confusion cases for short, mixed-language, technical, malformed, and repetitive inputs; do not describe the heuristic outputs as confidence scores. For chunk changes, verify ordering, metadata, configured size/overlap, and empty input through `test_toolbox_primitives`.

No local helper in `src/toolbox` trains or evaluates a vector embedding model. The vector handoff contract and its limits are described in [`EMBEDDING.md`](./EMBEDDING.md).

## References

- [`RESEARCH.md`](./RESEARCH.md)
- [`API.md`](./API.md)
- [`include/toolbox/language_detector.h`](../../include/toolbox/language_detector.h)
- [`include/toolbox/text_quality_scorer.h`](../../include/toolbox/text_quality_scorer.h)
- [`include/toolbox/content_fingerprinter.h`](../../include/toolbox/content_fingerprinter.h)

> Exact model backend not verifiable from final document text alone.
