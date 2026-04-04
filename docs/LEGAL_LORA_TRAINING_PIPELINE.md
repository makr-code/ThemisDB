# Legal LoRA Training Pipeline - Architecture & Usage

## Overview

The **Legal LoRA Training Pipeline** is a comprehensive system for training domain-specific AI models for German administrative authorities. It addresses the gap where generic Legal-BERT models miss organization-specific nuances like internal Verwaltungsvorschriften (administrative regulations), historical precedents, and jurisdiction-specific interpretations of modal verbs.

## Architecture

### High-Level Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│                    Legal LoRA Training Pipeline                      │
└─────────────────────────────────────────────────────────────────────┘
           │
           ├─── Phase 1: Multi-Source Ingestion
           │    ├─ HuggingFace Connector (lexlms/ger_legal_data)
           │    ├─ FileSystem Ingester (PDFs with OCR)
           │    ├─ API Connectors (future)
           │    └─ Database Connectors (future)
           │
           ├─── Phase 2: ThemisDB Storage
           │    ├─ Graph Layer (Document → Paragraph → Sentence → Modality)
           │    ├─ Relational Layer (legal_training_samples)
           │    └─ Vector Layer (Embeddings for semantic search)
           │
           ├─── Phase 3: Auto-Labeling
           │    ├─ Legal Modality Analyzer (PR #1 integration)
           │    └─ Knowledge Graph Enricher (context from graph)
           │
           └─── Phase 4: LoRA Training
                ├─ Initial Training (base model → legal_v1)
                ├─ Incremental Updates (legal_v1 → legal_v1.1)
                ├─ Version Management (A/B testing, rollback)
                └─ Production Deployment
```

## Key Components

### 1. Ingestion Framework

Located in `include/ingestion/` and `src/ingestion/`:

#### IngestionManager
- **Purpose**: Unified interface for multi-source data ingestion
- **Features**:
  - Source registration and priority management
  - Parallel processing support
  - Progress reporting
  - Error handling and retry logic

#### HuggingFaceConnector
- **Purpose**: Download datasets from HuggingFace Hub
- **Features**:
  - Streaming mode for large datasets (12+ GB)
  - Authentication with API tokens
  - Batch processing
  - REST API integration

#### FileSystemIngester
- **Purpose**: Read documents from local filesystem
- **Features**:
  - OCR support for scanned PDFs (Tesseract)
  - Multiple format support (PDF, DOCX, TXT, HTML, XML, JSON)
  - Metadata extraction
  - File filtering and pattern matching

### 2. Training Framework

Located in `include/training/` and `src/training/`:

#### LegalAutoLabeler
- **Purpose**: Automatically label legal documents using PR #1's Legal Modality Analyzer
- **Integration**: Uses `NlpTextAnalyzer::extractLegalModalities()`
- **Features**:
  - Detects modal verbs: "muss" (obligation), "soll" (default), "kann" (permission)
  - Generates deontic logic annotations
  - Confidence scoring
  - Low-confidence flagging for human review

#### KnowledgeGraphEnricher
- **Purpose**: Enrich training samples with graph context
- **Features**:
  - Graph traversal (2-level depth)
  - Related provisions lookup
  - Case law references
  - Internal guidance documents
  - Semantic similarity search

#### IncrementalLoRATrainer
- **Purpose**: Train and update LoRA adapters
- **Features**:
  - Initial training from scratch
  - Incremental updates without full retraining
  - Version management (legal_v1 → legal_v1.1)
  - A/B testing support
  - Auto-rollback on quality degradation

### 3. Database Schema

Located in `config/schemas/legal_training_schema.sql`:

#### Graph Layer
```sql
legal_documents → paragraphs → sentences → modal_verbs
                ↓
         legal_provisions
                ↓
         case_law, guidance
```

#### Collections
- **legal_documents**: Source documents with embeddings
- **paragraphs**: Document chunks
- **sentences**: Sentence-level analysis
- **modal_verbs**: Detected legal modalities
- **legal_provisions**: Legal provision references
- **legal_training_samples**: Training data with labels

#### Indexes
- Vector indexes for semantic search (HNSW, cosine similarity)
- B-tree indexes for filtering (confidence, category, source)

## Usage

### Quick Start

1. **Configure Data Sources**

Edit `config/ingestion/sources.yaml`:

```yaml
huggingface_sources:
  - source_id: "huggingface_legal"
    enabled: true
    location: "lexlms/ger_legal_data"
    priority: 5

filesystem_sources:
  - source_id: "custom_docs"
    enabled: true
    location: "/mnt/verwaltung/vorschriften"
    priority: 10  # Higher priority
    options:
      ocr_enabled: true
      ocr_language: "deu"
```

2. **Run Training Pipeline**

```bash
./bin/train_legal_lora --config config/lora/legal_german_training.yaml
```

Or use the C++ API:

```cpp
#include "ingestion/ingestion_manager.h"
#include "training/auto_labeler.h"
#include "training/knowledge_graph_enricher.h"
#include "training/incremental_lora_trainer.h"

// 1. Ingest data
ingestion::IngestionManager mgr(db);
mgr.registerSource({...});
auto report = mgr.ingestAll();

// 2. Auto-label
training::LegalAutoLabeler labeler(config, db);
auto stats = labeler.labelAll();

// 3. Enrich with graph
training::KnowledgeGraphEnricher enricher(config, db);
enricher.enrichAll();

// 4. Train LoRA
training::IncrementalLoRATrainer trainer(config, db);
auto result = trainer.train(training::TrainingMode::INITIAL);
```

### Configuration

#### Training Configuration

Edit `config/lora/legal_german_training.yaml`:

```yaml
lora:
  rank: 16                    # LoRA rank (capacity)
  alpha: 32.0                 # Scaling factor
  dropout: 0.1                # Regularization
  
training:
  hyperparameters:
    learning_rate: 0.0003
    batch_size: 4
    gradient_accumulation_steps: 4
    num_epochs: 3
    
  device: "cuda"              # cuda, cpu, mps
```

#### Ingestion Configuration

Control which documents to ingest:

```yaml
file_filter:
  extensions:
    - ".pdf"
    - ".docx"
  min_size_bytes: 100
  max_size_bytes: 104857600  # 100 MB
  exclude_patterns:
    - "**/backup/**"
    - "**/archive/**"
```

### Incremental Updates

As new regulations are published:

```cpp
// Add new documents to filesystem
// Then update the adapter

training::IncrementalTrainingConfig config;
config.adapter_version = "legal_v1.0";  // Start from existing
config.use_existing_adapter = true;
config.incremental_steps = 1000;

training::IncrementalLoRATrainer trainer(config, db);
auto result = trainer.train(training::TrainingMode::INCREMENTAL);
// Result: legal_v1.1 with improved accuracy
```

### A/B Testing

Deploy new versions gradually:

```cpp
// Deploy with 10% traffic split
trainer.deployVersion("legal_v1.1", 0.1f);

// Monitor metrics for 24 hours
// If successful, increase traffic
trainer.deployVersion("legal_v1.1", 0.5f);

// If issues detected, rollback
trainer.rollbackVersion("legal_v1.0");
```

## Integration with PR #1

The pipeline deeply integrates with PR #1's **Legal Modality Analyzer**:

### NlpTextAnalyzer::extractLegalModalities()

```cpp
analytics::NlpTextAnalyzer analyzer;
auto modalities = analyzer.extractLegalModalities(
    legal_text,
    "de",  // German
    "config/nlp/german_modal_verbs.yaml"
);

for (const auto& m : modalities) {
    std::cout << "Verb: " << m.verb << "\n";
    std::cout << "Category: " << m.category << "\n";
    std::cout << "Deontic Logic: " << m.deontic_logic << "\n";
    std::cout << "Strength: " << m.strength << "\n";
}
```

### Example Output

Input text:
```
"Die Behörde muss die Genehmigung erteilen, wenn alle Voraussetzungen erfüllt sind."
```

Detected modalities:
```
Verb: muss
Category: obligation
Deontic Logic: O(φ)
Strength: 0.95
Interpretation: Binding legal obligation
Context: Check all prerequisites before granting
```

This generates training samples like:
```json
{
  "input": "Analyze: Die Behörde muss die Genehmigung erteilen...",
  "output": "Obligation (O(φ)): Binding requirement to grant permission",
  "category": "obligation",
  "confidence": 0.95
}
```

## Performance Targets

### Ingestion
- **HuggingFace**: >1000 docs/sec (streaming mode)
- **Filesystem**: >100 docs/sec (with OCR)
- **Memory**: <4 GB for 10k documents

### Auto-Labeling
- **Throughput**: >100 docs/sec
- **Accuracy**: >90% confidence for high-confidence samples

### Training
- **Time**: <2 hours for 50k samples (single GPU)
- **Memory**: <16 GB VRAM (with gradient accumulation)
- **Quality**: 94% → 96% accuracy with custom data

## Monitoring & Quality Assurance

### Metrics

The pipeline exports Prometheus metrics:

```
# Ingestion
legal_ingestion_documents_total
legal_ingestion_errors_total
legal_ingestion_duration_seconds

# Labeling
legal_labeling_samples_created
legal_labeling_confidence_distribution

# Training
legal_training_loss
legal_training_accuracy
legal_training_samples_per_second
```

### Health Checks

Before deployment, the system runs validation tests:

```yaml
validation_tests:
  - question: "Was bedeutet 'muss' in einem Verwaltungsakt?"
    expected_keywords: ["bindend", "Verpflichtung"]
    min_confidence: 0.8
```

### Auto-Rollback

Automatically rollback if quality degrades:

```yaml
auto_rollback:
  enabled: true
  triggers:
    accuracy_drop_threshold: 0.1  # 10% drop
    min_avg_rating: 3.0
    max_error_rate: 0.05
```

## Security & Compliance

### Data Protection
- Encryption at rest (AES-256-GCM)
- Field-level encryption for sensitive fields
- TLS 1.3 for data in transit

### Audit Logging
All operations are logged:
- Document ingestion (source, timestamp, user)
- Training sessions (config, samples, metrics)
- Deployments and rollbacks
- Human reviews of low-confidence samples

### GDPR Compliance
- PII detection and redaction
- Data retention policies
- Right to deletion (cascading)

## Troubleshooting

### Common Issues

#### 1. HuggingFace Connection Timeout
```bash
# Set API token
export HF_TOKEN="your_token_here"

# Increase timeout
options:
  timeout_seconds: 300
```

#### 2. OCR Fails for Scanned PDFs
```bash
# Install Tesseract
apt-get install tesseract-ocr tesseract-ocr-deu

# Verify installation
tesseract --version
```

#### 3. Out of Memory During Training
```yaml
training:
  batch_size: 2  # Reduce batch size
  gradient_accumulation_steps: 8  # Increase accumulation
```

#### 4. Low Confidence Samples
```sql
-- Review low-confidence samples
FOR sample IN legal_training_samples
    FILTER sample.confidence < 0.5
    FILTER sample.reviewed == false
    LIMIT 100
    RETURN sample
```

## References

- **PR #1**: Legal Modality Analyzer (Merged ✅)
- **Dataset**: [lexlms/ger_legal_data](https://huggingface.co/datasets/lexlms/ger_legal_data)
- **LoRA Paper**: [LoRA: Low-Rank Adaptation of Large Language Models](https://arxiv.org/abs/2106.09685)
- **ThemisDB Docs**: [LLM LoRA Integration](docs/en/llm/LLM_LORA_LLAMACPP_INTEGRATION.md)

## Next Steps

1. **Extend Connectors**: Add API and database connectors
2. **Enhance OCR**: Support more languages and formats
3. **Active Learning**: Human-in-the-loop for low-confidence samples
4. **Cross-Lingual**: Extend to other European languages
5. **Federation**: Multi-organization collaborative training

## Support

For questions or issues:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: https://makr-code.github.io/ThemisDB/
- Examples: `examples/legal_lora_training/`
