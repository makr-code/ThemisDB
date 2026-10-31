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

# LLM Wiki Research Notes

## Purpose and scope

This page records the evidence and evaluation questions for semantic wiki
retrieval in `llm_wiki`. It is for maintainers deciding whether retrieval,
embedding, and provenance changes are supported by measurements. It is not a
claim that the module has completed a research benchmark.

The source-of-truth implementation is under `src/llm_wiki/`, the public plugin
contract is in [`../../include/llm_wiki/llm_wiki_plugin_interface.h`](../../include/llm_wiki/llm_wiki_plugin_interface.h),
and current performance requirements are summarized in
[`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md). Where this page
disagrees with source, source takes precedence.

## Problem and assumptions

The plugin contract accepts documents for ingestion and returns ranked wiki
chunks for a natural-language query. Retrieval quality therefore depends on
the corpus, chunk boundaries, embedding provider, and ranking implementation;
one result on one corpus is not evidence of general semantic quality.

The current implementation includes a deterministic token-hash embedding
helper in `wikipedia/llm_wiki_plugin_impl.cpp`: tokens matching
`[A-Za-z0-9_-]+` are lower-cased, hashed with FNV-1a into a fixed-dimensional
vector, accumulated, and L2-normalized. This is a lightweight baseline, not a
contextual language model. Token collisions, vocabulary mismatch, and loss of
word order are expected limitations. The plugin interface also names other
provider options, but their availability and quality must be verified in the
specific build rather than inferred from the option names.

The entity schema represents pages, chunks, claims, edges, and evidence
packages. It requires a schema version, entity type, provenance, confidence,
and timestamps; confidence is constrained to `[0, 1]`. These fields support
traceability, but do not by themselves establish that retrieved evidence is
correct.

## Evaluation design

Use a versioned, fixed corpus and a held-out query set. Record for every run:

1. Corpus revision, document count, and chunking configuration.
2. Provider and model identifier, embedding dimension, and index configuration.
3. Query set revision and relevance judgments, including assessor guidance.
4. `Recall@1`, `Recall@3`, `Recall@5`, `Recall@10`, and MRR where judgments are
   available; record query latency separately from quality.
5. Failed/empty queries, unsafe-query flags, and unsafe chunks excluded.

`WikiWorkspaceStats::EvaluationStats` declares these retrieval metrics, and
the test tree includes focused core, edge-case, integration, and soak coverage.
These are measurement surfaces, not proof of a representative corpus baseline.
No benchmark result is claimed here; use the benchmark and release evidence
for the exact hardware and revision being assessed.

## Comparisons and trade-offs

- Compare the hash baseline against any configured model-backed provider on
  the same corpus and judgments. Do not compare scores across different
  query sets or relevance-labeling rules.
- Separate retrieval quality from ingestion throughput and latency. A quality
  gain that violates the p95 target or increases operational dependencies is
  not automatically an improvement.
- Measure with and without guardrail filtering so unsafe-content rejection
  is not mistaken for retrieval recall loss.
- Keep provenance and policy gates fixed while comparing ranking methods.
  Changing confidence thresholds at the same time confounds the result.

## Reproducibility and current limits

Start with the focused tests under [`../../tests/llm_wiki/`](../../tests/llm_wiki/)
and [`../../tests/llm/`](../../tests/llm/), then run the corresponding
benchmark target only after confirming its current CMake registration and
hardware prerequisites. Save the commit, corpus/query-set revisions, build
configuration, and raw measurements with the report.

The module roadmap and performance documents contain targets and planned
workstreams, but they are not themselves benchmark evidence. In particular,
do not report the declared `EvaluationStats` fields as measured values unless
a run has populated them and the run artifacts are available for review.

## Related documents

- [`MODELING.md`](MODELING.md) — entity and provenance representation
- [`EMBEDDING.md`](EMBEDDING.md) — vector construction and provider caveats
- [`IMPLEMENTATION.md`](IMPLEMENTATION.md) — current code paths and known gaps
- [`ROADMAP.md`](ROADMAP.md) — milestone and acceptance status

> Exact model backend not verifiable from final document text alone.
