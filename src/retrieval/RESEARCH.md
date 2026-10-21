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

# Retrieval Research Notes

## Scope

This document records the technical assumptions and design pressure for the retrieval module as it exists in source today. The implementation currently centers on two concrete surfaces: the LoRA artifact contract in `src/retrieval/include/lora_package.h` and the retrieval validation path in `src/retrieval/dual_read_validator.cpp`.

The module is not yet a full production retrieval engine. Instead, it is a governed seam for artifact packaging, integrity verification, and regression-focused evaluation of retrieval quality.

## Source evidence

- `src/retrieval/include/lora_package.h` defines the public artifact model (`LoRAPackage`, `PortableAdapterProduct`, `LoRAManifestStore`), status states, integrity metadata, and serialization contract.
- `src/retrieval/src/lora_package.cc` implements the canonical hashing and manifest logic used to validate package integrity and lifecycle state.
- `src/retrieval/dual_read_validator.cpp` measures recall, nDCG, and MRR across candidate sets and uses ground-truth judgments to detect retrieval regressions.
- `src/retrieval/ROADMAP.md` and `src/retrieval/FUTURE_ENHANCEMENTS.md` show that production-quality retrieval routing and multi-shard coordination remain staged follow-on work.

## Research questions

### 1. What is the current retrieval runtime contract?

The present source does not define a general-purpose vector search engine API. Instead, the active runtime surfaces are narrower and more conservative:

- package lifecycle control for LoRA artifacts,
- deterministic JSON serialization and SHA-256 integrity checks,
- explicit validation of a current retrieval index versus a prior reference index.

This is aligned with the code's fail-closed posture: validation and integrity checks produce explicit failure states rather than silently accepting invalid or drifted artifacts.

### 2. Why do package lifecycle and retrieval quality share the same module?

The module sits at the boundary between model adaptation and retrieval quality evaluation. A retrieval system is only as safe as the adapters and model artifacts it is allowed to serve. The LoRA package contract therefore treats model provenance, policy, and integrity as part of the retrieval trust surface.

At the same time, an index can regress even when artifacts are valid. The dual-read validator is meant to catch that by comparing current and previous retrieval metrics under the same query judgments.

### 3. Which assumptions are explicit in the current implementation?

The code assumes the following:

- artifact metadata is authoritative only when serialized and verified deterministically,
- base-model compatibility is a policy concern, not an implicit runtime property,
- retrieval evaluation needs explicit ground truth to remain meaningful,
- index drift and metric regressions should surface as explicit evidence, not silent fallback.

## Constraints and trade-offs

### Safety over permissiveness

The `LoRAManifestStore` is designed around verification and policy gating. In `lora_package.cc`, hash verification and status flows are explicit and deterministic. This avoids the common failure mode where untrusted or inconsistent adapters are accepted under an ambiguous fallback path.

### Local validation before broad retrieval claims

The dual-read validator computes metrics only from provided query results and ground-truth sets. It does not claim to replace full online evaluation. This is a deliberate trade-off: the module prefers transparent, measurable regression checks over broad assumptions about production behavior.

### Narrow scope and staged rollout

The module documentation indicates that large retrieval surfaces, multi-shard coordination, and GPU acceleration remain planned work. The source is aligned to that: the current implementation covers the integrity and evaluation infrastructure rather than a full distributed retrieval stack.

## Evaluation criteria

This module is healthy when the following hold:

- every LoRA package can be serialized, hashed, and verified reproducibly,
- invalid or tampered package inputs are rejected explicitly,
- retrieval metrics are computed against concrete ground truth instead of implicit assumptions,
- the code remains transparent about what work is implemented and what remains future work.

## Conclusion

The retrieval module's research direction is conservative and governance-heavy: it prioritizes correctness, integrity, and explicit evaluation before broader search features are admitted. That design matches the source and the roadmap: the module is currently a reliable foundation layer, not yet a full production retrieval fabric.
