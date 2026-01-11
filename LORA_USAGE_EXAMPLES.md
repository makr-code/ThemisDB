# LoRA Framework Usage Examples

This document provides practical examples for using the unified LLM + LoRA framework in ThemisDB.

## Table of Contents

1. [Basic Usage](#basic-usage)
2. [LLM Model Management](#llm-model-management)
3. [LoRA Adapter Management](#lora-adapter-management)
4. [themis_help_lora Application](#themis_help_lora-application)
5. [Audit Logging](#audit-logging)
6. [Advanced Queries](#advanced-queries)

---

## Basic Usage

### Initialize the Framework

```cpp
#include "llm/llm_model_storage.h"
#include "llm/lora_framework/lora_orchestrator.h"
#include "llm/applications/themis_help_lora.h"

// Initialize LLM model storage
LLMModelStorage::Config model_config;
model_config.db = db_wrapper;
model_config.blob_manager = blob_manager;
model_config.enable_encryption = true;
model_config.enable_signatures = true;

auto model_storage = std::make_shared<LLMModelStorage>(model_config);

// Initialize LoRA orchestrator
LoRAOrchestrator::Config lora_config;
lora_config.db = db_wrapper;
lora_config.blob_manager = blob_manager;
lora_config.enable_encryption = true;
lora_config.enable_signatures = true;

auto lora_orchestrator = std::make_shared<LoRAOrchestrator>(lora_config);
```

---

## LLM Model Management

### Register a New Model

```cpp
// Create model metadata
LLMModelMetadata metadata;
metadata.model_id = "llama-2-7b";
metadata.model_name = "Llama 2 7B";
metadata.version = "2.0";
metadata.architecture = "llama";
metadata.format = "gguf";
metadata.quantization = "Q4_K_M";
metadata.parameter_count = 7000000000;
metadata.context_length = 4096;
metadata.capabilities = {"text-generation", "chat", "embeddings"};
metadata.languages = {"en", "de", "fr", "es"};
metadata.tags = {"instruction-tuned", "chat"};
metadata.source = "huggingface";
metadata.license = "Apache-2.0";

// Load model file
std::vector<uint8_t> model_data = loadModelFile("/path/to/llama-2-7b.gguf");

// Store model as BaseEntity
bool stored = model_storage->storeModel(metadata, model_data);
if (stored) {
    std::cout << "Model registered successfully\n";
}
```

### Load and Query Model

```cpp
// Load model metadata
auto model = model_storage->loadModel("llama-2-7b");
if (model) {
    std::cout << "Model: " << model->model_name << "\n";
    std::cout << "Parameters: " << model->parameter_count << "\n";
    std::cout << "Context: " << model->context_length << "\n";
}

// List all models
auto models = model_storage->listModels();
for (const auto& model_id : models) {
    std::cout << "- " << model_id << "\n";
}
```

### Find Similar Models

```cpp
// Find models similar to llama-2-7b
auto similar = model_storage->findSimilarModels("llama-2-7b", 5, 0.7f);

std::cout << "Similar models:\n";
for (const auto& [model_id, similarity] : similar) {
    std::cout << "- " << model_id << " (similarity: " << similarity << ")\n";
}
```

### Track Model Relationships (Graph)

```cpp
// Link quantized version to base model
model_storage->addEdge(
    "llama-2-7b-fp16",      // from: base model
    "llama-2-7b-q4",        // to: quantized model
    LLMEdgeType::QUANTIZED_FROM,
    1.0f
);

// Link model to adapter
model_storage->addEdge(
    "llama-2-7b",
    "themis_help_lora",
    LLMEdgeType::ADAPTED_WITH,
    1.0f
);

// Query relationships
auto edges = model_storage->getEdges("llama-2-7b", "outgoing");
for (const auto& edge : edges) {
    std::cout << "Edge: " << edge["from"] << " -> " << edge["to"] 
              << " (" << edge["type"] << ")\n";
}
```

---

## LoRA Adapter Management

### Create a New Adapter

```cpp
// Prepare training data
TrainingData training_data;
training_data.base_model_id = "llama-2-7b";
training_data.description = "Documentation assistant for ThemisDB";
training_data.samples = {
    {"How do I enable sharding?", "To enable sharding..."},
    {"What is replication?", "Replication in ThemisDB..."},
    // ... more samples
};

// Create adapter
bool created = lora_orchestrator->createAdapter("themis_help_lora", training_data);
if (created) {
    std::cout << "Adapter created successfully\n";
}
```

### Load and Use Adapter

```cpp
// Load adapter
bool loaded = lora_orchestrator->loadAdapter("themis_help_lora");
if (loaded) {
    std::cout << "Adapter loaded\n";
}

// Check if loaded
if (lora_orchestrator->isAdapterLoaded("themis_help_lora")) {
    std::cout << "Adapter is ready for inference\n";
}

// Get adapter info
auto info = lora_orchestrator->getAdapter("themis_help_lora");
if (info) {
    std::cout << "Adapter: " << info->adapter_id << "\n";
    std::cout << "Base model: " << info->base_model << "\n";
    std::cout << "Version: " << info->version << "\n";
    std::cout << "Training samples: " << info->training_samples << "\n";
    std::cout << "Accuracy: " << info->validation_accuracy << "\n";
}
```

### Update Adapter (Incremental Training)

```cpp
// Collect new training data
TrainingData additional_data;
additional_data.base_model_id = "llama-2-7b";
additional_data.samples = {
    {"How do I configure backups?", "ThemisDB backup configuration..."},
    // ... more samples
};

// Update adapter
bool updated = lora_orchestrator->updateAdapter("themis_help_lora", additional_data);
if (updated) {
    std::cout << "Adapter updated with new training data\n";
}
```

### List and Delete Adapters

```cpp
// List all adapters
auto adapters = lora_orchestrator->listAdapters();
std::cout << "Available adapters:\n";
for (const auto& adapter_id : adapters) {
    std::cout << "- " << adapter_id << "\n";
}

// List adapters for specific base model
auto llama_adapters = lora_orchestrator->listAdapters("llama-2-7b");

// Delete adapter
bool deleted = lora_orchestrator->deleteAdapter("old_adapter");
if (deleted) {
    std::cout << "Adapter deleted\n";
}
```

---

## themis_help_lora Application

### Initialize Documentation Assistant

```cpp
// Configure themis_help_lora
ThemisHelpLoRA::Config config;
config.adapter_id = "themis_help_lora";
config.base_model_id = "llama-2-7b";
config.db = db_wrapper;
config.blob_manager = blob_manager;

// Create instance
auto doc_assistant = std::make_shared<ThemisHelpLoRA>(config);
```

### Query Documentation

```cpp
// Ask a question
std::string question = "How do I enable sharding in ThemisDB?";
std::string user_id = "user_123";

std::string answer = doc_assistant->query(question, user_id);
std::cout << "Q: " << question << "\n";
std::cout << "A: " << answer << "\n";
```

### Collect Feedback

```cpp
// Positive feedback
doc_assistant->addPositiveFeedback(
    "How do I enable sharding?",
    "To enable sharding, use CREATE COLLECTION... SHARD BY...",
    "user_123"
);

// Negative feedback with correction
doc_assistant->addNegativeFeedback(
    "How do I configure replication?",
    "Incorrect answer...",
    "The correct answer is: Use REPLICATION 3 in collection definition",
    "user_456"
);

// Get feedback statistics
auto feedback_stats = doc_assistant->getFeedbackStats();
std::cout << "Total feedback: " << feedback_stats.total_feedback << "\n";
std::cout << "Positive: " << feedback_stats.positive_feedback << "\n";
std::cout << "Negative: " << feedback_stats.negative_feedback << "\n";
std::cout << "Positive ratio: " << feedback_stats.positive_ratio << "\n";
```

### Train from Feedback

```cpp
// Train from collected feedback
bool trained = doc_assistant->trainFromFeedback();
if (trained) {
    std::cout << "Adapter retrained with user feedback\n";
    std::cout << "New version: " << doc_assistant->getVersion() << "\n";
}
```

### Initial Training from Documentation

```cpp
// Train from documentation corpus (one-time)
bool trained = doc_assistant->trainFromDocumentation();
if (trained) {
    std::cout << "Adapter trained on ThemisDB documentation\n";
}
```

### Get Performance Metrics

```cpp
auto metrics = doc_assistant->getMetrics();
std::cout << "Performance Metrics:\n";
std::cout << "- Total queries: " << metrics.total_queries << "\n";
std::cout << "- Successful: " << metrics.successful_queries << "\n";
std::cout << "- Failed: " << metrics.failed_queries << "\n";
std::cout << "- Success rate: " << (metrics.success_rate * 100) << "%\n";
std::cout << "- Avg latency: " << metrics.average_latency_ms << "ms\n";
```

---

## Audit Logging

### Query Audit Logs

```cpp
#include "llm/llm_model_audit_logger.h"
#include "llm/lora_framework/lora_audit_logger.h"

// Create audit loggers
LLMModelAuditLogger llm_audit;
LoRAAuditLogger lora_audit;

// Get inference history for model
auto model_history = llm_audit.getInferenceHistory("llama-2-7b", 100);
for (const auto& audit : model_history) {
    std::cout << "Request: " << audit.request_id << "\n";
    std::cout << "Model: " << audit.model_id << " + " << audit.lora_adapter_id << "\n";
    std::cout << "Prompt: " << audit.prompt << "\n";
    std::cout << "Response: " << audit.response << "\n";
    std::cout << "User: " << audit.user_id << "\n";
    std::cout << "Success: " << (audit.success ? "yes" : "no") << "\n";
    std::cout << "---\n";
}

// Get adapter-specific audit logs
auto adapter_logs = lora_audit.queryLogs("themis_help_lora");
for (const auto& log : adapter_logs) {
    std::cout << "Event: " << log["event_type"] << "\n";
    std::cout << "Timestamp: " << log["timestamp"] << "\n";
    std::cout << "Details: " << log["details"].dump(2) << "\n";
}

// Get statistics
auto stats = llm_audit.getModelStats("llama-2-7b");
std::cout << "Model Statistics:\n" << stats.dump(2) << "\n";
```

### Complete Traceability Example

```cpp
// This example shows complete traceability:
// Model + LoRA → Response

// 1. User asks question
std::string question = "How do I enable sharding?";
std::string user_id = "user_42";

// 2. Query documentation assistant
std::string response = doc_assistant->query(question, user_id);

// 3. Audit logs now contain:
//    - LLM model used (llama-2-7b)
//    - LoRA adapter used (themis_help_lora v2.1)
//    - Model version and checksum
//    - Adapter version and hash
//    - Prompt and response
//    - User ID
//    - Timestamp and performance metrics

// 4. Later, query audit logs to trace this inference
auto history = llm_audit.getInferenceHistory("llama-2-7b", 1);
if (!history.empty()) {
    const auto& audit = history[0];
    
    // Reconstruct what happened:
    std::cout << "Traceability Report:\n";
    std::cout << "==================\n";
    std::cout << "Request ID: " << audit.request_id << "\n";
    std::cout << "User: " << audit.user_id << "\n";
    std::cout << "Base Model: " << audit.model_id << " " << audit.model_version << "\n";
    std::cout << "Model Hash: " << audit.model_checksum << "\n";
    std::cout << "LoRA Adapter: " << audit.lora_adapter_id << " " << audit.lora_version << "\n";
    std::cout << "Prompt: " << audit.prompt << "\n";
    std::cout << "Response: " << audit.response << "\n";
    std::cout << "Performance: " << audit.tokens_per_second << " tokens/sec\n";
    std::cout << "VRAM Used: " << audit.vram_used_mb << " MB\n";
}
```

---

## Advanced Queries

### Multi-Model Query: Find Best Model + Adapter

```cpp
// Find best combination for SQL generation task
std::string task = "I need help with SQL query generation for ThemisDB";

// 1. Find suitable base models using vector similarity
auto candidate_models = model_storage->findSimilarModels(task, 5, 0.6f);

// 2. For each model, find compatible adapters
std::vector<std::pair<std::string, std::string>> combinations;

for (const auto& [model_id, model_score] : candidate_models) {
    // Get adapters for this model
    auto edges = model_storage->getEdges(model_id, "outgoing");
    
    for (const auto& edge : edges) {
        if (edge["type"] == "ADAPTED_WITH") {
            std::string adapter_id = edge["to"];
            combinations.push_back({model_id, adapter_id});
        }
    }
}

// 3. Check historical performance
std::string best_model;
std::string best_adapter;
float best_quality = 0.0f;

for (const auto& [model_id, adapter_id] : combinations) {
    auto history = llm_audit.getInferenceHistory(model_id, 100);
    
    int successful = 0;
    float avg_confidence = 0.0f;
    
    for (const auto& audit : history) {
        if (audit.lora_adapter_id == adapter_id &&
            audit.success &&
            audit.confidence_score > 0.8) {
            successful++;
            avg_confidence += audit.confidence_score;
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

std::cout << "Best combination for task:\n";
std::cout << "Model: " << best_model << "\n";
std::cout << "Adapter: " << best_adapter << "\n";
std::cout << "Historical quality: " << best_quality << "\n";
```

### Graph Traversal: Track Adapter Lineage

```cpp
// Track complete lineage of an adapter
std::string adapter_id = "themis_help_lora_v3";

// Get lineage path
auto path = lora_storage.getLineagePath(adapter_id);

std::cout << "Adapter Lineage:\n";
for (size_t i = 0; i < path.size(); ++i) {
    std::cout << std::string(i * 2, ' ') << "- " << path[i] << "\n";
}

// Output:
// Adapter Lineage:
// - llama-2-7b (base model)
//   - themis_help_lora_v1 (initial training)
//     - themis_help_lora_v2 (retrained with feedback)
//       - themis_help_lora_v3 (current version)
```

---

## REST API Examples

### Using the API Endpoints

```bash
# Create new adapter
curl -X POST https://themisdb.example.com/api/v1/llm/lora/create \
  -H "Authorization: Bearer $JWT_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "adapter_id": "my_custom_adapter",
    "base_model": "llama-2-7b",
    "training_data": {
      "samples": [
        {"input": "question 1", "output": "answer 1"},
        {"input": "question 2", "output": "answer 2"}
      ]
    }
  }'

# Get adapter info
curl -X GET https://themisdb.example.com/api/v1/llm/lora/my_custom_adapter \
  -H "Authorization: Bearer $JWT_TOKEN"

# List all adapters
curl -X GET https://themisdb.example.com/api/v1/llm/lora/list \
  -H "Authorization: Bearer $JWT_TOKEN"

# Update adapter (retrain)
curl -X PUT https://themisdb.example.com/api/v1/llm/lora/my_custom_adapter \
  -H "Authorization: Bearer $JWT_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "additional_training_data": {
      "samples": [
        {"input": "question 3", "output": "answer 3"}
      ]
    }
  }'

# Delete adapter
curl -X DELETE https://themisdb.example.com/api/v1/llm/lora/my_custom_adapter \
  -H "Authorization: Bearer $JWT_TOKEN"

# Query themis_help_lora
curl -X POST https://themisdb.example.com/api/v1/llm/docs/query \
  -H "Authorization: Bearer $JWT_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "question": "How do I enable sharding?"
  }'
```

---

## Summary

This unified framework provides:

1. **Complete Feature Parity**: LLM models and LoRA adapters have identical capabilities
2. **BaseEntity Storage**: Everything stored as documents in ThemisDB
3. **Multi-Model Integration**: Document + Graph + Vector for both
4. **Complete Audit Trail**: Track Model + LoRA → Response
5. **Enterprise Security**: Encryption, signatures, RAID by default

For more information, see:
- `LORA_FRAMEWORK_ANALYSIS.md` - Architecture overview
- `LLM_LORA_UNIFIED_ARCHITECTURE.md` - Unified architecture guide
- `BASEENTITY_PRINCIPLE.md` - BaseEntity compliance
