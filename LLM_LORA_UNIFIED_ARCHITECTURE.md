# Unified LLM + LoRA Architecture - Complete Guide

**Date**: 2026-01-11  
**Status**: ✅ Complete Multi-Model Integration for both LLM and LoRA

---

## 🎯 Overview

ThemisDB now has a **unified architecture** for both LLM models and LoRA adapters:

1. **Both are BaseEntity documents** - Consistent storage pattern
2. **Both have graph relationships** - Lineage and provenance tracking
3. **Both have vector embeddings** - Semantic similarity search
4. **Both have audit logging** - Complete traceability
5. **Both use same infrastructure** - Security, encryption, RAID, blob storage

**Key Insight**: LoRA adapters are specialized LLM adapters, but in ThemisDB they're just documents/blobs following the same BaseEntity principle.

---

## 📊 Unified Architecture

```
┌──────────────────────────────────────────────────────────────────┐
│                    Unified LLM Architecture                       │
├──────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌────────────────────┐           ┌────────────────────┐        │
│  │   LLM Base Models  │           │   LoRA Adapters    │        │
│  │   (llama-2-7b)     │◄─────────►│  (themis_help)     │        │
│  └────────────────────┘           └────────────────────┘        │
│           │                                 │                     │
│           │          Both follow            │                     │
│           │        BaseEntity Pattern       │                     │
│           │                                 │                     │
│           ▼                                 ▼                     │
│  ┌─────────────────────────────────────────────────────┐        │
│  │              Document Model (BaseEntity)             │        │
│  │  • Metadata as structured fields                     │        │
│  │  • Weights in blob storage (smart tiering)          │        │
│  └─────────────────────────────────────────────────────┘        │
│                                                                   │
│  ┌─────────────────────────────────────────────────────┐        │
│  │              Graph Model (Relationships)             │        │
│  │  • DERIVED_FROM, ADAPTED_WITH, QUANTIZED_FROM       │        │
│  │  • Lineage paths and provenance tracking            │        │
│  └─────────────────────────────────────────────────────┘        │
│                                                                   │
│  ┌─────────────────────────────────────────────────────┐        │
│  │              Vector Model (Embeddings)               │        │
│  │  • Semantic similarity search                        │        │
│  │  • Model/adapter recommendations                     │        │
│  └─────────────────────────────────────────────────────┘        │
│                                                                   │
│  ┌─────────────────────────────────────────────────────┐        │
│  │              Audit Logging Layer                     │        │
│  │  Track: Model + LoRA → Response (Complete trace)    │        │
│  └─────────────────────────────────────────────────────┘        │
└──────────────────────────────────────────────────────────────────┘
```

---

## 1️⃣ LLM Base Models (BaseEntity)

### Storage Structure

```cpp
// LLM model stored as BaseEntity
BaseEntity::FieldMap fields;
fields["model_id"] = Value("llama-2-7b");
fields["model_name"] = Value("Llama 2 7B");
fields["version"] = Value("2.0");
fields["architecture"] = Value("llama");
fields["quantization"] = Value("Q4_K_M");
fields["parameter_count"] = Value(static_cast<int64_t>(7000000000));
fields["context_length"] = Value(4096);

// Model file: inline or blob reference
if (size > 100MB) {
    auto ref = blob_manager->put(model_id, model_data);
    fields["blob_ref_path"] = Value(ref.path);
} else {
    fields["model_data"] = Value(model_data);
}

BaseEntity entity = BaseEntity::fromFields(model_id, fields);
db->put("llm_models:llama-2-7b", entity.serialize());
```

### Document Structure

```json
{
  "_key": "llama-2-7b",
  "_id": "llm_models/llama-2-7b",
  "model_id": "llama-2-7b",
  "model_name": "Llama 2 7B",
  "version": "2.0",
  "architecture": "llama",
  "format": "gguf",
  "quantization": "Q4_K_M",
  "size_bytes": 4368445440,
  "checksum": "sha256:abc123...",
  "parameter_count": 7000000000,
  "context_length": 4096,
  "vocabulary_size": 32000,
  "num_layers": 32,
  "capabilities": ["text-generation", "chat", "embeddings"],
  "languages": ["en", "de", "fr", "es"],
  "tags": ["instruction-tuned", "chat"],
  "vram_required_mb": 4500,
  "blob_ref_path": "data/blobs/llama-2-7b.gguf",
  "source": "huggingface",
  "license": "Apache-2.0",
  "created_at": 1736601600
}
```

---

## 2️⃣ LoRA Adapters (BaseEntity)

### Storage Structure

```cpp
// LoRA adapter stored as BaseEntity
BaseEntity::FieldMap fields;
fields["adapter_id"] = Value("themis_help_lora");
fields["base_model"] = Value("llama-2-7b");
fields["version"] = Value("v2.1");
fields["description"] = Value("Documentation assistant");
fields["training_samples"] = Value(static_cast<int64_t>(5000));
fields["validation_accuracy"] = Value(0.92);

// Adapter weights: inline or blob reference
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
  "base_model": "llama-2-7b",
  "version": "v2.1",
  "description": "Documentation assistance adapter",
  "training_samples": 5000,
  "validation_accuracy": 0.92,
  "format": "safetensors",
  "size_bytes": 33554432,
  "rank": 8,
  "alpha": 16.0,
  "blob_ref_path": "data/blobs/themis_help_lora.bin",
  "created_at": 1736601600
}
```

---

## 3️⃣ Graph Relationships

### Unified Graph Model

```
[llama-2-7b Base Model]
        │
        │ QUANTIZED_FROM
        ▼
[llama-2-7b-Q4_K_M]
        │
        │ ADAPTED_WITH
        ▼
[themis_help_lora v1]
        │
        │ TRAINED_ON
        ▼
[Documentation Dataset]
        │
        │ (feedback → retraining)
        ▼
[themis_help_lora v2] ─────SIMILAR_TO─────► [themis_sql_lora]
        │                                            │
        │ USED_IN                           USED_IN │
        ▼                                            ▼
[Inference Session 1]                    [Inference Session 2]
        │                                            │
        └────────────────────┬───────────────────────┘
                             │
                             ▼
                    [Unified Audit Log]
```

### Edge Types

**LLM Model Edges:**
- `DERIVED_FROM` - Fine-tuned from base
- `QUANTIZED_FROM` - Quantized version
- `MERGED_FROM` - Merged models
- `ADAPTED_WITH` - Uses LoRA adapter
- `SIMILAR_TO` - Semantic similarity
- `DEPLOYED_ON` - Deployment location

**LoRA Adapter Edges:**
- `DERIVED_FROM` - Adapter from base model
- `TRAINED_ON` - Training dataset
- `RETRAINED` - Incremental training (v1 → v2)
- `USED_IN` - Inference sessions
- `SIMILAR_TO` - Semantic similarity
- `FEEDBACK_FOR` - User feedback

### Graph Operations

```cpp
// Link model to adapter
model_storage.addEdge(
    "llama-2-7b",             // base model
    "themis_help_lora",       // LoRA adapter
    LLMEdgeType::ADAPTED_WITH,
    1.0f
);

// Link adapter to training data
lora_storage.addGraphEdge(
    "themis_help_lora",       // adapter
    "docs_dataset_v1",        // training data
    LoRAEdgeType::TRAINED_ON,
    1.0f
);

// Query: Find all adapters for a model
auto edges = model_storage.getEdges("llama-2-7b", "outgoing");
for (const auto& edge : edges) {
    if (edge["type"] == "ADAPTED_WITH") {
        std::cout << "Adapter: " << edge["to"] << std::endl;
    }
}
```

---

## 4️⃣ Vector Embeddings

### Both Models and Adapters Have Embeddings

```cpp
// Model embedding (from description + capabilities)
std::vector<float> model_embedding = embed_model.encode(
    model.model_name + " " + 
    model.architecture + " " +
    join(model.capabilities, " ")
);
model_storage.storeEmbedding("llama-2-7b", model_embedding);

// Adapter embedding (from description + task)
std::vector<float> adapter_embedding = embed_model.encode(
    adapter.description + " " +
    adapter.task_description
);
lora_storage.storeEmbedding("themis_help_lora", adapter_embedding);
```

### Semantic Search

```cpp
// Find similar models
auto similar_models = model_storage.findSimilarModels(
    "llama-2-7b",
    k = 5,
    threshold = 0.7f
);
// Results: mistral-7b, falcon-7b, llama-2-13b, ...

// Find similar adapters
auto similar_adapters = lora_storage.findSimilarAdapters(
    "themis_help_lora",
    k = 5,
    threshold = 0.7f
);
// Results: themis_sql_lora, themis_docs_lora, ...
```

---

## 5️⃣ Unified Audit Logging

### Complete Traceability: Model + LoRA → Response

```cpp
// Combined audit record
LLMModelInferenceAudit audit;

// Base model identification
audit.model_id = "llama-2-7b";
audit.model_version = "2.0";
audit.model_checksum = "sha256:abc123...";
audit.quantization = "Q4_K_M";

// LoRA adapter identification (if used)
audit.lora_adapter_id = "themis_help_lora";
audit.lora_version = "v2.1";

// Request details
audit.request_id = generateRequestId();
audit.user_id = "user_42";
audit.prompt = "How do I enable sharding?";
audit.response = "To enable sharding in ThemisDB...";

// Quality & performance
audit.confidence_score = 0.92f;
audit.tokens_per_second = 45.2f;
audit.vram_used_mb = 4500;

// Log it
model_audit_logger.logInference(audit);
```

### Audit Log Output (JSON Lines)

```json
{"timestamp":1736601600,"request_id":"abc123","model_id":"llama-2-7b","model_version":"2.0","model_checksum":"sha256:...","quantization":"Q4_K_M","lora_adapter_id":"themis_help_lora","lora_version":"v2.1","prompt":"How do I enable sharding?","response":"To enable sharding...","input_tokens":15,"output_tokens":120,"confidence_score":0.92,"tokens_per_second":45.2,"vram_used_mb":4500,"user_id":"user_42","success":true}
```

**Key Benefits:**
- ✅ Know **exactly** which model + which adapter → which response
- ✅ Full traceability for debugging and compliance
- ✅ Quality analysis and A/B testing
- ✅ Cost tracking (tokens, VRAM, time)
- ✅ GDPR/SOC2 compliance

---

## 6️⃣ Storage Strategy

### Smart Tiering for Both

| Data Type | Size | Storage Backend | Rationale |
|-----------|------|-----------------|-----------|
| **LLM Model** | < 100 MB | Inline in BaseEntity | Fast access |
| **LLM Model** | > 100 MB | Blob Storage (S3/Azure/FS) | Efficient for large files |
| **LoRA Adapter** | < 1 MB | Inline in BaseEntity | Fast access |
| **LoRA Adapter** | > 1 MB | Blob Storage | Efficient for large files |
| **Metadata** | Any | BaseEntity (RocksDB) | Structured queries |
| **Embeddings** | ~3 KB | BaseEntity or vector index | Similarity search |

---

## 7️⃣ Complete Example: Multi-Model Query

### Scenario: Find Best Model + Adapter for Task

```cpp
// 1. Find suitable base models (Vector search)
std::string task = "I need a model for code generation with SQL assistance";
auto candidate_models = model_storage.findSimilarModels(task, k=10, threshold=0.6);

// 2. For each model, find compatible adapters (Graph query)
std::vector<std::pair<std::string, std::string>> combinations;
for (const auto& [model_id, score] : candidate_models) {
    auto edges = model_storage.getEdges(model_id, "outgoing");
    
    for (const auto& edge : edges) {
        if (edge["type"] == "ADAPTED_WITH") {
            std::string adapter_id = edge["to"];
            
            // Check if adapter is suitable for task
            auto adapter_embedding = lora_storage.getEmbeddings(adapter_id);
            float adapter_score = cosine_similarity(task_embedding, adapter_embedding);
            
            if (adapter_score > 0.7) {
                combinations.push_back({model_id, adapter_id});
            }
        }
    }
}

// 3. Check audit history for quality (Audit logging)
std::string best_model;
std::string best_adapter;
float best_quality = 0.0f;

for (const auto& [model_id, adapter_id] : combinations) {
    // Get combined inference history
    auto history = model_audit_logger.getInferenceHistory(model_id, 100);
    
    int successful = 0;
    float avg_confidence = 0.0f;
    
    for (const auto& audit : history) {
        // Filter by adapter
        if (audit.lora_adapter_id == adapter_id) {
            if (audit.success && audit.confidence_score > 0.8) {
                successful++;
                avg_confidence += audit.confidence_score;
            }
        }
    }
    
    if (successful > 0) {
        avg_confidence /= successful;
        if (avg_confidence > best_quality) {
            best_quality = avg_confidence;
            best_model = model_id;
            best_adapter = adapter_id;
        }
    }
}

std::cout << "Best combination:" << std::endl;
std::cout << "  Model: " << best_model << std::endl;
std::cout << "  Adapter: " << best_adapter << std::endl;
std::cout << "  Historical quality: " << best_quality << std::endl;
```

---

## 8️⃣ API Examples

### Unified API Pattern

```cpp
// LLM Model Storage (BaseEntity)
LLMModelStorage model_storage(config);

// CREATE
model_storage.storeModel(metadata, model_data);

// READ
auto model = model_storage.loadModel("llama-2-7b");
auto models = model_storage.listModels();

// UPDATE
model_storage.updateModel("llama-2-7b", updated_metadata);
model_storage.updateUsageStats("llama-2-7b", tokens_generated);

// DELETE
model_storage.deleteModel("llama-2-7b");

// GRAPH
model_storage.addEdge("llama-2-7b", "themis_help_lora", ADAPTED_WITH);
auto edges = model_storage.getEdges("llama-2-7b");

// VECTOR
model_storage.storeEmbedding("llama-2-7b", embedding);
auto similar = model_storage.findSimilarModels("llama-2-7b", k=10);

// LoRA Adapter Storage (BaseEntity) - Same pattern!
LoRAStorageService lora_storage(config);

// CREATE, READ, UPDATE, DELETE - Same API
lora_storage.saveAdapter(adapter_id, weights, metadata);
auto adapter = lora_storage.loadAdapter("themis_help_lora");
lora_storage.updateMetadata("themis_help_lora", metadata);
lora_storage.deleteAdapter("themis_help_lora");

// GRAPH, VECTOR - Same API
lora_storage.addGraphEdge(from, to, RETRAINED);
auto similar = lora_storage.findSimilarAdapters("themis_help_lora", k=10);
```

---

## 9️⃣ Benefits of Unified Architecture

### 1. Consistency
- ✅ Same storage pattern for models and adapters
- ✅ Same API for CRUD, graph, vector operations
- ✅ Same audit logging format
- ✅ Same security (encryption, signatures)

### 2. Powerful Queries
- ✅ Combined document + graph + vector queries
- ✅ Find models by relationships AND semantics
- ✅ Track complete lineage (base model → adapter → inference)

### 3. Complete Traceability
- ✅ Know exactly: Model + Adapter → Response
- ✅ Full audit trail for compliance
- ✅ Quality analysis across models and adapters
- ✅ Cost tracking and optimization

### 4. Scalability
- ✅ Smart tiering: Inline for small, blob for large
- ✅ RAID redundancy for reliability
- ✅ Multi-tenant support
- ✅ Efficient caching

### 5. Flexibility
- ✅ Add new models without schema changes
- ✅ Add new adapters dynamically
- ✅ Evolve relationships over time
- ✅ Custom metadata per use case

---

## 🎓 Key Architectural Principles

### 1. Everything is a BaseEntity
**Models**, **adapters**, **datasets**, **inference sessions** - all stored as BaseEntity documents.

### 2. Graph for Relationships
**Lineage**, **dependencies**, **similarity** - captured in graph edges.

### 3. Vectors for Semantics
**Search**, **recommendations**, **clustering** - powered by vector embeddings.

### 4. Audit for Traceability
**Who**, **what**, **when**, **how** - complete audit trail.

### 5. Security by Default
**Encryption**, **signatures**, **RAID** - built into infrastructure.

---

## ✅ Implementation Status

### LLM Models ✅
- [x] `llm_model_storage.h` - BaseEntity storage
- [x] `llm_model_audit_logger.h` - Audit logging
- [x] Graph relationships
- [x] Vector embeddings
- [x] CRUD operations
- [x] Security features

### LoRA Adapters ✅
- [x] `lora_storage_service.h` - BaseEntity storage
- [x] `lora_audit_logger.h` - Audit logging
- [x] `lora_graph.h` - Graph & vector support
- [x] CRUD operations (orchestrator)
- [x] Security features

### Infrastructure ✅
- [x] BaseEntity compliance
- [x] RocksDB CRUD
- [x] BlobStorage smart tiering
- [x] Security (encryption + signatures)
- [x] RAID support
- [x] Unified audit logging

---

## 📝 Summary

**ThemisDB now has a world-class, unified architecture for LLM management:**

1. **Both models and adapters** follow BaseEntity principle
2. **Complete multi-model support**: Document + Graph + Vector
3. **Full audit traceability**: Model + LoRA → Response
4. **Enterprise security**: Encryption, signatures, RAID
5. **Powerful queries**: Combine semantics, relationships, and history

**Result**: Production-ready, enterprise-grade LLM & LoRA management system with complete multi-model integration and audit traceability.

---

*Generated: 2026-01-11*
*Status: ✅ Unified Architecture Complete*
*Ready for: Production Deployment*
