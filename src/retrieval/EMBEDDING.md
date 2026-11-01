---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "ThemisDB documentation orchestration assistant"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Retrieval Embedding and Representation Notes

## Purpose

This document explains the representation boundaries in the current `src/retrieval` implementation. The module does not currently hold a production embedding generator or embedding index implementation. Instead, it represents adapter metadata and exposes a separate, currently stubbed retrieval-metrics helper.

## What this module does not do

The active source files are not an embedding training pipeline. There is no in-module implementation that generates dense vectors or indexes them directly. The actual retrieval engine behavior is still staged in the roadmap; the current source ground truth is narrower:

- LoRA artifacts carry canonical metadata and integrity hashes,
- retrieval quality is measured through `RetrievalMetrics`,
- current-vs-previous retrieval evaluation is performed with `DualReadValidator`.

This is an important trust boundary. The retrieval module describes how retrieval artifacts and quality checks are represented and validated, not how every embedding is produced upstream.

## The representation model in practice

### Artifact representation

`LoRAPackage` and `PortableAdapterProduct` are metadata structures for adapter packages and model-bound products. They carry compatibility and integrity fields, but the module does not decide whether an adaptation is valid or safe to deploy.

The representation includes:

- identity and versioning,
- base-model compatibility,
- target modules and rank/alpha information,
- provenance and integrity state,
- deployable resource metadata.

These are metadata fields; this module does not perform model adaptation or deployment.

### Quality representation

`RetrievalMetrics` represents the evaluation result of a retrieval run. It is intentionally small and explicit:

- recall at 10,
- nDCG at 10,
- MRR at 10,
- comparison deltas,
- total relevant count.

The metric structure can hold these values, but the validator's query executor returns no results and `ValidateQuery()` does not return a comparison delta. It is not currently evidence of measured behavior across real index versions.

## Assumptions and boundaries

The current representation model assumes:

- the artifact classes do not contain or generate embedding vectors,
- `DualReadValidator` consumes results only from its current query implementation, which is a placeholder,
- ground-truth judgments are provided by callers through `SetGroundTruth()`,
- policy and verification fields are metadata; lifecycle or deployment policy is not enforced here.

## Evaluation questions (not current capabilities)

These are possible evaluation questions for a future integration; the current
module cannot answer them from live retrieval results:

- Do we know which adapter created a retrieval artifact?
- Is the artifact intact and policy-compliant?
- Do we have concrete evidence that the current retrieval index is better, worse, or equivalent to the prior index?
- Are failures explicit and actionable rather than silent?

When those questions are answered poorly, the system is drifting into a representation problem rather than a retrieval problem.

## Trade-offs

### Precision of representation vs. genericity

The code favors explicit, typed representation over broad generics. This increases clarity and reduces ambiguity for artifact verification and retrieval metric comparisons, but it limits the module's flexibility as a full embedding system.

### Evaluation evidence vs. optimism

The metric helper accepts explicit result and judgment values; because query execution is stubbed, this does not provide operational retrieval evidence.

## Conclusion

The current module contains adapter metadata/hash helpers and a separate metrics helper. It does not generate embeddings, execute retrieval, establish adapter trust, or produce auditable production performance measurements.
