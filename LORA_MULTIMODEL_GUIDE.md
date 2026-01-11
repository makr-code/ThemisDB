# LoRA Multi-Model Architecture - Complete Guide

**Date**: 2026-01-11  
**Status**: ✅ Production Ready - Multi-Model Integration Complete

---

## 🎯 Overview

ThemisDB's LoRA framework fully embraces the **multi-model architecture**:

1. **Document Model** - LoRA adapters as BaseEntity documents
2. **Graph Model** - Relationships and lineage tracking
3. **Vector Model** - Semantic embeddings for similarity search

Plus **comprehensive audit logging** for complete traceability.

---

## 📊 Multi-Model Architecture

### Visual Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                    LoRA Adapter Multi-Model                      │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌──────────────────┐    ┌──────────────────┐                  │
│  │  Document Model  │    │   Graph Model    │                  │
│  │  (BaseEntity)    │◄───┤   (Edges)        │                  │
│  │                  │    │                  │                  │
│  │ • adapter_id     │    │ DERIVED_FROM     │                  │
│  │ • version        │    │ TRAINED_ON       │                  │
│  │ • metadata       │    │ RETRAINED        │                  │
│  │ • weights_ref    │    │ USED_IN          │                  │
│  └────────┬─────────┘    │ SIMILAR_TO       │                  │
│           │              └──────────────────┘                  │
│           │                                                      │
│           │              ┌──────────────────┐                  │
│           └──────────────┤  Vector Model    │                  │
│                          │  (Embeddings)    │                  │
│                          │                  │                  │
│                          │ • description    │                  │
│                          │ • task           │                  │
│                          │ • performance    │                  │
│                          │ • similarity     │                  │
│                          └──────────────────┘                  │
│                                                                  │
│  ┌────────────────────────────────────────────────────────┐   │
│  │              Audit Logging Layer                        │   │
│  │  Track: LLM + LoRA → Response                           │   │
│  └────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

---

## 1️⃣ Document Model (BaseEntity)

### Storage Format

Every LoRA adapter is stored as a **BaseEntity** with the following structure:

```cpp
// Create adapter as BaseEntity
BaseEntity::FieldMap fields;
fields["adapter_id"] = Value("themis_help_lora");
fields["version"] = Value("v2.1");
fields["base_model"] = Value("llama-2-7b");
fields["description"] = Value("Documentation assistance adapter");
fields["training_samples"] = Value(static_cast<int64_t>(5000));
fields["validation_accuracy"] = Value(0.92);

// Weights: inline or blob reference
if (size > 1MB) {
    auto ref = blob_manager->put(adapter_id, weights);
    fields["blob_ref_path"] = Value(ref.path);
} else {
    fields["weights_data"] = Value(weights);
}

BaseEntity entity = BaseEntity::fromFields(adapter_id, fields);
db->put("lora_adapters:themis_help_lora", entity.serialize());
```

### Document Structure

```json
{
  "_key": "themis_help_lora",
  "_id": "lora_adapters/themis_help_lora",
  "adapter_id": "themis_help_lora",
  "version": "v2.1",
  "base_model": "llama-2-7b",
  "base_model_version": "2.0",
  "description": "Documentation assistance adapter",
  "training_samples": 5000,
  "validation_accuracy": 0.92,
  "format": "safetensors",
  "size_bytes": 33554432,
  "blob_ref_path": "data/blobs/themis_help_lora.bin",
  "created_at": 1736601600,
  "updated_at": 1736605200,
  "created_by": "system",
  "tags": ["documentation", "qa", "help"]
}
```

---

## 2️⃣ Graph Model (Relationships)

### Graph Structure

LoRA adapters form a rich relationship graph:

```
[llama-2-7b Base Model]
         │
         │ DERIVED_FROM
         ▼
[themis_help_lora v1]
         │
         │ TRAINED_ON
         ▼
[Documentation Dataset]
         │
         │ (feedback collected)
         ▼
[themis_help_lora v2] ─────SIMILAR_TO─────► [themis_sql_lora]
         │                                            │
         │ USED_IN                           USED_IN │
         ▼                                            ▼
[Inference Session 1]                    [Inference Session 2]
         │                                            │
         │ FEEDBACK_FOR                  FEEDBACK_FOR │
         ▼                                            ▼
[User Feedback]                          [User Feedback]
```

### Edge Types (12 total)

| Edge Type | Description | Example |
|-----------|-------------|---------|
| `DERIVED_FROM` | Adapter derived from base | `themis_help_lora` → `llama-2-7b` |
| `TRAINED_ON` | Training dataset used | `themis_help_lora` → `docs_dataset` |
| `RETRAINED` | Incremental training | `v1` → `v2` |
| `FORKED_FROM` | Adapter forked | `custom_adapter` → `themis_help_lora` |
| `MERGED_INTO` | Adapters merged | `adapter_a` + `adapter_b` → `merged` |
| `USED_IN` | Inference sessions | `adapter` → `session_123` |
| `SIMILAR_TO` | Semantic similarity | `help_lora` ↔ `sql_lora` |
| `DEPENDS_ON` | Dependencies | `composed` → `base_adapter` |
| `VERSIONED_AS` | Version history | `v1` → `v2` → `v3` |
| `DEPLOYED_TO` | Deployment location | `adapter` → `shard_01` |
| `TRAINED_BY` | Training provenance | `adapter` ← `user_123` |
| `FEEDBACK_FOR` | User feedback | `feedback` → `adapter` |

### Graph Operations

```cpp
// Add edge
storage.addGraphEdge(
    "themis_help_lora_v1",           // from
    "themis_help_lora_v2",           // to
    LoRAEdgeType::RETRAINED,         // type
    0.95f                            // weight (similarity)
);

// Get edges
auto edges = storage.getGraphEdges("themis_help_lora", "both");
for (const auto& edge : edges) {
    std::cout << edge.from_id << " -[" 
              << edge.edgeTypeToString(edge.edge_type) 
              << "]-> " << edge.to_id << std::endl;
}

// Get lineage path
auto path = storage.getLineagePath("themis_help_lora_v2");
std::cout << "Lineage: " << path.toString() << std::endl;
// Output: llama-2-7b -[DERIVED_FROM]-> themis_help_lora_v1 -[RETRAINED]-> themis_help_lora_v2
```

### Graph Queries (AQL)

```sql
-- Find all adapters derived from llama-2-7b
FOR v, e IN 1..10 OUTBOUND 'llama-2-7b' lora_edges
    FILTER e.type == 'DERIVED_FROM'
    RETURN v.adapter_id

-- Find retraining chain
FOR v, e, p IN 1..10 OUTBOUND 'themis_help_lora_v1' lora_edges
    FILTER e.type == 'RETRAINED'
    RETURN p.vertices[*].adapter_id

-- Find similar adapters
FOR v IN lora_adapters
    FOR w IN 1..1 ANY v lora_edges
        FILTER w.type == 'SIMILAR_TO' AND w.weight > 0.8
        RETURN w
```

---

## 3️⃣ Vector Model (Embeddings)

### Vector Embeddings

Each adapter has multiple vector embeddings for semantic search:

```cpp
struct LoRAVectorEmbedding {
    std::string adapter_id;
    std::vector<float> embedding;        // 768-dim vector
    std::string embedding_model;         // "text-embedding-ada-002"
    Source source;                       // What was embedded
};

// Embedding sources:
enum class Source {
    DESCRIPTION,        // From adapter description
    HYPERPARAMETERS,    // From training config
    TRAINING_DATA,      // From training samples
    PERFORMANCE,        // From metrics (accuracy, loss)
    COMBINED            // Combined from all sources
};
```

### Creating Embeddings

```cpp
// Description embedding
LoRAVectorEmbedding desc_emb;
desc_emb.adapter_id = "themis_help_lora";
desc_emb.embedding = embed_model.encode(metadata.description);
desc_emb.embedding_model = "text-embedding-ada-002";
desc_emb.source = Source::DESCRIPTION;

storage.storeEmbedding("themis_help_lora", desc_emb);

// Hyperparameters embedding
LoRAVectorEmbedding hyper_emb;
hyper_emb.embedding = embed_model.encode(hyperparameters.toJSON().dump());
hyper_emb.source = Source::HYPERPARAMETERS;

storage.storeEmbedding("themis_help_lora", hyper_emb);
```

### Similarity Search

```cpp
// Find similar adapters
auto similar = storage.findSimilarAdapters(
    "themis_help_lora",     // reference adapter
    k = 10,                 // top 10 results
    threshold = 0.7f        // minimum similarity
);

for (const auto& [adapter_id, score] : similar) {
    std::cout << adapter_id << ": " << score << std::endl;
}

// Output:
// themis_sql_lora: 0.89
// themis_docs_lora: 0.85
// themis_qa_lora: 0.78
```

### Vector Queries (AQL)

```sql
-- Find adapters similar to a given one
FOR doc IN lora_adapters
    LET similarity = COSINE_SIMILARITY(
        doc.description_embedding,
        (SELECT description_embedding FROM lora_adapters 
         WHERE adapter_id == 'themis_help_lora')[0].description_embedding
    )
    FILTER similarity > 0.7
    SORT similarity DESC
    LIMIT 10
    RETURN {
        adapter_id: doc.adapter_id,
        similarity: similarity
    }

-- Semantic search by task description
FOR doc IN lora_adapters
    LET score = COSINE_SIMILARITY(
        doc.task_embedding,
        EMBED_TEXT('I need help with SQL query generation')
    )
    FILTER score > 0.6
    SORT score DESC
    LIMIT 5
    RETURN doc.adapter_id
```

---

## 4️⃣ Audit Logging (Traceability)

### Inference Audit

**Most Critical**: Track which LLM + which LoRA = which response

```cpp
LoRAInferenceAudit audit;

// Request identification
audit.request_id = generateRequestId();
audit.session_id = "session_abc123";
audit.user_id = "user_42";

// Model identification (CRITICAL)
audit.base_model_id = "llama-2-7b";
audit.base_model_version = "2.0";
audit.adapter_id = "themis_help_lora";
audit.adapter_version = "v2.1";
audit.adapter_hash = computeAdapterHash(weights);  // SHA256

// Inference details
audit.prompt = "How do I enable sharding?";
audit.response = "To enable sharding in ThemisDB...";
audit.input_tokens = 15;
audit.output_tokens = 120;

// Quality metrics
audit.confidence_score = 0.92f;
audit.perplexity = 12.5f;
audit.hallucination_detected = false;

// Configuration
audit.temperature = 0.7f;
audit.top_p = 0.9f;
audit.lora_scaling = 1.0f;

// Log it
audit_logger.logInference(audit);
```

### Audit Log File

Format: **JSON Lines** (one JSON object per line)

```json
{"timestamp":1736601600,"request_id":"a1b2c3","base_model_id":"llama-2-7b","base_model_version":"2.0","adapter_id":"themis_help_lora","adapter_version":"v2.1","adapter_hash":"sha256:abc123...","prompt":"How do I enable sharding?","response":"To enable sharding...","input_tokens":15,"output_tokens":120,"confidence_score":0.92,"temperature":0.7,"lora_scaling":1.0,"user_id":"user_42","success":true}
{"timestamp":1736601605,"request_id":"d4e5f6","base_model_id":"llama-2-7b","adapter_id":"themis_help_lora","adapter_version":"v2.1","event_type":"FEEDBACK_POSITIVE","user_id":"user_42"}
```

### Audit Queries

```cpp
// Get inference history for adapter
auto history = audit_logger.getInferenceHistory("themis_help_lora", limit=100);

for (const auto& audit : history) {
    std::cout << "Request " << audit.request_id 
              << ": " << audit.prompt 
              << " → " << audit.response << std::endl;
}

// Get statistics
auto stats = audit_logger.getAdapterStats("themis_help_lora");
std::cout << "Inferences: " << stats["inferences"] << std::endl;
std::cout << "Positive feedback: " << stats["positive_feedback"] << std::endl;
std::cout << "Negative feedback: " << stats["negative_feedback"] << std::endl;

// Query by time range
auto start = std::chrono::system_clock::now() - std::chrono::hours(24);
auto end = std::chrono::system_clock::now();
auto logs = audit_logger.queryLogs("themis_help_lora", start, end);
```

---

## 🔍 Complete Example: Multi-Model Query

### Scenario: Find Best Adapter for Task

```cpp
// 1. Semantic search (Vector Model)
std::string task = "I need an adapter for SQL query assistance";
auto candidates = storage.findSimilarAdapters(task, k=20, threshold=0.6);

// 2. Filter by graph relationships (Graph Model)
std::vector<std::string> filtered;
for (const auto& [adapter_id, score] : candidates) {
    auto edges = storage.getGraphEdges(adapter_id, "outgoing");
    
    // Check if used successfully in past
    bool has_successful_usage = false;
    for (const auto& edge : edges) {
        if (edge.edge_type == LoRAEdgeType::USED_IN && edge.weight > 0.8) {
            has_successful_usage = true;
            break;
        }
    }
    
    if (has_successful_usage) {
        filtered.push_back(adapter_id);
    }
}

// 3. Check quality metrics (Document Model)
std::string best_adapter;
float best_score = 0.0f;

for (const auto& adapter_id : filtered) {
    auto metadata = storage.loadMetadata(adapter_id);
    if (metadata && metadata->validation_accuracy > best_score) {
        best_score = metadata->validation_accuracy;
        best_adapter = adapter_id;
    }
}

// 4. Check audit history (Audit Logging)
auto history = audit_logger.getInferenceHistory(best_adapter, 100);
int successful = 0;
for (const auto& audit : history) {
    if (audit.success && audit.confidence_score > 0.8) {
        successful++;
    }
}

float success_rate = static_cast<float>(successful) / history.size();

std::cout << "Best adapter: " << best_adapter << std::endl;
std::cout << "  Validation accuracy: " << best_score << std::endl;
std::cout << "  Historical success rate: " << success_rate << std::endl;
```

---

## 🎓 Benefits of Multi-Model Approach

### 1. Rich Queries
- Combine document, graph, and vector queries
- Find adapters by relationships AND semantics
- Trace lineage while searching by similarity

### 2. Complete Traceability
- Every inference fully audited
- Track training provenance through graph
- Quality analysis from audit logs

### 3. Intelligent Recommendations
- Vector similarity for semantic matching
- Graph relationships for proven combinations
- Audit history for quality assurance

### 4. Compliance & Debugging
- Full audit trail for compliance (SOC2, GDPR)
- Easy debugging: "Which LLM + LoRA generated this?"
- Quality monitoring and A/B testing

---

## 📊 Storage Statistics

```
Multi-Model Storage Breakdown:

Document Storage (BaseEntity):
- Small adapters (< 1MB): Inline in RocksDB
- Large adapters (> 1MB): Blob storage
- Metadata: Always in BaseEntity

Graph Storage:
- Edges: ~100 bytes each
- Typical adapter: 5-10 edges
- Total: ~1KB per adapter

Vector Storage:
- Embedding: 768 × 4 bytes = 3KB per embedding
- Typical adapter: 3 embeddings (description, task, performance)
- Total: ~9KB per adapter

Audit Logs:
- Per inference: ~1-2KB (JSON Lines)
- Daily production: ~1M inferences = 1-2GB
- Compressed + encrypted: ~300-500MB/day
```

---

## ✅ Summary

The LoRA framework now provides:

1. **Document Model** - Flexible BaseEntity storage ✅
2. **Graph Model** - Rich relationship tracking ✅
3. **Vector Model** - Semantic similarity search ✅
4. **Audit Logging** - Complete traceability ✅

All integrated with ThemisDB's native infrastructure:
- BaseEntity principle
- RocksDB CRUD
- BlobStorage tiering
- Security (encryption, signatures)
- RAID redundancy

**Result**: Production-ready, enterprise-grade LoRA management system with complete multi-model support and audit traceability.

---

*Generated: 2026-01-11*
*Status: ✅ Complete Multi-Model Integration*
*Ready for: Production Deployment*
