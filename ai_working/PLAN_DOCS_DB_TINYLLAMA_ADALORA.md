---
Author: "ThemisDB Team"
Created: "2026-11-11"
Last Updated: "2026-11-11"
Status: "in_progress"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Docs DB to TinyLlama AdaLoRA Plan

## Scope

Provide a source-neutral native C++ AdaLoRA consumer with bounded `ITrainingSampleProvider` batches. Keep AQL/TrainingDataIterator as a generic DB provider and add docs.db only as a read-only concrete provider adapter; export the existing GGUF adapter format.

## Acceptance Checks

- The docs-db importer preserves full generated chunks under a distinct key prefix while keeping existing document records readable.
- AQL and docs.db providers emit the same bounded normalized sample records; the AdaLoRA consumer has no source-format dependency.
- The docs.db provider opens its source read-only, consumes only recognized document/chunk records, applies `max_samples`, and fails closed on malformed or empty data.
- Training reuses the existing frozen-model next-token AdaLoRA implementation; no synthetic data or stub path is added to production code.
- Focused tests cover importer schema, read-only bounded extraction, invalid/missing DB errors, and a TinyLlama GGUF end-to-end path gated by `THEMIS_TEST_MODEL_PATH`.
- Module README and roadmap describe source-data limitations, invocation contract, and remaining authorization/signing requirements.

## Known Constraints

- Current generated `documents` records store only a first-chunk preview; full chunks are present in the generated JSON artifact but are not imported into RocksDB.
- The training module's production requirements still call for caller authorization and signed adapter persistence; this delivery must not claim those gates are closed.
