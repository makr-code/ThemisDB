---
Author: "ThemisDB Team"
Created: "2026-10-18"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Training module capabilities

The training module provides C++ components for preparing training samples,
selecting data, training and managing LoRA/AdaLoRA adapters, and coordinating
checkpoint, provenance, and serving-handoff operations. This is a
source-grounded capability summary, not a release-readiness statement.

## Current capabilities

- **Sample preparation:** `LegalAutoLabeler` and
  `DatabaseDomainAutoLabeler` produce labeled samples from their configured
  inputs. `ModalityDetector` extracts text clauses, tables, and citations and
  can emit OCR-tagged samples.
- **Selection and enrichment:** `DataSelectionPipeline` filters, de-duplicates,
  clusters, scores, and records selection results. `KnowledgeGraphEnricher`
  adds graph context and can query a caller-provided vector index.
- **Adapter training and lifecycle:** `IncrementalLoRATrainer` and
  `MultiTaskLoRATrainer` expose training operations; adapter implementations
  support LoRA/AdaLoRA representation, merge, checkpoint, and resume paths.
- **Orchestration and governance:** `TrainingPipeline` coordinates enabled
  stages, while `ProvenanceTracker` records sample lineage and
  `adapter_serving.h` exposes router handoff results.

## Limits relevant to integration

- The roadmap still tracks real bounded database-backed sample readers for the
  iterator, trainer, and complete pipeline as unfinished. Offline/test behavior
  is not evidence that these paths read production collections.
- Selection clustering currently uses character statistics, not embeddings
  produced by the configured model name.
- The optional OCR path does not currently return recognized image text.
- A trained adapter is not automatically serving: deployment requires a
  configured router and callers must inspect its result.

## Source references

- [`API.md`](./API.md) — public interfaces and integration requirements.
- [`ARCHITECTURE.md`](./ARCHITECTURE.md) — data flow and component boundaries.
- [`MODELING.md`](./MODELING.md) and [`EMBEDDING.md`](./EMBEDDING.md) — method
  details and current limitations.
- [`ROADMAP.md`](./ROADMAP.md) — current implementation work and open gates.
