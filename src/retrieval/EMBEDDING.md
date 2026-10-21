---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Retrieval Embedding and Representation Notes

## Purpose

This document explains the representation boundaries in the current `src/retrieval` implementation. The module does not currently hold a production embedding generator or embedding index implementation. Instead, it governs the assets and evaluation surfaces that sit around retrieval quality and adapter deployment.

## What this module does not do

The active source files are not an embedding training pipeline. There is no in-module implementation that generates dense vectors or indexes them directly. The actual retrieval engine behavior is still staged in the roadmap; the current source ground truth is narrower:

- LoRA artifacts carry canonical metadata and integrity hashes,
- retrieval quality is measured through `RetrievalMetrics`,
- current-vs-previous retrieval evaluation is performed with `DualReadValidator`.

This is an important trust boundary. The retrieval module describes how retrieval artifacts and quality checks are represented and validated, not how every embedding is produced upstream.

## The representation model in practice

### Artifact representation

`LoRAPackage` and `PortableAdapterProduct` are the model's primary representation objects. They bundle the information a downstream system needs to decide whether a model adaptation is valid, compatible, and safe to deploy.

The representation includes:

- identity and versioning,
- base-model compatibility,
- target modules and rank/alpha information,
- provenance and integrity state,
- deployable resource metadata.

This is the module's operational representation layer for retrieval-related model adaptation.

### Quality representation

`RetrievalMetrics` represents the evaluation result of a retrieval run. It is intentionally small and explicit:

- recall at 10,
- nDCG at 10,
- MRR at 10,
- comparison deltas,
- total relevant count.

This gives the module an observable, objective representation for comparing retrieval behavior across index versions.

## Assumptions and boundaries

The current representation model assumes:

- the actual embedding vectors are created upstream by the model or vector pipeline,
- the retrieval module is responsible for validating quality and artifacts, not generating all ranking features in-house,
- cross-index comparisons are meaningful only when judgments are explicit and stable,
- policy and verification metadata matter as much as raw score values.

## Evaluation stance

A module is considered healthy when it can answer these questions:

- Do we know which adapter created a retrieval artifact?
- Is the artifact intact and policy-compliant?
- Do we have concrete evidence that the current retrieval index is better, worse, or equivalent to the prior index?
- Are failures explicit and actionable rather than silent?

When those questions are answered poorly, the system is drifting into a representation problem rather than a retrieval problem.

## Trade-offs

### Precision of representation vs. genericity

The code favors explicit, typed representation over broad generics. This increases clarity and reduces ambiguity for artifact verification and retrieval metric comparisons, but it limits the module's flexibility as a full embedding system.

### Evaluation evidence vs. optimism

`DualReadValidator` and the metric definitions prefer measurable signal with explicit ground truth. That is a stronger operational model than heuristic or unbounded performance claims.

## Conclusion

The current retrieval module is best understood as the representation and validation layer around retrieval quality and adapter trust, not as the full embeddings stack itself. Its value is in making artifact provenance, compatibility, and retrieval performance explicit and auditable.
