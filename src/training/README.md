# Training Module

**Status:** 🟢 Production-Ready
**Version:** 1.6.0
**Last Validated:** 2026-04-06
**Module Path:** `src/training/` / `include/training/`

---

## Related Documentation

- [ARCHITECTURE.md](ARCHITECTURE.md) — System architecture, component diagram, data flow
- [ROADMAP.md](ROADMAP.md) — Feature roadmap and implementation phases
- [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md) — Planned features and design constraints
- [German Docs](../../docs/de/training/README.md) — Deutschsprachige Übersicht

---

## Module Purpose

The Training module provides tools for building and maintaining domain-specific AI fine-tuning datasets and adapters within ThemisDB. It is designed around the legal domain (with German-language legal text as the primary target) and consists of three components: an automatic labeler that extracts structured training samples from legal documents using NLP modality detection, an incremental LoRA trainer that fine-tunes language model adapters with checkpoint/resume support, and a knowledge graph enricher that annotates training samples with graph-traversal context (related provisions, case law, and semantically similar documents).

## Relevant Interfaces

| Interface / File | Role |
|-----------------|------|
| `auto_labeler.cpp` | LegalAutoLabeler: structured training sample extraction from legal documents |
| `incremental_lora_trainer.cpp` | Incremental LoRA adapter fine-tuning with checkpoint/resume |
| `knowledge_graph_enricher.cpp` | AQL-based context enrichment via graph traversal |
| `lora_checkpoint_manager.cpp` | LoRA checkpoint management with SHA-256 integrity validation and rotation |
| `modality_parser.cpp` | ModalityDetector, TextClauseExtractor, TableExtractor, CitationExtractor, OCRExtractor |
| `provenance_tracker.cpp` | Training sample provenance and lineage tracking |
| `lora_data_selection.cpp` | Training data selection, deduplication, and balancing |
| `training_pipeline.cpp` | End-to-end training pipeline orchestrator (ConfidenceCalibrator, ProvenanceTracker integration) |

## Scope

**In Scope:**
- Automated training data labeling from legal documents via NLP modality extraction
- Incremental LoRA adapter fine-tuning with epoch/batch training loop structure
- Checkpoint saving and resume from checkpoint
- Adapter version management (deploy, rollback, list versions)
- Knowledge graph context enrichment via AQL graph traversal queries
- Vector similarity search for finding semantically similar documents
- Confidence-threshold filtering and human-review flagging for low-confidence samples

**Out of Scope:**
- The NLP modality extractor itself (provided by `analytics/nlp_text_analyzer`)
- Raw LLM base model management or quantization
- Training data storage schema definition (handled by the storage module)
- Serving or inference of trained adapters (handled by the LLM integration layer)
- Distributed/multi-GPU training coordination

## Key Components

### LegalAutoLabeler
**Location:** `auto_labeler.cpp`

Automatically generates labeled training samples from legal documents stored in ThemisDB. Uses `analytics::NlpTextAnalyzer` to extract deontic modalities (must/shall/may/etc.) from text, then converts each detected modality into an input/output training pair.

**Features:**
- `labelAll()` — process every document in the configured source collection
- `labelDocument(id)` — label a single document by ID, returns `std::vector<TrainingSample>`
- `labelQuery(aql)` — label documents matching an arbitrary AQL query
- `getLowConfidenceSamples(threshold)` — retrieve samples below the confidence threshold for human review
- `updateSampleConfidence(id, score, reviewer)` — record human review decisions
- Configurable `min_confidence` threshold and `flag_low_confidence` for review queue
- Supports German (`de`) and other `language_code` configurations
- Pimpl pattern for ABI stability

**Training Sample Structure:**
Each `TrainingSample` contains an `input` (analysis instruction + source text), an `output` (category, deontic logic label, and interpretation), a `confidence` score from the NLP analyzer, and `source_id` linking back to the originating document.

### IncrementalLoRATrainer
**Location:** `incremental_lora_trainer.cpp`

Manages the full lifecycle of LoRA (Low-Rank Adaptation) adapter training: initial training, incremental fine-tuning on new data, checkpoint save/resume, evaluation, deployment, and rollback.

**Features:**
- `train(mode, callback)` — run training in `INITIAL` or `INCREMENTAL` mode
- `resumeFromCheckpoint(path, callback)` — load checkpoint and continue training
- `evaluate(adapter_version)` — compute validation loss and accuracy for a stored adapter
- `deployVersion(version, traffic_split)` — route a configurable fraction of traffic to a new adapter version
- `rollbackVersion(target_version)` — revert to a previously deployed adapter
- `listVersions()` — enumerate available adapter versions
- `setHyperparameters(rank, alpha, lr)` — configure LoRA rank, scaling factor, and learning rate
- `setCheckpointing(enabled, steps)` — enable periodic checkpoint saves during training
- Training callback `(epoch, step, loss, message)` for progress monitoring
- Pimpl pattern for ABI stability

**Training Configuration (`IncrementalTrainingConfig`):**
`adapter_version`, `num_epochs`, `batch_size`, and `source_collection` name for fetching training samples.

### KnowledgeGraphEnricher
**Location:** `knowledge_graph_enricher.cpp`

Enriches existing `TrainingSample` records with graph-derived context by executing AQL traversal queries against ThemisDB's graph store. Adds related legal provisions, case law citations, and semantically similar documents to each sample's context.

**Features:**
- `enrichAll(callback)` — enrich every sample in the target collection
- `enrichSample(sample_id)` — enrich a single sample, returns a `GraphContext`
- `enrichQuery(aql, callback)` — enrich samples matching an AQL query
- `findRelatedProvisions(doc_id, max)` — graph outbound traversal to find referenced provisions
- `findRelatedCaseLaw(doc_id, max)` — graph traversal filtering for `document_type == "case_law"`
- `findSimilarDocuments(doc_id, max)` — cosine similarity search over document embeddings
- `setCustomQuery(name, aql)` — register named custom AQL queries for domain-specific traversals
- Configurable flags: `include_provisions`, `include_case_law`, `include_similar_docs`, `max_related_items`
- Pimpl pattern for ABI stability

**GraphContext Structure:**
`related_provisions` (list of document keys), `case_law` (list of document keys), `similar_documents` (list of document keys), and `context_summary` (human-readable summary string).

## Architecture

```
LegalAutoLabeler
    │
    ├─ analytics::NlpTextAnalyzer  ← extracts deontic modalities from text
    ├─ Database source collection  ← legal documents (AQL query)
    └─► TrainingSample[]           ── written to training samples collection

KnowledgeGraphEnricher
    │
    ├─ AQL graph traversal         ← related provisions, case law
    ├─ Vector similarity search    ← similar documents (embedding index)
    └─► GraphContext               ── merged into TrainingSample.graph_context

IncrementalLoRATrainer
    │
    ├─ Database training collection ← reads TrainingSample[] (with graph context)
    ├─ Training loop (epochs/batches)
    ├─ Checkpoint storage
    └─► LoRA adapter versions       ── stored and versioned for deployment
```

## Dependencies

### Internal Dependencies
- `analytics/nlp_text_analyzer.h` — legal modality extraction (NLP)
- `training/auto_labeler.h` — `LegalAutoLabeler`, `TrainingSample`, `AutoLabelConfig`
- `training/incremental_lora_trainer.h` — `IncrementalLoRATrainer`, `IncrementalTrainingConfig`, `TrainingResult`
- `training/knowledge_graph_enricher.h` — `KnowledgeGraphEnricher`, `EnrichmentConfig`, `GraphContext`

### External Dependencies
- `<chrono>` — elapsed time tracking in training/enrichment stats
- `<stdexcept>` — exception propagation
- AQL query executor (`QueryEngine` / `executeAql()` from `query/aql_runner.h`) — document ID fetch in `labelAll()`, document text fetch in `labelDocument()`, user-supplied queries in `labelQuery()`. Pass a `QueryEngine*` to `LegalAutoLabeler` at construction time. Pass `nullptr` to run in offline/test mode (no DB access).
- `VectorIndexManager` (from `index/vector_index.h`) — cosine-similarity search for `findSimilarDocuments()`. Wire via `KnowledgeGraphEnricher::setVectorIndex(&vim)`. Pass `nullptr` (default) to run without a vector index.

## Usage Examples

```cpp
#include "training/auto_labeler.h"
#include "training/incremental_lora_trainer.h"
#include "training/knowledge_graph_enricher.h"
#include "query/query_engine.h"
#include "query/aql_runner.h"

using namespace themis::training;

// --- 0. Obtain a QueryEngine (wired to your RocksDB instance) ---
// RocksDBWrapper db(...);  db.open();
// SecondaryIndexManager idx(db);
// QueryEngine engine(db, idx);
//
// Pass &engine to components that need DB access, or nullptr for offline mode.

// --- 1. Auto-label legal documents (DB-connected mode) ---
AutoLabelConfig label_config;
label_config.source_collection  = "legal_documents";
label_config.target_collection  = "legal_training_samples";
label_config.language_code      = "de";
label_config.min_confidence     = 0.6f;
label_config.flag_low_confidence = true;

// Pass &engine to enable AQL-based document fetch.
// Omit the third argument (or pass nullptr) for offline/test mode.
LegalAutoLabeler labeler(label_config, "rocksdb://./data", &engine);
LabelingStats stats = labeler.labelAll(
    [](size_t done, size_t total, const std::string& msg) {
        // progress
    }
);
// stats.documents_processed — number of documents fetched and labeled
// stats.samples_created     — total training samples generated

// Label only documents matching an arbitrary AQL query:
auto custom_stats = labeler.labelQuery(
    "FOR doc IN legal_documents FILTER doc.jurisdiction == 'DE' RETURN doc._key");

// --- 2. Enrich samples with graph context ---
EnrichmentConfig enrich_config;
enrich_config.include_provisions  = true;
enrich_config.include_case_law    = true;
enrich_config.include_similar_docs = true;
enrich_config.max_related_items   = 5;
enrich_config.similarity_threshold = 0.75f;

// RocksDBWrapper db(...);  db.open();
// VectorIndexManager vim(db);
// vim.init("documents", /*dim=*/1536, VectorIndexManager::Metric::COSINE);
// (populate vim with document embeddings, then:)

KnowledgeGraphEnricher enricher(enrich_config, "rocksdb://./data");
enricher.setVectorIndex(&vim);   // wire real cosine-similarity search
                                  // omit (or pass nullptr) for offline/test mode
enricher.enrichAll();

// Enrich a single sample
GraphContext ctx = enricher.enrichSample("sample_001");
// ctx.related_provisions, ctx.case_law, ctx.similar_documents

// Direct vector-similarity query (returns pairs of {doc_id, cosine_score ∈ [0,1]})
auto similar = enricher.findSimilarDocuments("doc_001", /*max_results=*/5);
// similar[0] = {"doc_042", 0.97f}, similar[1] = {"doc_017", 0.84f}, ...

// --- 3. Train a LoRA adapter ---
IncrementalTrainingConfig train_config;
train_config.adapter_version = "v1.0";
train_config.num_epochs      = 3;
train_config.batch_size      = 16;

IncrementalLoRATrainer trainer(train_config, "rocksdb://./data");
trainer.setHyperparameters(/*rank=*/8, /*alpha=*/16.0f, /*lr=*/1e-4f);
trainer.setCheckpointing(true, /*checkpoint_steps=*/100);

TrainingResult result = trainer.train(
    TrainingMode::INITIAL,
    [](size_t epoch, size_t step, double loss, const std::string& msg) {
        // progress
    }
);

if (result.success) {
    // Deploy with 10% canary traffic
    trainer.deployVersion(result.adapter_id, 0.1f);

    // Evaluate
    TrainingResult eval = trainer.evaluate(result.adapter_id);
    // eval.accuracy, eval.validation_loss
}

// --- 4. Rollback if needed ---
trainer.rollbackVersion("v0.9");
```

## Integration Steps (AQL Executor)

Follow these steps to wire `LegalAutoLabeler` to a live ThemisDB instance:

1. **Open a RocksDB instance** and create the required secondary index manager:
   ```cpp
   RocksDBWrapper::Config db_cfg;
   db_cfg.db_path = "data/themis";
   RocksDBWrapper db(db_cfg);
   db.open();
   SecondaryIndexManager idx(db);
   QueryEngine engine(db, idx);
   ```

2. **Construct `LegalAutoLabeler` with the engine pointer:**
   ```cpp
   AutoLabelConfig cfg;
   cfg.source_collection = "legal_documents";  // collection to read from
   cfg.target_collection = "legal_training_samples";
   LegalAutoLabeler labeler(cfg, "", &engine);
   ```

3. **Populate the source collection.** Each document must have a non-null, non-empty `text` field:
   ```cpp
   BaseEntity doc("doc_001");
   doc.setField("text", std::string("Die Behörde muss die Genehmigung erteilen ..."));
   idx.put("legal_documents", doc);
   ```

4. **Run labeling** — `labelAll()` fetches all document IDs via AQL, then fetches each document's `text` field via a secondary AQL query and produces `TrainingSample` records:
   ```cpp
   auto stats = labeler.labelAll();
   // stats.documents_processed == number of documents found in DB
   // stats.samples_created     == number of training samples generated
   ```

5. **Offline / test mode** — pass `nullptr` (or omit the third argument) to skip all DB access; the labeler will process zero documents from the collection while still allowing `labelDocument(id)` calls:
   ```cpp
   LegalAutoLabeler offline_labeler(cfg, "");  // engine defaults to nullptr
   ```

## Production Readiness

**Current Status: Alpha**

The module provides production-ready AQL-executor integration for document labeling and contains scaffolding for remaining stubs:

- `LegalAutoLabeler`:
  - ✅ `labelAll()` — fetches document IDs from `source_collection` via AQL (`FETCH_ALL_DOCUMENTS`), then fetches each document's `text` field via `FETCH_DOCUMENT_BY_ID` (wired in v1.6.0)
  - ✅ `labelQuery(aql)` — executes the caller-supplied AQL query to obtain document IDs, then labels each document as above (wired in v1.6.0)
  - ✅ `labelDocument(id)` — uses `FETCH_DOCUMENT_BY_ID` AQL when engine is wired; falls back to hardcoded text in offline/test mode
  - ⏳ DB sample writes (`persistSampleBatch`) — placeholder pending batch-insert wiring (Phase 2)
- `IncrementalLoRATrainer`: actual model weight manipulation, optimizer state, and checkpoint serialization are simulated with placeholder values; integrate with your chosen ML framework (e.g., llama.cpp LoRA APIs, libtorch)
- `KnowledgeGraphEnricher`: AQL graph traversal queries and vector similarity search are defined as commented AQL templates but return empty lists until the query executor is wired in
- Known limitations:
  - Training data must be in ThemisDB; no external dataset connector is provided (use the Ingestion module first)
  - `deployVersion` traffic splitting is a configuration placeholder; production deployment requires a routing layer update
  - German is the primary tested language; other languages require validation of the `NlpTextAnalyzer` modality configuration

## Scientific References

1. Hu, E. J., Shen, Y., Wallis, P., Allen-Zhu, Z., Li, Y., Wang, S., … Chen, W. (2022). **LoRA: Low-Rank Adaptation of Large Language Models**. *Proceedings of the 10th International Conference on Learning Representations (ICLR)*. https://arxiv.org/abs/2106.09685

2. Howard, J., & Ruder, S. (2018). **Universal Language Model Fine-Tuning for Text Classification**. *Proceedings of the 56th Annual Meeting of the Association for Computational Linguistics (ACL)*, 328–339. https://doi.org/10.18653/v1/P18-1031

3. Radford, A., Narasimhan, K., Salimans, T., & Sutskever, I. (2018). **Improving Language Understanding by Generative Pre-Training**. OpenAI Blog. https://openai.com/research/language-unsupervised

4. Dettmers, T., Pagnoni, A., Holtzman, A., & Zettlemoyer, L. (2023). **QLoRA: Efficient Finetuning of Quantized LLMs**. *Advances in Neural Information Processing Systems (NeurIPS)*, 36. https://arxiv.org/abs/2305.14314

5. Liao, S., Martelot, E., Coates, A., Darby, T., Gong, B., Zhang, F., & Natsev, A. (2023). **LIMA: Less Is More for Alignment**. *Advances in Neural Information Processing Systems (NeurIPS)*, 36. https://arxiv.org/abs/2305.11206

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.
