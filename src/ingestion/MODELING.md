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

# Ingestion Data and Quality Model

## Purpose and scope

This document explains the data representations and quality model used by the
ingestion pipeline. It is intended for developers changing extraction, schema
validation, entity assembly, or quality evaluation. The public structures in
[`base_entity.h`](../../include/ingestion/base_entity.h),
[`extraction_context.h`](../../include/ingestion/extraction_context.h), and
[`ingestion_quality_judge.h`](../../include/ingestion/ingestion_quality_judge.h)
are authoritative; this page describes their composition, not a new storage
schema.

## Data progression

An ingestion run carries source identity in `FileManifest`, then progressively
adds extraction outputs to `ExtractionContext`:

1. A connector supplies source content and source-specific metadata.
2. A workflow selects a profile using the detected MIME type and filename.
3. Ordered steps add parsed text, chunks, entities, embeddings, or tensor records
   to the context; the manifest is retained for provenance.
4. Entity assembly derives `BaseEntity` and `EntityRelation` records.
5. Sink interfaces accept graph nodes/edges, vector records, or document data
   where a connector or caller wires them into the execution path.

The workflow engine itself does not persist its assembled output. In particular,
the filesystem workflow branch in `IngestionManager` currently increments
processed counters after successful execution without passing that result to a
sink; see [`IMPLEMENTATION.md`](./IMPLEMENTATION.md).

The implementation must preserve stable source/chunk linkage. For example,
`chunk_embed` forms a chunk ID from `manifest.file_id` and the chunk sequence;
entity assembly also uses the source file ID when deriving canonical IDs. A step
that drops or rewrites those identifiers can break lineage and sink upserts.

## Validation and quality model

`SchemaConfig` combines document-level byte/text constraints with optional JSON
field rules. Field rules are applied only when the content can be parsed as
JSON; non-JSON input skips field-level validation. When `reject_invalid` is
enabled (the default), a violation is failed and may enter quarantine; setting it
false records a warning rather than rejecting the document.

The optional `IngestionQualityJudge` evaluates four enabled dimensions:

| Dimension | Default threshold | Default weight |
|---|---:|---:|
| Completeness | 0.75 | 0.30 |
| Groundedness | 0.80 | 0.35 |
| Entity coverage | 0.70 | 0.20 |
| Relation coherence | 0.70 | 0.15 |

The overall threshold defaults to 0.75. A report passes only when the overall
score and each enabled dimension satisfy their configured thresholds. These are
defaults in `IngestionJudgeConfig`; deployments should not treat them as
universally calibrated quality guarantees.

The judge uses source text as its reference for completeness and groundedness.
If the source context is sparse or the inference backend is unavailable, the
implementation returns `passed = true` with scores left at `-1.0` (not
evaluated). Consumers that require model evaluation must inspect the scores and
backend availability, not only `passed`.

## Assumptions and trade-offs

- The extraction context is mutable during workflow execution; the manifest is
  the source/provenance anchor shared across steps.
- JSON schema rules do not validate non-JSON field structure. Use content
  constraints or a format-specific parser when that distinction matters.
- LLM-based scoring adds backend latency and can depend on response parsing.
  Null/unavailable backends keep the pipeline operable but do not produce
  evaluated scores.
- Re-ingestion is bounded by the configured maximum attempts; more passes cost
  additional source processing and must not be used as an unbounded retry loop.
- In-memory writers and lineage stores are useful for tests and dry runs but do
  not provide persistence or production durability.

## Validation path

- Schema behavior: [`test_ingestion_schema_validation.cpp`](../../tests/ingestion/test_ingestion_schema_validation.cpp).
- Quality evaluation and backend injection: [`test_ingestion_quality_judge.cpp`](../../tests/ingestion/test_ingestion_quality_judge.cpp) and [`test_ingestion_inference_backend.cpp`](../../tests/ingestion/test_ingestion_inference_backend.cpp).
- Entity and relation construction: [`test_ingestion_base_entity.cpp`](../../tests/ingestion/test_ingestion_base_entity.cpp) and [`test_ingestion_lineage.cpp`](../../tests/ingestion/test_ingestion_lineage.cpp).
- Performance paths: [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md).

> Exact model backend not verifiable from final document text alone.
