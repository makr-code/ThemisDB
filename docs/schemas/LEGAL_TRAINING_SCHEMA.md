# Legal Training Data Schema for ThemisDB

![Status](https://img.shields.io/badge/status-ready-brightgreen)
![Version](https://img.shields.io/badge/version-1.0.0-blue)
![Format](https://img.shields.io/badge/format-YAML-orange)

## 📝 Overview

The Legal Training Data Schema provides a comprehensive multi-model database design for training domain-specific LoRA adapters on German legal language. It leverages ThemisDB's unique multi-model architecture using **YAML schema definition** to combine:

- **Document Model**: Legal texts, training samples, and metadata
- **Graph Model**: Relationships between documents, provisions, and case law
- **Vector Model**: Embeddings for semantic search and similarity
- **Relational Indexes**: Fast filtering and sorting

## 🎯 Use Cases

- **Legal LoRA Training**: Store and manage training data for legal language models
- **Knowledge Graph**: Build connections between legal documents and provisions
- **Semantic Search**: Find similar legal texts using vector embeddings
- **Auto-Labeling**: Track confidence scores and review status for ML-generated labels
- **Data Provenance**: Track ingestion sources and metadata

## 🏗️ Architecture

### Three-Layer Design

```
┌─────────────────────────────────────────────────┐
│  GRAPH LAYER: Knowledge Graph                   │
├─────────────────────────────────────────────────┤
│  legal_documents → paragraphs → sentences       │
│  sentences → modal_verbs                        │
│  legal_documents → legal_provisions             │
│  case_law → legal_provisions                    │
│  legal_documents ↔ legal_documents (similar)    │
└─────────────────────────────────────────────────┘
                     ↓
┌─────────────────────────────────────────────────┐
│  RELATIONAL LAYER: Training Data                │
├─────────────────────────────────────────────────┤
│  legal_training_samples (auto-labeled)          │
│  modal_verb_annotations (extracted)             │
│  ingestion_metadata (tracking)                  │
│  lora_adapters (stored adapters)                │
└─────────────────────────────────────────────────┘
                     ↓
┌─────────────────────────────────────────────────┐
│  VECTOR LAYER: Embeddings                       │
├─────────────────────────────────────────────────┤
│  legal_documents.embedding                      │
│  legal_training_samples.embedding               │
│  lora_adapters.embedding (for routing)          │
└─────────────────────────────────────────────────┘
```

## 📊 Collections

### Graph Layer

#### legal_documents
Source legal texts (laws, regulations, case law, custom documents).

**Schema:**
```json
{
  "text": "string (required)",
  "source": "string (required)",
  "source_type": "string",  // "huggingface", "filesystem", "api"
  "title": "string",
  "document_type": "string",  // "law", "regulation", "case_law", "custom"
  "date": "string",
  "authority": "string",
  "tags": "array",
  "priority": "number (default: 0)",
  "raw_data": "boolean (default: true)",
  "labeled_at": "string",
  "samples_created": "number",
  "metadata": "object",
  "embedding": "array",  // 768-dimensional E5-multilingual-base
  "created_at": "string",
  "updated_at": "string"
}
```

#### paragraphs
Structural elements of documents.

**Schema:**
```json
{
  "document_id": "string (required)",
  "index": "number (required)",
  "text": "string (required)",
  "start_char": "number",
  "end_char": "number"
}
```

#### sentences
Individual sentences for fine-grained analysis.

**Schema:**
```json
{
  "paragraph_id": "string (required)",
  "index": "number (required)",
  "text": "string (required)",
  "start_char": "number",
  "end_char": "number"
}
```

#### legal_provisions
Legal provisions and regulations (e.g., § 242 BGB).

**Schema:**
```json
{
  "provision_id": "string (required)",
  "law": "string",  // e.g., "BGB", "StGB"
  "section": "string",  // e.g., "§ 242"
  "text": "string",
  "interpretation": "string",
  "created_at": "string"
}
```

#### modal_verbs
Modal verbs extracted from sentences for normative analysis.

**Schema:**
```json
{
  "verb": "string (required)",
  "category": "string (required)",  // "obligation", "permission", "prohibition"
  "strength": "number",  // Normative strength [0, 1]
  "deontic_logic": "string",  // "MUST", "MAY", "MUST_NOT"
  "interpretation": "string",
  "position": "number",
  "sentence_id": "string"
}
```

#### case_law
Court decisions and case law.

**Schema:**
```json
{
  "case_id": "string (required)",
  "court": "string",
  "date": "string",
  "summary": "string",
  "full_text": "string",
  "provisions_cited": "array",
  "created_at": "string"
}
```

### Edge Collections

- **contains**: Document hierarchy (documents → paragraphs → sentences)
- **has_modality**: Links sentences/paragraphs to modal verbs
- **references**: Documents referencing legal provisions
- **interprets**: Case law interpreting provisions
- **similar_to**: Similar documents (semantic similarity)
- **supersedes**: Document version history
- **cited_in**: Citation relationships

### Named Graph

**legal_knowledge_graph** connects all collections through edge definitions.

### Relational Layer

#### legal_training_samples
Training data for LoRA adapters (auto-labeled and enriched).

**Schema:**
```json
{
  "input": "string (required)",
  "output": "string (required)",
  "category": "string (required)",  // "obligation", "permission", "prohibition"
  "strength": "number",
  "source_doc_id": "string",
  "source_type": "string",  // "huggingface", "custom", "legacy"
  "auto_labeled": "boolean (default: true)",
  "confidence": "number",  // [0, 1]
  "needs_review": "boolean (default: false)",
  "reviewed_by": "string",
  "review_date": "string",
  "context": "string",
  "graph_context": "string",
  "enrichment_sources": "number",
  "context_quality_score": "number",
  "embedding": "array",  // 768-dimensional
  "created_at": "string",
  "updated_at": "string"
}
```

#### modal_verb_annotations
Annotations extracted by NLP Analyzer.

**Schema:**
```json
{
  "document_id": "string (required)",
  "verb": "string (required)",
  "category": "string (required)",
  "position": "number",
  "context_before": "string",
  "context_after": "string",
  "deontic_logic": "string",
  "interpretation": "string",
  "confidence": "number",
  "sentence_index": "number",
  "created_at": "string"
}
```

#### ingestion_metadata
Track data ingestion from various sources.

**Schema:**
```json
{
  "source_id": "string (required)",
  "source_type": "string",
  "last_ingested": "string",
  "documents_count": "number",
  "status": "string",
  "error_message": "string",
  "config": "object",
  "created_at": "string",
  "updated_at": "string"
}
```

#### lora_adapters
Metadata for trained LoRA adapters.

**Schema:**
```json
{
  "adapter_id": "string (required)",
  "base_model": "string (required)",
  "domain": "string",
  "rank": "number",
  "alpha": "number",
  "training_samples": "number",
  "validation_accuracy": "number",
  "embedding": "array",  // For semantic routing
  "metadata": "object",
  "created_at": "string",
  "version": "string"
}
```

## 🔍 Indexes

### Full-Text Indexes
- `legal_documents_text`: Full-text search on document text
- `legal_documents_title`: Full-text search on titles
- `legal_training_samples_input`: Full-text search on training inputs

### Vector Indexes (HNSW)
- `legal_documents_embedding`: Semantic search on document embeddings (768D)
- `legal_training_samples_embedding`: Semantic search on training samples (768D)
- `lora_adapters_embedding`: Semantic routing for adapters (768D)

### Performance Indexes
- Document indexes: source, source_type, tags, raw_data, date, priority
- Training sample indexes: category, source_type, confidence, needs_review, auto_labeled
- Annotation indexes: document_id, category
- Ingestion indexes: source_id, status

## 📈 Views

### legal_training_view
Enriched training data with graph context.

```sql
FOR sample IN legal_training_samples
    LET doc = DOCUMENT(sample.source_doc_id)
    LET modalities = (
        FOR v IN 1..1 OUTBOUND doc contains, has_modality
            FILTER v._collection == "modal_verbs"
            RETURN v
    )
    LET related = (
        FOR v IN 1..2 OUTBOUND doc references, similar_to
            LIMIT 5
            RETURN v
    )
    RETURN MERGE(sample, {
        document: doc,
        modalities: modalities,
        related_docs: related
    })
```

### samples_needing_review
Samples that need human review, sorted by confidence.

```sql
FOR sample IN legal_training_samples
    FILTER sample.needs_review == true
    FILTER sample.reviewed_by == null OR sample.reviewed_by == ""
    SORT sample.confidence ASC
    RETURN sample
```

### documents_by_source
Document statistics grouped by source.

### samples_by_category
Training sample distribution by category.

### recent_ingestion_activity
Recent ingestion activity (last 20 items).

## 🚀 Usage Examples

### Schema Structure (YAML)

The schema is defined in YAML format following ThemisDB conventions:

```yaml
version: "1.0.0"

tables:
  legal_documents:
    description: "Source legal texts"
    primary_key: document_id
    fields:
      document_id:
        type: string
        format: uuid
        required: true
      text:
        type: text
        required: true
      embedding:
        type: vector
        dimensions: 768
    indexes:
      - name: idx_legal_documents_embedding
        columns: [embedding]
        type: vector
        algorithm: hnsw
```

### Insert a Legal Document (AQL)

```sql
INSERT {
    text: "Der Antragsteller muss die erforderlichen Unterlagen einreichen.",
    source: "custom",
    source_type: "filesystem",
    title: "Verwaltungsvorschrift BImSchG",
    document_type: "regulation",
    date: "2024-01-15",
    authority: "Umweltbundesamt",
    tags: ["BImSchG", "Genehmigung"],
    priority: 10,
    raw_data: true,
    created_at: DATE_ISO8601(DATE_NOW())
} INTO legal_documents
```

### Find Unlabeled Documents

```sql
FOR doc IN legal_documents
    FILTER doc.raw_data == true
    SORT doc.priority DESC
    LIMIT 100
    RETURN doc
```

### Get Training Samples by Category

```sql
FOR sample IN legal_training_samples
    FILTER sample.category == "obligation"
    FILTER sample.confidence >= 0.8
    RETURN sample
```

### Find Similar Documents

```sql
FOR doc IN legal_documents
    FILTER doc._key == "DOC-12345"
    FOR similar IN 1..1 OUTBOUND doc similar_to
        SORT similar.similarity_score DESC
        LIMIT 5
        RETURN similar
```

### Semantic Search with Vector Embeddings

```sql
FOR doc IN legal_documents
    FILTER COSINE_SIMILARITY(doc.embedding, @query_embedding) > 0.8
    SORT COSINE_SIMILARITY(doc.embedding, @query_embedding) DESC
    LIMIT 10
    RETURN doc
```

### Training Data with Graph Context

```sql
FOR sample IN legal_training_view
    FILTER sample.category == "obligation"
    LIMIT 100
    RETURN sample
```

### Track Ingestion Status

```sql
FOR meta IN ingestion_metadata
    FILTER meta.status == "failed"
    RETURN meta
```

## 🔧 Installation

### Initialize Schema

The schema is defined in YAML format and uploaded to ThemisDB:

```bash
# Set ThemisDB URL (optional)
export THEMISDB_URL="http://localhost:8529"

# Run initialization script
./scripts/init_legal_training_schema.sh
```

### Manual Initialization

```bash
# Upload YAML schema directly
curl -X POST "http://localhost:8529/schema" \
    -H "Content-Type: application/yaml" \
    --data-binary @config/schemas/legal_training_schema.yaml
```

## 📁 Schema File Format

The schema is defined in `config/schemas/legal_training_schema.yaml` using ThemisDB's YAML format:

- **Tables**: Define collections with fields, types, and validation
- **Indexes**: Full-text, vector (HNSW), and secondary indexes
- **Graphs**: Named graphs with vertex and edge definitions  
- **Views**: Pre-defined AQL queries for common operations
- **Configuration**: Performance, security, and monitoring settings

## ⚡ Performance Considerations

### Vector Indexes
- **HNSW** algorithm for fast approximate nearest neighbor search
- **768 dimensions** for E5-multilingual-base embeddings
- **Cosine similarity** metric for semantic search
- **Tunable parameters**: m=16, ef_construction=200, ef_search=100

### Composite Indexes
- Indexed on common query patterns (source_type, category, confidence)
- Array indexes for tags (tags[*])
- Priority and date indexes for sorting

### Graph Optimization
- Limited traversal depth (1-2 hops) for performance
- Edge type filtering for efficient queries
- Cached results for common graph patterns

### Full-Text Search
- Inverted indexes on text fields
- Fast phrase and fuzzy matching
- Language-aware tokenization (German)

## 🔐 Security Considerations

- **Data Provenance**: Track all data sources and ingestion metadata
- **Review Flags**: Mark low-confidence samples for human review
- **Audit Trail**: created_at, updated_at, reviewed_by timestamps
- **Versioning**: Version tracking for LoRA adapters

## 📚 Best Practices

### Data Ingestion
1. Set appropriate priority levels for processing order
2. Mark new documents with `raw_data: true`
3. Track ingestion metadata for all sources
4. Use consistent date formats (ISO 8601)

### Auto-Labeling
1. Store confidence scores for all auto-labeled data
2. Flag low-confidence samples for review (threshold: 0.8)
3. Record enrichment sources and quality scores
4. Maintain audit trail of review activities

### Graph Construction
1. Build document hierarchy first (documents → paragraphs → sentences)
2. Extract modal verbs and link to sentences
3. Identify provisions and create reference links
4. Compute similarity and create similar_to edges

### Vector Embeddings
1. Use consistent embedding model (E5-multilingual-base)
2. Generate embeddings for all documents and samples
3. Update embeddings when text changes
4. Normalize embeddings before storage

### Training Data
1. Balance categories (obligation, permission, prohibition)
2. Include graph context for better training
3. Review high-impact samples manually
4. Track training/validation split in metadata

## 🔗 Dependencies

- **ThemisDB Core**: Multi-model support
- **Vector Search**: HNSW indexes
- **Graph Engine**: AQL graph queries
- **No external dependencies**

## 🧪 Testing

Run the test suite to verify schema functionality:

```bash
# Build and run tests
./build.sh --tests
./build/tests/test_legal_training_schema
```

## 📄 License

This schema is part of ThemisDB and follows the same license.

## 🤝 Contributing

Contributions are welcome! Please follow ThemisDB contribution guidelines.

## 📞 Support

For issues and questions:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: https://docs.themisdb.com

---

**Next Steps**: Check out the [LoRA Training Guide](../training/LORA_TRAINING.md) to start training legal language models.
