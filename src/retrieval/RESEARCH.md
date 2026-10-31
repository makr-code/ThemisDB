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

# Retrieval Research Notes

## Scope

This document records the technical assumptions and design pressure for the retrieval module as it exists in source today. The implementation currently centers on two concrete surfaces: the LoRA artifact contract in `src/retrieval/include/lora_package.h` and the retrieval validation path in `src/retrieval/dual_read_validator.cpp`.

The module is not a production retrieval engine. It provides artifact data/hash helpers and a separate metrics helper; it does not enforce artifact governance or perform live retrieval evaluation.

## Source evidence

- `src/retrieval/include/lora_package.h` defines the public artifact model (`LoRAPackage`, `PortableAdapterProduct`, `LoRAManifestStore`), status states, integrity metadata, and serialization contract.
- `src/retrieval/src/lora_package.cc` implements the canonical hashing and manifest logic used to validate package integrity and lifecycle state.
- `src/retrieval/dual_read_validator.cpp` contains metric helpers and ground-truth lookup, but `ExecuteQuery()` returns an empty result vector and `ValidateQuery()` does not return a comparison delta.
- `src/retrieval/ROADMAP.md` contains historical rollout material; its old phase claims are not verified current implementation status.

## Research questions

### 1. What is the current retrieval runtime contract?

The present source does not define a general-purpose vector search engine API. Instead, the active runtime surfaces are narrower and more conservative:

- package lifecycle control for LoRA artifacts,
- deterministic JSON serialization and SHA-256 integrity checks,
- metric calculation over caller-supplied result and ground-truth values. The current validator does not query index backends.

The explicit manifest verifier returns failure for absent records, empty hashes, or hash mismatches. However, CRUD does not require verification, and a missing signature callback or signature does not fail a matching manifest hash. No general fail-closed or deployment policy is implemented.

### 2. Why do package lifecycle and retrieval quality share the same module?

The same module contains adapter metadata and metric helper types. Their coexistence does not imply an implemented runtime integration between model adaptation and retrieval.

Although `DualReadValidator` has current/previous path fields, its query executor returns no results; the current implementation does not detect index regressions.

### 3. Which assumptions are explicit in the current implementation?

The code currently implements these facts:

- artifact metadata can be serialized and manifest hashes can be explicitly checked,
- compatibility and lifecycle fields exist but do not enforce deployment policy,
- ground truth is supplied by the caller and missing entries return empty data,
- query execution is a placeholder rather than an index-drift detection path.

## Constraints and trade-offs

### Safety over permissiveness

`LoRAManifestStore` provides explicit manifest-hash verification, but no policy gate. If a callback and signature are both absent, a matching manifest hash still passes verification.

### Local validation before broad retrieval claims

Metric methods can calculate metrics from provided result and ground-truth values. `ValidateQuery()` cannot currently do this with live results because its executor is a placeholder.

### Narrow scope and staged rollout

The source in this scope does not implement large retrieval surfaces, multi-shard coordination, or GPU acceleration. Historical planning claims do not change that implementation boundary.

## Evaluation criteria

This module is healthy when the following hold:

- package hash/serialization behavior is tested by the actual relevant test sources,
- manifest hash mismatches can be detected by explicit verification calls,
- metric helpers accept concrete ground-truth values, while real retrieval result acquisition is absent,
- the code remains transparent about what work is implemented and what remains future work.

## Conclusion

The current source establishes a limited artifact-hash implementation and metric helper, not a production retrieval or governance system. Research and delivery priorities beyond this boundary need an identified owner and canonical source; none are inferred here.
