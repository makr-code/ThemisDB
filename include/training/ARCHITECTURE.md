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

Exact model backend not verifiable from final document text alone.

# Training Public-Header Architecture

`include/training/` declares contracts for training data preparation, adapter
lifecycle, checkpoint and provenance handling, and adapter handoff. Runtime
behavior is implemented under [`../../src/training/`](../../src/training/);
this document describes public boundaries, not a guarantee that every
configured integration is available in every build.

## Contract groups

| Group | Public entry points | Boundary |
|---|---|---|
| Data preparation | `LegalAutoLabeler`, `DatabaseDomainAutoLabeler`, `ModalityDetector`, `KnowledgeGraphEnricher` | Convert configured source data to samples and optional graph/vector context |
| Selection and orchestration | `DataSelectionPipeline`, `TrainingPipeline` | Filter/select samples and run enabled pipeline stages |
| Training and adapters | `IncrementalLoRATrainer`, `LoRAAdapter`, `AdaLoRAAdapter`, `MultiTaskLoRATrainer` | Train or update adapters and expose their lifecycle |
| Persistence and lineage | `LoRACheckpointManager`, `ProvenanceTracker` | Persist/validate checkpoint data and record sample lineage |
| Serving handoff | `ILLMRouter`, `DeployResult` | Request adapter traffic updates and report the operation outcome |
| Shared contracts | Interfaces, errors, diagnostics, failsafe and incident-emitter headers | Define extension points and failure reporting |

The public namespace is `themis::training`. `training_interfaces.h` declares
checkpoint, provenance, enrichment, calibration, asynchronous pipeline, and
lineage interfaces; it does not declare the `ITrainer` or `IDataIterator` types
claimed by older versions of this page.

## Data and control flow

1. Labeling and modality extraction produce `TrainingSample` values.
2. Selection accepts the separate `DataSample` type and returns a selected
   subset with selection audit data. Do not treat the two sample structures as
   interchangeable without conversion.
3. Optional graph enrichment can query configured graph data and an injected
   vector index.
4. `TrainingPipeline` conditionally coordinates preparation, quality/drift
   checks, selection, and training according to `PipelineConfig`.
5. The trainer/checkpoint manager persist or resume adapter state; the
   `ILLMRouter` integration separately handles deployment and rollback.

The data-selection clustering path currently uses a character-statistic
heuristic, not a learned embedding service. The optional OCR path does not
produce recognized image text. The module roadmap also tracks real bounded
database-backed readers for the iterator, trainer, and pipeline as unfinished;
offline/test behavior must not be presented as production database evidence.

## Ownership and failure boundaries

- Injected query, storage, vector-index, and router dependencies retain their
  documented ownership/lifetime rules; consult the corresponding header
  comments before integration.
- Checkpoint validation/recovery and adapter deployment return explicit
  outcomes. Callers must inspect those outcomes and must not infer serving
  success from training success.
- Vector-index search requires an initialized index and vectors keyed by
  document ID. Missing vectors and failed searches can yield no results.
- See [`TRAINING_CONTRACT.md`](./TRAINING_CONTRACT.md) for the lifecycle/error
  summary and [`../../src/training/ARCHITECTURE.md`](../../src/training/ARCHITECTURE.md)
  for module-level implementation flow.
