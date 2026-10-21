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

# Retrieval Implementation Notes

## Purpose

This document describes the implementation behavior that is actually present in `src/retrieval` today. It is intentionally grounded in the source and avoids claiming that the module is a complete retrieval platform.

## Implemented surfaces

### LoRA artifact lifecycle

`src/retrieval/include/lora_package.h` defines the public artifact model for the module. It includes:

- `LoRAPackageStatus` with `DRAFT`, `VALIDATED`, `DEPRECATED`, and `REVOKED`
- `AdapterProductStatus` with `BUILDING`, `READY`, `DEPLOYED`, `RETIRED`, and `FAILED`
- `AdapterUsagePolicy`, `LoRAPackageProvenance`, and `ArtifactIntegrity`
- `LoRAManifestStore` CRUD and integrity verification APIs

`src/retrieval/src/lora_package.cc` implements the actual serialization and integrity behavior. The critical implementation details are:

- canonical JSON serialization using `nlohmann::json`
- SHA-256 hashing via an internal portable implementation
- deterministic field encoding and manifest hashing
- `from_json` validation for required fields and policy data
- verification helpers that compare computed hashes against expected values

### Dual-read regression validation

`src/retrieval/dual_read_validator.cpp` implements the operational regression check used to compare current and previous retrieval runs.

The validator computes:

- `ComputeRecallAtK`
- `ComputeNdcgAtK`
- `ComputeMrrAtK`
- aggregate retrieval metrics and delta checks

This is a concrete source-grounded implementation path for evaluating whether a new retrieval index has regressed relative to a previous baseline.

## Key invariants

The current implementation strongly implies the following correctness invariants:

1. Manifest hashing must be deterministic across equivalent JSON payloads.
2. Package validation must fail closed if data is missing, malformed, or inconsistent.
3. Retrieval comparisons need explicit ground truth; otherwise the validation path is not meaningful.
4. Lifecycle status changes are not optional metadata: they are part of the governance contract.

## Error-handling model

### Artifact handling

The code is explicit about malformed packages. Missing required fields, invalid hash values, and missing policy metadata are treated as operational failures, not silent defaults.

### Retrieval comparison

The validator returns zero values for empty ground truth, but it still maintains direct and auditable metric calculations. This is conservative and transparent: the system does not claim a positive result when the evidence is unavailable.

## Source-level constraints

The actual code indicates that the module is intentionally conservative:

- `DualReadValidator::ExecuteQuery()` is a placeholder and returns empty result sets until a concrete index path is implemented.
- `GetGroundTruth()` warns and returns empty data when no judgment set is cached.
- `ROADMAP.md` explicitly marks the broader retrieval rollout as staged and not yet feature-complete.

This matters because the module's documented behavior is currently a measured, staged maturity level rather than a fully launched retrieval platform.

## Implementation recommendations

When touching this module, keep the following priorities:

- preserve integrity and serialization determinism,
- preserve explicit error handling,
- keep policy and verification state auditable,
- avoid claiming end-to-end retrieval readiness beyond the source-backed evidence,
- document any new “production” path with matching roadmap and runbook updates.

## Conclusion

The implemented retrieval module is a disciplined foundation layer: it enforces adapter integrity, keeps retrieval quality measurable, and does so without overstating the production maturity of the broader retrieval stack.
