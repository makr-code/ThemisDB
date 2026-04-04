# LoRA Framework Usage Examples

This document provides practical examples for using the unified LLM + LoRA framework in ThemisDB.

## Table of Contents

1. [Docker Quick Start](#docker-quick-start) ⭐ **New!**
2. [Basic Usage](#basic-usage)
3. [LLM Model Management](#llm-model-management)
4. [LoRA Adapter Management](#lora-adapter-management)
5. [themis_help_lora Application](#themis_help_lora-application)
6. [Audit Logging](#audit-logging)
7. [Advanced Queries](#advanced-queries)

---

## Docker Quick Start

🐳 **Fastest way to start using the LoRA framework** - Complete environment ready in < 5 minutes!

### Start the Environment

```bash
# Navigate to docker directory
cd docker

# Start all services (ThemisDB + Prometheus + Grafana)
./scripts/start.sh

# Services are now running:
# - ThemisDB:   http://localhost:8529
# - Prometheus: http://localhost:9091
# - Grafana:    http://localhost:3000 (admin/admin)
```

### Quick Test

```bash
# Check ThemisDB is running
curl http://localhost:8529/health

# View LoRA metrics in Grafana
open http://localhost:3000  # Navigate to "LoRA Framework Overview" dashboard
```

### Run Tests in Docker

```bash
# Run integration tests
./scripts/test.sh

# View logs
./scripts/logs.sh themisdb

# Stop environment
./scripts/stop.sh
```

**Full Docker documentation**: See [docker/README.md](docker/README.md)

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

---

## AQL Function Examples

### Basic LORA_TRAIN Usage

Train a new adapter from a dataset:

```aql
// Train adapter with basic configuration
LET result = LORA_TRAIN(
  "documentation_assistant",
  "llama-2-7b",
  {
    "task": "documentation_qa",
    "samples": [
      {
        "input": "How do I enable sharding?",
        "output": "To enable sharding in ThemisDB, use the CREATE COLLECTION command..."
      },
      {
        "input": "What is replication?",
        "output": "Replication in ThemisDB provides data redundancy..."
      }
    ]
  },
  {
    "rank": 8,
    "alpha": 16,
    "learning_rate": 0.0003,
    "epochs": 3
  }
)

RETURN result
```

### Batch Training from Collection

Train multiple adapters from a collection of datasets:

```aql
FOR dataset IN training_datasets
  FILTER dataset.status == "ready"
  FILTER dataset.sample_count >= 1000
  LET job = LORA_TRAIN(
    CONCAT(dataset.task_type, "_lora_", DATE_ISO8601(DATE_NOW())),
    dataset.base_model,
    dataset,
    {
      "rank": dataset.config.rank || 8,
      "alpha": dataset.config.alpha || 16,
      "learning_rate": 0.0003,
      "epochs": 3
    }
  )
  RETURN {
    adapter_id: job.adapter_id,
    job_id: job.job_id,
    status: job.status,
    estimated_completion: job.estimated_completion
  }
```

### Simple LORA_QUERY Usage

Execute inference with a LoRA adapter:

```aql
// Single query
LET answer = LORA_QUERY(
  "llama-2-7b",
  "documentation_assistant",
  "How do I configure backups?",
  {
    "max_tokens": 500,
    "temperature": 0.7
  }
)

RETURN {
  question: "How do I configure backups?",
  answer: answer,
  timestamp: DATE_NOW()
}
```

### Batch Query Processing

Process multiple questions:

```aql
FOR question IN user_questions
  FILTER question.status == "pending"
  FILTER question.category == "documentation"
  
  LET answer = LORA_QUERY(
    "llama-2-7b",
    "documentation_assistant",
    question.text,
    {
      "max_tokens": 500,
      "temperature": 0.7
    }
  )
  
  UPDATE question WITH {
    answer: answer,
    status: "answered",
    answered_at: DATE_NOW()
  } IN user_questions
  
  RETURN {
    question_id: question._key,
    question: question.text,
    answer: answer
  }
```

### Adaptive Query Routing

Automatically select best adapter for each query:

```aql
FOR query IN user_queries
  // Get recommendation for best adapter
  LET recommendation = LORA_RECOMMEND(
    query.text,
    query.preferred_model || "llama-2-7b",
    query.category,
    {
      "min_accuracy": 0.85,
      "max_latency_ms": 100
    }
  )
  
  // Execute query with recommended adapter
  LET answer = recommendation.adapter_id != null 
    ? LORA_QUERY(
        query.preferred_model || "llama-2-7b",
        recommendation.adapter_id,
        query.text,
        {}
      )
    : "No suitable adapter found"
  
  RETURN {
    query_id: query._key,
    query: query.text,
    answer: answer,
    adapter_used: recommendation.adapter_id,
    confidence: recommendation.confidence,
    reason: recommendation.reason
  }
```

### Find Similar Adapters

Discover related adapters:

```aql
// Find adapters similar to a specific adapter
LET similar = LORA_SIMILAR("documentation_assistant", 5, 0.85)

FOR adapter IN similar
  LET stats = LORA_STATS(adapter.adapter_id, ["validation_accuracy", "inference_count"])
  RETURN {
    adapter_id: adapter.adapter_id,
    similarity_score: adapter.score,
    task: adapter.task,
    accuracy: stats.validation_accuracy,
    usage_count: stats.inference_count
  }
```

### Adapter Performance Analysis

Analyze and rank adapters by performance:

```aql
FOR adapter IN lora_adapters
  FILTER adapter.base_model == "llama-2-7b"
  FILTER adapter.status == "ready"
  
  LET stats = LORA_STATS(
    adapter.adapter_id,
    ["validation_accuracy", "inference_count", "avg_latency", "cache_hit_rate"]
  )
  
  // Calculate composite performance score
  LET performance_score = (
    stats.validation_accuracy * 0.5 +
    (MIN([stats.cache_hit_rate, 1.0])) * 0.3 +
    (1.0 - MIN([stats.avg_latency_ms / 1000, 1.0])) * 0.2
  )
  
  SORT performance_score DESC
  LIMIT 10
  
  RETURN {
    rank: ROW_NUMBER(),
    adapter_id: adapter.adapter_id,
    task: adapter.task,
    performance_score: ROUND(performance_score, 3),
    accuracy: ROUND(stats.validation_accuracy, 3),
    latency_ms: stats.avg_latency_ms,
    usage: stats.inference_count,
    cache_hit_rate: ROUND(stats.cache_hit_rate, 2)
  }
```

### Track Adapter Evolution

Monitor adapter improvements over versions:

```aql
FOR adapter IN lora_adapters
  FILTER adapter.adapter_id == "documentation_assistant"
  
  LET lineage = LORA_LINEAGE(adapter.adapter_id, 20)
  
  FOR version IN lineage
    LET stats = LORA_STATS(
      CONCAT(adapter.adapter_id, "@", version.version),
      ["validation_accuracy"]
    )
    
    LET parent_stats = version.parent != null
      ? LORA_STATS(
          CONCAT(adapter.adapter_id, "@", version.parent),
          ["validation_accuracy"]
        )
      : null
    
    LET improvement = parent_stats != null
      ? stats.validation_accuracy - parent_stats.validation_accuracy
      : 0
    
    RETURN {
      version: version.version,
      created: version.created,
      accuracy: ROUND(stats.validation_accuracy, 4),
      improvement: ROUND(improvement, 4),
      improvement_pct: parent_stats != null 
        ? ROUND((improvement / parent_stats.validation_accuracy) * 100, 2)
        : null
    }
```

### Model Adaptation Path Discovery

Find paths between models using adapters:

```aql
FOR source_model IN ["llama-2-7b", "mistral-7b"]
  FOR target_model IN ["llama-2-13b", "llama-2-70b"]
    FILTER source_model != target_model
    
    LET path = LORA_PATH(source_model, target_model, 5)
    FILTER LENGTH(path) > 0
    
    LET adapters_in_path = (
      FOR step IN path
        FILTER step.type == "adapter"
        RETURN step.node
    )
    
    RETURN {
      from: source_model,
      to: target_model,
      path_length: LENGTH(path),
      adapters_used: adapters_in_path,
      full_path: path[*].node
    }
```

### Multi-Task Adapter Recommendation

Compare adapters across different tasks:

```aql
FOR task IN ["documentation_qa", "code_generation", "sql_translation"]
  LET sample_query = task == "documentation_qa"
    ? "How do I configure replication?"
    : task == "code_generation"
    ? "Generate a Python function to parse JSON"
    : "Convert this query to SQL"
  
  LET recommendation = LORA_RECOMMEND(
    sample_query,
    "llama-2-7b",
    task,
    {
      "min_accuracy": 0.80,
      "max_latency_ms": 150
    }
  )
  
  RETURN {
    task: task,
    recommended_adapter: recommendation.adapter_id,
    confidence: ROUND(recommendation.confidence, 3),
    reason: recommendation.reason,
    metrics: recommendation.metrics
  }
```

### Comprehensive Adapter Dashboard

Create a complete overview of adapter ecosystem:

```aql
LET total_adapters = LENGTH(lora_adapters)

LET by_model = (
  FOR adapter IN lora_adapters
    COLLECT model = adapter.base_model WITH COUNT INTO count
    RETURN {
      model: model,
      adapter_count: count
    }
)

LET by_task = (
  FOR adapter IN lora_adapters
    COLLECT task = adapter.task WITH COUNT INTO count
    SORT count DESC
    LIMIT 10
    RETURN {
      task: task,
      adapter_count: count
    }
)

LET top_performers = (
  FOR adapter IN lora_adapters
    FILTER adapter.status == "ready"
    LET stats = LORA_STATS(adapter.adapter_id, ["validation_accuracy", "inference_count"])
    FILTER stats.validation_accuracy >= 0.85
    SORT stats.inference_count DESC
    LIMIT 5
    RETURN {
      adapter_id: adapter.adapter_id,
      accuracy: ROUND(stats.validation_accuracy, 3),
      usage: stats.inference_count
    }
)

LET recent_training = (
  FOR adapter IN lora_adapters
    FILTER adapter.created_at >= DATE_SUBTRACT(DATE_NOW(), 7, "days")
    SORT adapter.created_at DESC
    LIMIT 10
    RETURN {
      adapter_id: adapter.adapter_id,
      task: adapter.task,
      created: adapter.created_at
    }
)

RETURN {
  overview: {
    total_adapters: total_adapters,
    by_model: by_model,
    by_task: by_task
  },
  top_performers: top_performers,
  recent_training: recent_training,
  generated_at: DATE_ISO8601(DATE_NOW())
}
```

### A/B Testing Adapters

Compare performance of different adapters on the same queries:

```aql
LET test_queries = [
  "How do I enable sharding?",
  "What is replication?",
  "Configure backup settings"
]

LET adapters_to_test = [
  "documentation_assistant_v1",
  "documentation_assistant_v2",
  "documentation_assistant_v3"
]

FOR query IN test_queries
  LET results = (
    FOR adapter_id IN adapters_to_test
      LET start_time = DATE_NOW()
      
      LET answer = LORA_QUERY(
        "llama-2-7b",
        adapter_id,
        query,
        {"max_tokens": 500, "temperature": 0.7}
      )
      
      LET end_time = DATE_NOW()
      LET latency_ms = DATE_DIFF(start_time, end_time, "millisecond", true)
      
      LET stats = LORA_STATS(adapter_id, ["validation_accuracy"])
      
      RETURN {
        adapter_id: adapter_id,
        answer: answer,
        latency_ms: latency_ms,
        accuracy: stats.validation_accuracy
      }
  )
  
  RETURN {
    query: query,
    results: results,
    winner: FIRST(
      FOR r IN results
        SORT r.accuracy DESC, r.latency_ms ASC
        RETURN r.adapter_id
    )
  }
```

---

## Related Documentation

- [LoRA AQL Reference](LORA_AQL_REFERENCE.md) - Complete function reference
- [LoRA Framework Analysis](LORA_FRAMEWORK_ANALYSIS.md) - Architecture overview
- [AQL Documentation](docs/aql_reference.md) - AQL language reference

---

Last updated: 2026-01-11

---

## REST API Usage

### API Endpoint Overview

The LoRA framework provides a complete REST API for remote management and inference. All endpoints require JWT Bearer Token authentication.

**Base URL:** `http://localhost:8080/api/v1/llm`

**Authentication:**
```bash
export TOKEN="your-jwt-token-here"
```

### Register a New Model

```bash
curl -X POST http://localhost:8080/api/v1/llm/models \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "model_id": "llama-2-7b",
    "architecture": "llama",
    "parameter_count": 7000000000,
    "quantization": "Q4_K_M",
    "gguf_path": "/models/llama-2-7b-Q4.gguf",
    "description": "Llama 2 7B model with Q4 quantization"
  }'
```

**Response:**
```json
{
  "model_id": "llama-2-7b",
  "status": "registered",
  "timestamp": "2026-01-11T14:00:00Z"
}
```

### Create a LoRA Adapter

```bash
curl -X POST http://localhost:8080/api/v1/llm/lora/adapters \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "adapter_id": "themis_help_lora",
    "base_model": "llama-2-7b",
    "task": "documentation_qa",
    "rank": 8,
    "alpha": 16,
    "training_data": {
      "dataset_id": "docs_v1",
      "samples": 10000
    },
    "description": "Documentation Q&A adapter"
  }'
```

**Response:**
```json
{
  "adapter_id": "themis_help_lora",
  "version": "v1.0",
  "status": "training",
  "job_id": "job_123"
}
```

### List All Adapters

```bash
# List all adapters
curl -X GET "http://localhost:8080/api/v1/llm/lora/adapters" \
  -H "Authorization: Bearer $TOKEN"

# Filter by base model and status
curl -X GET "http://localhost:8080/api/v1/llm/lora/adapters?base_model=llama-2-7b&status=ready&limit=20" \
  -H "Authorization: Bearer $TOKEN"
```

**Response:**
```json
{
  "adapters": [
    {
      "adapter_id": "themis_help_lora",
      "base_model": "llama-2-7b",
      "status": "ready",
      "is_loaded": true,
      "version": "v1.0"
    }
  ],
  "total": 15,
  "limit": 10,
  "offset": 0
}
```

### Get Adapter Details

```bash
curl -X GET http://localhost:8080/api/v1/llm/lora/adapters/themis_help_lora \
  -H "Authorization: Bearer $TOKEN"
```

**Response:**
```json
{
  "adapter_id": "themis_help_lora",
  "base_model": "llama-2-7b",
  "version": "v1.0",
  "status": "ready",
  "is_loaded": true,
  "metrics": {
    "validation_accuracy": 0.92,
    "training_loss": 0.15
  },
  "created_at": "2026-01-11T14:30:00Z",
  "hyperparameters": {
    "rank": 8,
    "alpha": 16,
    "dropout": 0.1
  }
}
```

### Load Adapter into Memory

```bash
curl -X POST http://localhost:8080/api/v1/llm/lora/adapters/themis_help_lora/load \
  -H "Authorization: Bearer $TOKEN"
```

**Response:**
```json
{
  "adapter_id": "themis_help_lora",
  "status": "loaded",
  "load_time_ms": 45
}
```

### Check Adapter Status

```bash
curl -X GET http://localhost:8080/api/v1/llm/lora/adapters/themis_help_lora/status \
  -H "Authorization: Bearer $TOKEN"
```

**Response:**
```json
{
  "adapter_id": "themis_help_lora",
  "is_loaded": true,
  "memory_usage_mb": 32,
  "last_used": "2026-01-11T15:00:00Z"
}
```

### Query with LoRA Adapter

```bash
curl -X POST http://localhost:8080/api/v1/llm/lora/query \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "model_id": "llama-2-7b",
    "adapter_id": "themis_help_lora",
    "prompt": "How do I enable sharding in ThemisDB?",
    "max_tokens": 500,
    "temperature": 0.7,
    "user_id": "user_42"
  }'
```

**Response:**
```json
{
  "response": "To enable sharding in ThemisDB, you need to configure the sharding settings in your themis.conf file. First, set 'sharding.enabled = true', then define your shard key using 'sharding.key = @collection_name'. You can also specify the number of shards with 'sharding.count = 4'. After configuration, restart ThemisDB and use the SHARD keyword in your AQL queries...",
  "model_id": "llama-2-7b",
  "adapter_id": "themis_help_lora",
  "tokens_used": 145,
  "inference_time_ms": 850,
  "audit_id": "audit_789"
}
```

### Update Adapter with New Training Data

```bash
curl -X PUT http://localhost:8080/api/v1/llm/lora/adapters/themis_help_lora \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "additional_training_data": {
      "dataset_id": "feedback_v1",
      "samples": 500
    }
  }'
```

**Response:**
```json
{
  "adapter_id": "themis_help_lora",
  "version": "v1.1",
  "status": "training",
  "job_id": "job_124"
}
```

### Get Framework Statistics

```bash
curl -X GET http://localhost:8080/api/v1/llm/lora/stats \
  -H "Authorization: Bearer $TOKEN"
```

**Response:**
```json
{
  "total_adapters": 15,
  "loaded_adapters": 3,
  "cache_hit_rate": 0.842,
  "total_inferences": 1234567,
  "avg_load_time_ms": 450,
  "uptime_seconds": 864000
}
```

### Health Check

```bash
curl -X GET http://localhost:8080/api/v1/llm/lora/health \
  -H "Authorization: Bearer $TOKEN"
```

**Response:**
```json
{
  "status": "healthy",
  "storage": "ok",
  "manager": "ok",
  "training": "ok",
  "checks_passed": 3,
  "checks_failed": 0
}
```

### Unload Adapter from Memory

```bash
curl -X POST http://localhost:8080/api/v1/llm/lora/adapters/themis_help_lora/unload \
  -H "Authorization: Bearer $TOKEN"
```

**Response:**
```json
{
  "adapter_id": "themis_help_lora",
  "status": "unloaded"
}
```

### Delete Adapter

```bash
# Delete specific version
curl -X DELETE "http://localhost:8080/api/v1/llm/lora/adapters/themis_help_lora?version=v1.0" \
  -H "Authorization: Bearer $TOKEN"

# Delete all versions
curl -X DELETE http://localhost:8080/api/v1/llm/lora/adapters/themis_help_lora \
  -H "Authorization: Bearer $TOKEN"
```

**Response:** `204 No Content`

### Python Example

```python
import requests
import json

# Configuration
BASE_URL = "http://localhost:8080/api/v1/llm"
TOKEN = "your-jwt-token-here"
HEADERS = {
    "Authorization": f"Bearer {TOKEN}",
    "Content-Type": "application/json"
}

# Create adapter
create_data = {
    "adapter_id": "customer_support_lora",
    "base_model": "llama-2-7b",
    "task": "customer_support",
    "rank": 8,
    "alpha": 16,
    "training_data": {
        "dataset_id": "support_tickets_2024",
        "samples": 5000
    }
}

response = requests.post(
    f"{BASE_URL}/lora/adapters",
    headers=HEADERS,
    json=create_data
)
print("Create adapter:", response.json())

# Load adapter
response = requests.post(
    f"{BASE_URL}/lora/adapters/customer_support_lora/load",
    headers=HEADERS
)
print("Load adapter:", response.json())

# Query with adapter
query_data = {
    "model_id": "llama-2-7b",
    "adapter_id": "customer_support_lora",
    "prompt": "How do I reset my password?",
    "max_tokens": 300,
    "temperature": 0.7
}

response = requests.post(
    f"{BASE_URL}/lora/query",
    headers=HEADERS,
    json=query_data
)
print("Query response:", response.json()["response"])

# Get statistics
response = requests.get(
    f"{BASE_URL}/lora/stats",
    headers=HEADERS
)
print("Statistics:", response.json())
```

### JavaScript Example

```javascript
const BASE_URL = 'http://localhost:8080/api/v1/llm';
const TOKEN = 'your-jwt-token-here';

const headers = {
  'Authorization': `Bearer ${TOKEN}`,
  'Content-Type': 'application/json'
};

// Create adapter
async function createAdapter() {
  const response = await fetch(`${BASE_URL}/lora/adapters`, {
    method: 'POST',
    headers: headers,
    body: JSON.stringify({
      adapter_id: 'chatbot_lora',
      base_model: 'llama-2-7b',
      task: 'conversational',
      rank: 8,
      alpha: 16,
      training_data: {
        dataset_id: 'conversations_2024',
        samples: 8000
      }
    })
  });
  
  const data = await response.json();
  console.log('Adapter created:', data);
  return data;
}

// Query with adapter
async function queryAdapter(adapterId, prompt) {
  const response = await fetch(`${BASE_URL}/lora/query`, {
    method: 'POST',
    headers: headers,
    body: JSON.stringify({
      model_id: 'llama-2-7b',
      adapter_id: adapterId,
      prompt: prompt,
      max_tokens: 500,
      temperature: 0.7
    })
  });
  
  const data = await response.json();
  console.log('Response:', data.response);
  return data;
}

// Usage
(async () => {
  const adapter = await createAdapter();
  const result = await queryAdapter('chatbot_lora', 'Hello, how can you help me?');
})();
```

### Complete Workflow Example

```bash
#!/bin/bash
# complete-lora-workflow.sh

TOKEN="your-jwt-token-here"
BASE_URL="http://localhost:8080/api/v1/llm"

# 1. Register model
echo "1. Registering model..."
curl -X POST ${BASE_URL}/models \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "model_id": "llama-2-7b",
    "architecture": "llama",
    "parameter_count": 7000000000
  }'

# 2. Create adapter
echo -e "\n2. Creating adapter..."
ADAPTER_RESPONSE=$(curl -X POST ${BASE_URL}/lora/adapters \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "adapter_id": "support_lora",
    "base_model": "llama-2-7b",
    "rank": 8,
    "alpha": 16,
    "training_data": {"dataset_id": "support_data", "samples": 10000}
  }')
echo $ADAPTER_RESPONSE

# Extract job_id from response (requires jq)
JOB_ID=$(echo $ADAPTER_RESPONSE | jq -r '.job_id')
echo "Job ID: $JOB_ID"

# 3. Wait for training (simplified - would poll job status in production)
echo -e "\n3. Waiting for training to complete..."
sleep 60

# 4. Load adapter
echo -e "\n4. Loading adapter..."
curl -X POST ${BASE_URL}/lora/adapters/support_lora/load \
  -H "Authorization: Bearer $TOKEN"

# 5. Check status
echo -e "\n5. Checking adapter status..."
curl -X GET ${BASE_URL}/lora/adapters/support_lora/status \
  -H "Authorization: Bearer $TOKEN"

# 6. Perform inference
echo -e "\n6. Running inference..."
curl -X POST ${BASE_URL}/lora/query \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "model_id": "llama-2-7b",
    "adapter_id": "support_lora",
    "prompt": "How do I contact support?",
    "max_tokens": 300
  }'

# 7. Get statistics
echo -e "\n7. Getting statistics..."
curl -X GET ${BASE_URL}/lora/stats \
  -H "Authorization: Bearer $TOKEN"

echo -e "\n\nWorkflow complete!"
```

### Error Handling Example

```python
import requests
from requests.exceptions import RequestException

def query_with_retry(prompt, adapter_id, max_retries=3):
    """Query with automatic retry on failure"""
    
    BASE_URL = "http://localhost:8080/api/v1/llm"
    TOKEN = "your-jwt-token-here"
    
    headers = {
        "Authorization": f"Bearer {TOKEN}",
        "Content-Type": "application/json"
    }
    
    payload = {
        "model_id": "llama-2-7b",
        "adapter_id": adapter_id,
        "prompt": prompt,
        "max_tokens": 500
    }
    
    for attempt in range(max_retries):
        try:
            response = requests.post(
                f"{BASE_URL}/lora/query",
                headers=headers,
                json=payload,
                timeout=30
            )
            
            # Check status code
            if response.status_code == 200:
                return response.json()
            elif response.status_code == 401:
                print("Authentication failed. Check your token.")
                return None
            elif response.status_code == 404:
                print(f"Adapter '{adapter_id}' not found.")
                return None
            elif response.status_code == 429:
                # Rate limited
                retry_after = int(response.headers.get('Retry-After', 60))
                print(f"Rate limited. Waiting {retry_after} seconds...")
                time.sleep(retry_after)
                continue
            else:
                error_data = response.json()
                print(f"Error: {error_data.get('error', 'Unknown error')}")
                return None
                
        except RequestException as e:
            print(f"Request failed (attempt {attempt + 1}/{max_retries}): {e}")
            if attempt < max_retries - 1:
                time.sleep(2 ** attempt)  # Exponential backoff
            
    return None

# Usage
result = query_with_retry("How do I configure sharding?", "themis_help_lora")
if result:
    print("Response:", result["response"])
```

### API Integration Best Practices

1. **Authentication Management**
   - Store tokens securely (environment variables, secret managers)
   - Refresh tokens before expiration
   - Implement token rotation

2. **Error Handling**
   - Always check status codes
   - Implement retry logic with exponential backoff
   - Handle rate limiting (429 status)

3. **Performance Optimization**
   - Load adapters once and reuse
   - Check adapter status before querying
   - Use pagination for list operations

4. **Monitoring**
   - Track inference latency
   - Monitor cache hit rates
   - Set up health check alerts

5. **Security**
   - Use HTTPS in production
   - Validate all user inputs
   - Implement request signing for critical operations
   - Enable audit logging

---

## API Documentation

For complete API documentation, see:
- **[API Reference](API_REFERENCE.md)** - Complete endpoint reference with examples
- **[OpenAPI Specification](openapi/lora_api.yaml)** - Machine-readable API spec

---

Last updated: 2026-01-11
