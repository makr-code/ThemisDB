---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Embedding and Vector-Path Governance Boundary

## Purpose and scope

This page is for developers integrating governance with vector search or model-training pipelines. `src/governance/` does not generate, transform, persist, or benchmark embedding vectors. Embedding models and vector-index execution belong to consuming modules. Governance contributes decisions that those callers must apply before using or exporting governed data.

## Governance decisions used by vector consumers

`PolicyDecision::ann_allowed` indicates whether approximate-nearest-neighbor use is permitted for the request classification/profile. `export_allowed` and `cache_allowed` constrain related data flows. `PolicyEngine::checkQueryPermission()` returns both the decision and the configured field-masking policy; the caller must apply the mask to the output path rather than assume that obtaining a decision transforms the result.

For model-training export, `ModelGovernancePolicy::checkExportPermission()` denies classifications `geheim` and `streng-geheim` and explicitly restricted collections. A permitted export can add a `MODEL_TRAINING` lineage event when a tracker is configured. This is an export gate and provenance record, not an embedding-generation hook.

## Integration sequence

1. Establish authenticated user, tenant, route, and classification context in the owning request layer. Do not treat caller-supplied headers as authentication.
2. Call the governance permission API before vector search, cache use, third-party export, or training-data collection.
3. Enforce `ann_allowed`, `cache_allowed`, and `export_allowed` at the operation boundary; do not infer an allow from a missing governance call.
4. Apply `FieldMaskingPolicy` to every returned JSON document path that can expose governed fields.
5. For a permitted training export, retain the decision and any resulting lineage event ID with the job’s audit evidence.
6. Test both permitted and denied classifications and verify that the vector/model consumer does not proceed after denial.

## Evaluation and limits

The governance module can test decision and export behavior but cannot measure embedding quality, vector recall, distance semantics, model fairness in production, or index isolation. Such evaluations belong to the relevant vector/model module and must use its own benchmark and dataset. Governance bias-audit reports consume supplied adapter/dataset statistics; they do not inspect embedding vectors.

An `ann_allowed` value is only an advisory contract unless the caller enforces it. A disabled or empty masking policy returns input unchanged. The built-in OPA WASM stub must not be used to authorize vector access or export; see [TRUST.md](TRUST.md) and [SECURITY.md](SECURITY.md).

## Source references

- [`policy_engine.h`](../../include/governance/policy_engine.h)
- [`model_governance.h`](../../include/governance/model_governance.h)
- [`data_masker.h`](../../include/governance/data_masker.h)
- [`AI_ML_IMPACT_ASSESSMENT.md`](AI_ML_IMPACT_ASSESSMENT.md)
- [API contract](../../include/governance/governance_api_contract.h)
