> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Training Module — Architecture Guide

**Version:** 1.0
**Last Updated:** 2026-04-06
**Module Path:** `src/training/`

---

## 1. Overview

The Training module provides tools for building and maintaining domain-specific LLM
fine-tuning datasets and LoRA adapters from data stored in ThemisDB. It automates training
sample extraction from domain documents (legal text, technical documentation), manages
the incremental LoRA training lifecycle (train, checkpoint, evaluate, deploy, rollback),
and enriches training samples with knowledge graph context.

---

## 2. Design Principles

- **Data-Driven Labeling** – `auto_labeler.cpp` uses ThemisDB's own NLP analytics to
  extract structured training samples from stored documents without manual annotation.
- **Incremental Training** – `incremental_lora_trainer.cpp` supports initial training
  and incremental fine-tuning on new data, with checkpoint/resume.
- **Graph-Enriched Context** – `knowledge_graph_enricher.cpp` adds related entities
  (provisions, citations, similar documents) from the knowledge graph to training samples.
- **Confidence Gating** – low-confidence samples are flagged for human review before
  being included in training data.
- **Adapter Versioning** – `incremental_lora_trainer.cpp` manages adapter versions with
  deploy/rollback and configurable traffic splitting.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `auto_labeler.cpp` | LegalAutoLabeler: extract structured training samples from documents |
| `incremental_lora_trainer.cpp` | LoRA adapter training with checkpoint/resume |
| `knowledge_graph_enricher.cpp` | AQL-based context enrichment via graph traversal |
| `lora_checkpoint_manager.cpp` | Checkpoint management with SHA-256 integrity validation and rotation |
| `lora_data_selection.cpp` | Training data selection and deduplication |
| `modality_parser.cpp` | Multi-modality content extraction (text, tables, citations, OCR) |
| `provenance_tracker.cpp` | Training sample provenance and lineage tracking |
| `training_pipeline.cpp` | End-to-end training pipeline orchestrator |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│              ThemisDB Document Store                             │
│   legal documents, technical docs, ...                          │
└──────────────────────────┬──────────────────────────────────────┘
                           │ documents
┌──────────────────────────▼──────────────────────────────────────┐
│                     AutoLabeler                                  │
│  NlpTextAnalyzer: extract deontic modalities                    │
│  → TrainingSample {input, output, confidence, source_id}        │
│  low confidence? → flag for human review                        │
└──────────────────────────┬──────────────────────────────────────┘
                           │ training samples
┌──────────────────────────▼──────────────────────────────────────┐
│                 KnowledgeGraphEnricher                           │
│  AQL traversal: find related provisions, case law, similar docs │
│  → enrich sample.context with graph-derived neighbors           │
└──────────────────────────┬──────────────────────────────────────┘
                           │ enriched samples
┌──────────────────────────▼──────────────────────────────────────┐
│                 LoraDataSelection                                │
│  dedup, balance, stratify                                       │
└──────────────────────────┬──────────────────────────────────────┘
                           │ curated dataset
┌──────────────────────────▼──────────────────────────────────────┐
│              IncrementalLoRATrainer                              │
│  train / resume / evaluate / deploy / rollback                  │
│  → LoRA adapter weights (INITIAL or INCREMENTAL mode)           │
└──────────────────────────┬──────────────────────────────────────┘
                           │ adapter
┌──────────────────────────▼──────────────────────────────────────┐
│                  LLM Module (src/llm/)                          │
│   multi_lora_manager: load new adapter for inference            │
└──────────────────────────────────────────────────────────────────┘
```

---

## 4. Data Flow

### 4.1 Automated Training Data Creation

```
training_pipeline.run("legal_docs", mode=INCREMENTAL)
    │
    ├─ auto_labeler.labelQuery("FOR doc IN legal_docs RETURN doc")
    │       → [TrainingSample{input, output, confidence}]
    │       → low_confidence → flag for review
    │
    ├─ knowledge_graph_enricher.enrichAll(samples)
    │       → AQL: traverse RELATED_TO edges from doc → related_provisions
    │       → vector similarity search → top-5 similar docs
    │       → annotate samples with graph context
    │
    ├─ lora_data_selection: dedup → balance → stratify
    │
    └─ incremental_lora_trainer.train(mode=INCREMENTAL, callback=...)
               → save checkpoint every 100 steps
               → final: evaluate validation loss → deploy if better
```

### 4.2 Adapter Deployment

```
incremental_lora_trainer.deployVersion("v1.3", traffic_split=0.1)
    │
    ├─ lora module: load adapter_v1.3 alongside current adapter
    ├─ route 10% of LLM requests to v1.3
    │
    ├─ monitor quality metrics (RAG judge scores)
    │       → better than baseline → increase split → full rollout
    │       → worse → rollback: deployVersion("v1.2", split=1.0)
    │
    └─ incremental_lora_trainer.cpp: record version history
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Uses** | `src/analytics/` | NLP modality extraction |
| **Uses** | `src/storage/` | Document access |
| **Uses** | `src/query/` | AQL for document and graph queries |
| **Uses** | `src/llm/` | LoRA adapter loading and inference |
| **Produces to** | `src/exporters/` | JSONL training data export |
| **Consumes from** | `src/rag/` | Quality metrics for deployment decisions |

---

## 6. Threading & Concurrency Model

- Training loop runs on a dedicated background thread; UI/API calls are non-blocking.
- `auto_labeler` is stateless per document; safe for parallel invocation.
- `knowledge_graph_enricher` uses async AQL queries; parallelism configurable.
- Checkpoint saves are atomic (write to temp file, then rename).

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| Incremental training | Only new documents processed; no full re-training |
| Graph-batch enrichment | AQL queries batch multiple samples per round-trip |
| LoRA efficiency | Full fine-tuning of 7B model → LoRA: 100× less compute |
| Checkpoint resume | Training survives server restart |

---

## 8. Security Considerations

- Training data access is scoped to the authenticated tenant.
- Adapter weights are versioned and signed before deployment.
- Human review queue is access-controlled (reviewer role required).
- Low-confidence samples are never deployed without human sign-off.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `training.min_confidence` | 0.8 | Min auto-labeler confidence |
| `training.lora.rank` | 16 | LoRA rank |
| `training.lora.alpha` | 32 | LoRA scaling factor |
| `training.lora.learning_rate` | 1e-4 | Learning rate |
| `training.checkpoint.enabled` | true | Enable checkpoint saves |
| `training.checkpoint.steps` | 100 | Steps between checkpoints |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| NLP labeling failure | Skip document; log warning; continue |
| Graph query failure | Skip enrichment for affected samples; log |
| Training divergence | Stop training; restore last checkpoint |
| Deployment quality regression | Auto-rollback to previous version |

---

## 11. Known Limitations & Future Work

- Multi-GPU and distributed training coordination is out of scope (GPU module handles
  single-node multi-GPU).
- Domain specialization is currently legal text (German); other domains require
  custom `auto_labeler` configurations.
- Training pipeline UI for human review is planned.

---

## 12. References

- `src/training/README.md` — module overview
- `src/training/ROADMAP.md` — feature roadmap
- `src/training/FUTURE_ENHANCEMENTS.md` — planned enhancements
- `docs/de/training/README.md` — Deutschsprachige Übersicht
- `src/llm/lora_framework/` — LoRA implementation
- `ARCHITECTURE.md` (root) — full system architecture
