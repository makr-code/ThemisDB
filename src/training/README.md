# Training Module

## Module Purpose

The Training module provides tools for building and maintaining domain-specific AI fine-tuning datasets and adapters within ThemisDB. It is designed around the legal domain (with German-language legal text as the primary target) and consists of three components: an automatic labeler that extracts structured training samples from legal documents using NLP modality detection, an incremental LoRA trainer that fine-tunes language model adapters with checkpoint/resume support, and a knowledge graph enricher that annotates training samples with graph-traversal context (related provisions, case law, and semantically similar documents).

## Relevant Interfaces

| Interface / File | Role |
|-----------------|------|
| `auto_labeler.cpp` | LegalAutoLabeler: structured training sample extraction from legal documents |
| `lora_trainer.cpp` | Incremental LoRA adapter fine-tuning with checkpoint/resume |
| `knowledge_graph_enricher.cpp` | AQL-based context enrichment via graph traversal |
| `adapter_version_manager.cpp` | Adapter version management (deploy, rollback, list versions) |

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
- AQL query executor (ThemisDB internal) — database reads and graph traversal (production integration; currently stubbed with `// TODO` comments)
- Vector index / embedding store — cosine similarity for `findSimilarDocuments` (production integration; currently stubbed)

## Usage Examples

```cpp
#include "training/auto_labeler.h"
#include "training/incremental_lora_trainer.h"
#include "training/knowledge_graph_enricher.h"

using namespace themis::training;

// --- 1. Auto-label legal documents ---
AutoLabelConfig label_config;
label_config.language_code = "de";
label_config.min_confidence = 0.6f;
label_config.flag_low_confidence = true;

LegalAutoLabeler labeler(label_config, "rocksdb://./data");
LabelingStats stats = labeler.labelAll(
    [](size_t done, size_t total, const std::string& msg) {
        // progress
    }
);

// --- 2. Enrich samples with graph context ---
EnrichmentConfig enrich_config;
enrich_config.include_provisions  = true;
enrich_config.include_case_law    = true;
enrich_config.include_similar_docs = true;
enrich_config.max_related_items   = 5;

KnowledgeGraphEnricher enricher(enrich_config, "rocksdb://./data");
enricher.enrichAll();

// Enrich a single sample
GraphContext ctx = enricher.enrichSample("sample_001");
// ctx.related_provisions, ctx.case_law, ctx.similar_documents

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

## Production Readiness

**Current Status: Alpha**

The module provides the correct structural scaffolding and integration points but contains several production stubs awaiting integration with the database query executor:
- `LegalAutoLabeler`: document fetching (`labelAll`, `labelQuery`) and DB sample writes are `// TODO` stubs; the document-labeling logic (`labelDocument`) is fully implemented
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
