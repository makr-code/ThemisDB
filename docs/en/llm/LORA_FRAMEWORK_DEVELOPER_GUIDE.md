# LoRA Framework Developer Guide

**Version:** 1.0  
**Date:** 2026-01-11  
**Status:** Complete

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Core Components](#core-components)
4. [API Reference](#api-reference)
5. [Integration Guide](#integration-guide)
6. [Best Practices](#best-practices)
7. [Advanced Topics](#advanced-topics)
8. [Troubleshooting](#troubleshooting)

---

## Overview

### What is the LoRA Framework?

The ThemisDB LoRA Framework provides a complete infrastructure for training, managing, and deploying Low-Rank Adaptation (LoRA) adapters for Large Language Models. It enables domain-specific fine-tuning without modifying base model weights.

### Key Features

- **Complete CRUD Operations**: Create, Read, Update, Delete adapters
- **Multi-Storage Backend**: Filesystem, ThemisDB, S3-compatible storage
- **Training Pipeline**: Full training workflow with feedback integration
- **Version Management**: Semantic versioning and rollback support
- **Security First**: Encryption, signatures, audit logging by default
- **BaseEntity Compliance**: Unified storage pattern across ThemisDB
- **Graph Integration**: Track adapter lineage and relationships
- **Vector Search**: Semantic similarity for adapter discovery

### Architecture Principles

1. **Unified Storage Pattern**: LoRA adapters follow the same BaseEntity pattern as all ThemisDB entities
2. **Separation of Concerns**: Clear separation between lifecycle, storage, and training
3. **Extensibility**: Plugin architecture for custom storage and training backends
4. **Enterprise-Ready**: Built-in security, audit logging, and RAID support

---

## Architecture

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    LoRA Framework                            │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌─────────────────────────────────────────────────────┐   │
│  │          LoRA Orchestrator (Coordinator)             │   │
│  │  • Unified CRUD interface                            │   │
│  │  • Workflow management                               │   │
│  │  • Job scheduling                                     │   │
│  └─────────────────────────────────────────────────────┘   │
│           │                │                │                │
│           ▼                ▼                ▼                │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │   Adapter    │  │   Storage    │  │   Training   │     │
│  │   Manager    │  │   Service    │  │   Service    │     │
│  │              │  │              │  │              │     │
│  │ • Lifecycle  │  │ • BaseEntity │  │ • Pipeline   │     │
│  │ • Loading    │  │ • Filesystem │  │ • Feedback   │     │
│  │ • Caching    │  │ • ThemisDB   │  │ • Metrics    │     │
│  │ • Metrics    │  │ • S3 compat  │  │ • Versioning │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
│           │                │                │                │
│           └────────────────┴────────────────┘                │
│                            │                                  │
│                            ▼                                  │
│  ┌─────────────────────────────────────────────────────┐   │
│  │          Multi-LoRA Manager (Advanced)              │   │
│  │  • Multi-GPU support                                │   │
│  │  • Quantization                                      │   │
│  │  • Batch inference                                   │   │
│  │  • Adapter fusion                                    │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                               │
│  ┌─────────────────────────────────────────────────────┐   │
│  │              Supporting Services                     │   │
│  │  • Audit Logger  • Security Validator               │   │
│  │  • Metadata Cache  • Graph Service                  │   │
│  │  • Vector Service  • Blob Manager                   │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### Component Responsibilities

#### LoRA Orchestrator
- **Purpose**: Central coordinator for all LoRA operations
- **Responsibilities**: 
  - Unified CRUD interface
  - Workflow and job management
  - Component coordination
  - Event notifications

#### Adapter Manager
- **Purpose**: Manage adapter lifecycle
- **Responsibilities**:
  - Load/unload adapters
  - Adapter caching (LRU)
  - Reference counting
  - Performance metrics

#### Storage Service
- **Purpose**: Persist adapter data
- **Responsibilities**:
  - Store/retrieve adapter weights and metadata
  - Support multiple backends (filesystem, ThemisDB, S3)
  - Version management
  - Backup and recovery

#### Training Service
- **Purpose**: Train and fine-tune adapters
- **Responsibilities**:
  - Training pipeline execution
  - Feedback integration
  - Hyperparameter management
  - Quality metrics tracking

---

## Core Components

### 1. LoRA Orchestrator

The central coordinator providing a unified interface for all LoRA operations.

#### Header: `lora_orchestrator.h`

```cpp
namespace themis::llm::lora {

class LoRAOrchestrator {
public:
    struct Config {
        std::shared_ptr<DatabaseWrapper> db;
        std::shared_ptr<BlobManager> blob_manager;
        bool enable_encryption = true;
        bool enable_signatures = true;
        std::string storage_backend = "themisdb"; // or "filesystem", "s3"
    };
    
    explicit LoRAOrchestrator(const Config& config);
    
    // CREATE operations
    std::string createAdapter(
        const std::string& adapter_id,
        const std::string& base_model,
        const TrainingData& data,
        const LoRAHyperparameters& params
    );
    
    // READ operations
    std::optional<AdapterInfo> getAdapter(const std::string& adapter_id);
    std::vector<std::string> listAdapters(const std::string& base_model = "");
    bool isAdapterLoaded(const std::string& adapter_id);
    
    // UPDATE operations
    bool updateAdapter(
        const std::string& adapter_id,
        const TrainingData& additional_data
    );
    bool reloadAdapter(const std::string& adapter_id);
    
    // DELETE operations
    bool deleteAdapter(const std::string& adapter_id);
    bool deleteVersion(const std::string& adapter_id, const std::string& version);
    
    // Job management
    JobInfo getJobStatus(const std::string& job_id);
    std::vector<JobInfo> listJobs(JobStatus status = JobStatus::All);
    bool cancelJob(const std::string& job_id);
};

} // namespace themis::llm::lora
```

#### Usage Example

```cpp
#include "llm/lora_framework/lora_orchestrator.h"

// Initialize orchestrator
LoRAOrchestrator::Config config;
config.db = db_wrapper;
config.blob_manager = blob_manager;
config.enable_encryption = true;

auto orchestrator = std::make_shared<LoRAOrchestrator>(config);

// Create a new adapter
TrainingData data;
data.base_model = "llama-2-7b";
data.samples = loadTrainingData("docs_qa_dataset.json");

LoRAHyperparameters params;
params.rank = 8;
params.alpha = 16;
params.learning_rate = 0.0003;

std::string job_id = orchestrator->createAdapter(
    "documentation_assistant",
    "llama-2-7b",
    data,
    params
);

// Check job status
auto job = orchestrator->getJobStatus(job_id);
if (job.status == JobStatus::Completed) {
    std::cout << "Training completed successfully!\n";
}
```

### 2. Adapter Manager

Manages adapter lifecycle, hot-swapping, and multi-GPU placement.

> **Note:** `LoRAAdapterManager` was removed in v1.5.0. Use `MultiLoRAManager`
> for all new code. See `docs/llm/LORA_ADAPTER_MIGRATION.md` for the full
> migration guide.

#### Header: `llm/multi_lora_manager.h`

```cpp
namespace themis::llm {

class MultiLoRAManager {
public:
    struct Config {
        size_t max_loaded_adapters = 10;
        bool enable_hot_swap       = true;
        bool enable_metrics        = true;
    };

    explicit MultiLoRAManager(const Config& config,
                              std::shared_ptr<LlamaWrapper> wrapper);

    // Load adapter into memory (thread-safe)
    bool loadAdapter(const std::string& adapter_id,
                     const std::string& adapter_path);

    // Unload adapter from memory
    bool unloadAdapter(const std::string& adapter_id);

    // Apply adapter to an inference context
    bool applyAdapter(llama_context* ctx,
                      const std::string& adapter_id,
                      float scale = 1.0f);

    // List all currently loaded adapter IDs
    std::vector<std::string> listAdapters() const;

    // Check GPU health for a device index
    bool isGPUHealthy(int gpu_id) const;
};

} // namespace themis::llm
```

#### Usage Example

```cpp
#include "llm/multi_lora_manager.h"

MultiLoRAManager::Config config;
config.max_loaded_adapters = 20;
config.enable_hot_swap     = true;

auto manager = std::make_shared<MultiLoRAManager>(config, wrapper);

// Load adapter
bool loaded = manager->loadAdapter("documentation_assistant",
                                   "/models/lora/doc_assistant.gguf");
if (loaded) {
    // Apply to an active inference context
    manager->applyAdapter(ctx, "documentation_assistant", /*scale=*/1.0f);
}

// Enumerate loaded adapters
for (const auto& id : manager->listAdapters()) {
    std::cout << "Loaded: " << id << "\n";
}
```

### 3. Storage Service

Handles persistence of adapter data.

#### Header: `lora_storage_service.h`

```cpp
namespace themis::llm::lora {

class LoRAStorageService {
public:
    enum class Backend {
        Filesystem,
        ThemisDB,
        S3
    };
    
    struct Config {
        Backend backend = Backend::ThemisDB;
        std::string base_path = "/var/lib/themisdb/lora";
        std::shared_ptr<DatabaseWrapper> db;
        std::shared_ptr<BlobManager> blob_manager;
        bool enable_encryption = true;
        bool enable_compression = true;
    };
    
    explicit LoRAStorageService(const Config& config);
    
    // Store adapter
    bool storeAdapter(
        const std::string& adapter_id,
        const AdapterMetadata& metadata,
        const std::vector<uint8_t>& weights
    );
    
    // Load adapter
    std::optional<std::pair<AdapterMetadata, std::vector<uint8_t>>>
    loadAdapter(const std::string& adapter_id);
    
    // Delete adapter
    bool deleteAdapter(const std::string& adapter_id);
    
    // List adapters
    std::vector<std::string> listAdapters();
    
    // Version management
    bool storeVersion(
        const std::string& adapter_id,
        const std::string& version,
        const std::vector<uint8_t>& weights
    );
    std::vector<std::string> listVersions(const std::string& adapter_id);
    bool deleteVersion(const std::string& adapter_id, const std::string& version);
};

} // namespace themis::llm::lora
```

#### Usage Example

```cpp
#include "llm/lora_framework/lora_storage_service.h"

LoRAStorageService::Config config;
config.backend = LoRAStorageService::Backend::ThemisDB;
config.db = db_wrapper;
config.blob_manager = blob_manager;
config.enable_encryption = true;

auto storage = std::make_shared<LoRAStorageService>(config);

// Store adapter
AdapterMetadata metadata;
metadata.adapter_id = "my_adapter";
metadata.base_model = "llama-2-7b";
metadata.version = "v1.0";
metadata.created_at = std::chrono::system_clock::now();

std::vector<uint8_t> weights = loadWeights("adapter_weights.bin");

bool stored = storage->storeAdapter("my_adapter", metadata, weights);

// Load adapter
auto result = storage->loadAdapter("my_adapter");
if (result) {
    auto [metadata, weights] = *result;
    // Use adapter
}
```

### 4. Training Service

Executes the training pipeline.

#### Header: `lora_training_service.h`

```cpp
namespace themis::llm::lora {

class LoRATrainingService {
public:
    struct Config {
        std::string training_backend = "llamacpp";  // or "pytorch", "custom"
        size_t max_concurrent_jobs = 4;
        bool enable_checkpoints = true;
        std::string checkpoint_dir = "/var/lib/themisdb/checkpoints";
    };
    
    explicit LoRATrainingService(const Config& config);
    
    // Train new adapter
    TrainingResult train(
        const std::string& adapter_id,
        const std::string& base_model,
        const TrainingData& data,
        const LoRAHyperparameters& params
    );
    
    // Continue training (incremental)
    TrainingResult continueTraining(
        const std::string& adapter_id,
        const TrainingData& additional_data,
        const LoRAHyperparameters& params
    );
    
    // Get training metrics
    TrainingMetrics getMetrics(const std::string& job_id);
    
    // Stop training
    bool stopTraining(const std::string& job_id);
};

struct TrainingResult {
    bool success;
    std::string adapter_id;
    std::string version;
    float final_loss;
    float validation_accuracy;
    std::chrono::milliseconds duration;
    std::string error_message;
};

} // namespace themis::llm::lora
```

#### Usage Example

```cpp
#include "llm/lora_framework/lora_training_service.h"

LoRATrainingService::Config config;
config.max_concurrent_jobs = 2;
config.enable_checkpoints = true;

auto training = std::make_shared<LoRATrainingService>(config);

// Prepare training data
TrainingData data;
data.base_model = "llama-2-7b";
data.samples = {
    {"What is ThemisDB?", "ThemisDB is a distributed database..."},
    {"How to enable sharding?", "To enable sharding..."}
};

// Set hyperparameters
LoRAHyperparameters params;
params.rank = 8;
params.alpha = 16;
params.learning_rate = 0.0003;
params.epochs = 3;
params.batch_size = 32;

// Train
auto result = training->train("doc_assistant", "llama-2-7b", data, params);

if (result.success) {
    std::cout << "Training successful!\n";
    std::cout << "Final loss: " << result.final_loss << "\n";
    std::cout << "Accuracy: " << result.validation_accuracy << "\n";
}
```

---

## API Reference

### Data Structures

#### AdapterMetadata

```cpp
struct AdapterMetadata {
    std::string adapter_id;
    std::string base_model;
    std::string version;
    std::string description;
    
    // Training info
    int training_samples;
    LoRAHyperparameters hyperparameters;
    
    // Quality metrics
    float validation_loss;
    float validation_accuracy;
    
    // Timestamps
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
    
    // Security
    std::string checksum;  // SHA-256
    std::string signature;  // Ed25519
    
    // Metadata
    std::map<std::string, std::string> tags;
    nlohmann::json custom_metadata;
};
```

#### LoRAHyperparameters

```cpp
struct LoRAHyperparameters {
    int rank = 8;                      // LoRA rank
    int alpha = 16;                    // LoRA alpha
    float learning_rate = 0.0003f;     // Learning rate
    int epochs = 3;                    // Training epochs
    int batch_size = 32;               // Batch size
    float dropout = 0.1f;              // Dropout rate
    std::string optimizer = "adamw";   // Optimizer
    float weight_decay = 0.01f;        // Weight decay
    bool use_gradient_checkpointing = false;
};
```

#### TrainingData

```cpp
struct TrainingData {
    std::string base_model;
    std::string task_type;  // e.g., "qa", "summarization", "translation"
    
    struct Sample {
        std::string input;
        std::string output;
        std::map<std::string, std::string> metadata;
    };
    
    std::vector<Sample> samples;
    
    // Optional validation split
    std::optional<std::vector<Sample>> validation_samples;
};
```

### Error Handling

The framework uses exceptions for error handling:

```cpp
namespace themis::llm::lora {

class LoRAException : public std::runtime_error {
public:
    explicit LoRAException(const std::string& message);
};

class AdapterNotFoundException : public LoRAException {
public:
    explicit AdapterNotFoundException(const std::string& adapter_id);
};

class TrainingFailedException : public LoRAException {
public:
    explicit TrainingFailedException(const std::string& reason);
};

class StorageException : public LoRAException {
public:
    explicit StorageException(const std::string& reason);
};

} // namespace themis::llm::lora
```

Usage:

```cpp
try {
    auto adapter = orchestrator->getAdapter("my_adapter");
    if (!adapter) {
        throw AdapterNotFoundException("my_adapter");
    }
    // Use adapter
} catch (const AdapterNotFoundException& e) {
    std::cerr << "Adapter not found: " << e.what() << "\n";
} catch (const LoRAException& e) {
    std::cerr << "LoRA error: " << e.what() << "\n";
}
```

---

## Integration Guide

### Step-by-Step Integration

#### 1. Add Dependencies

In your `CMakeLists.txt`:

```cmake
# Link LoRA framework
target_link_libraries(your_target
    PRIVATE
        themis::lora_framework
        themis::llm_core
)
```

#### 2. Initialize Components

```cpp
#include "llm/lora_framework/lora_orchestrator.h"

class YourApplication {
private:
    std::shared_ptr<LoRAOrchestrator> lora_orchestrator_;
    
public:
    void initialize() {
        // Setup orchestrator
        LoRAOrchestrator::Config config;
        config.db = database_wrapper_;
        config.blob_manager = blob_manager_;
        config.enable_encryption = true;
        
        lora_orchestrator_ = std::make_shared<LoRAOrchestrator>(config);
    }
};
```

#### 3. Implement Training Workflow

```cpp
void trainDocumentationAssistant() {
    // Load training data
    TrainingData data;
    data.base_model = "llama-2-7b";
    data.task_type = "documentation_qa";
    data.samples = loadFromDatabase("training_data");
    
    // Configure hyperparameters
    LoRAHyperparameters params;
    params.rank = 8;
    params.alpha = 16;
    params.learning_rate = 0.0003;
    params.epochs = 3;
    
    // Start training (async)
    std::string job_id = lora_orchestrator_->createAdapter(
        "doc_assistant_v1",
        "llama-2-7b",
        data,
        params
    );
    
    // Monitor progress
    while (true) {
        auto job = lora_orchestrator_->getJobStatus(job_id);
        
        if (job.status == JobStatus::Completed) {
            std::cout << "Training completed!\n";
            break;
        } else if (job.status == JobStatus::Failed) {
            std::cerr << "Training failed: " << job.error_message << "\n";
            break;
        }
        
        std::cout << "Progress: " << (job.progress * 100) << "%\n";
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}
```

#### 4. Implement Inference

```cpp
std::string queryAdapter(const std::string& question) {
    // Ensure adapter is loaded
    if (!lora_orchestrator_->isAdapterLoaded("doc_assistant_v1")) {
        lora_orchestrator_->loadAdapter("doc_assistant_v1");
    }
    
    // Get adapter info
    auto adapter = lora_orchestrator_->getAdapter("doc_assistant_v1");
    if (!adapter) {
        throw std::runtime_error("Adapter not found");
    }
    
    // TODO: Integrate with LLM inference engine
    // This would typically call into llama.cpp or similar
    std::string response = llm_engine_->generate(
        question,
        adapter->adapter_id
    );
    
    return response;
}
```

#### 5. Add Feedback Loop

```cpp
void collectFeedback(
    const std::string& question,
    const std::string& answer,
    bool is_positive,
    const std::string& correction = ""
) {
    FeedbackItem feedback;
    feedback.question = question;
    feedback.answer = answer;
    feedback.is_positive = is_positive;
    feedback.correction = correction;
    feedback.timestamp = std::chrono::system_clock::now();
    
    // Store feedback
    feedback_store_->addFeedback("doc_assistant_v1", feedback);
    
    // Trigger retraining if threshold reached
    if (feedback_store_->getFeedbackCount("doc_assistant_v1") >= 100) {
        retrainFromFeedback("doc_assistant_v1");
    }
}
```

### REST API Integration

The framework exposes REST API endpoints:

```bash
# Create adapter
curl -X POST http://localhost:8529/api/v1/llm/lora/create \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "adapter_id": "my_adapter",
    "base_model": "llama-2-7b",
    "training_data": {...},
    "hyperparameters": {...}
  }'

# Get adapter info
curl -X GET http://localhost:8529/api/v1/llm/lora/my_adapter \
  -H "Authorization: Bearer $TOKEN"

# List adapters
curl -X GET http://localhost:8529/api/v1/llm/lora/list \
  -H "Authorization: Bearer $TOKEN"

# Delete adapter
curl -X DELETE http://localhost:8529/api/v1/llm/lora/my_adapter \
  -H "Authorization: Bearer $TOKEN"
```

---

## Best Practices

### 1. Adapter Naming Conventions

```cpp
// Good
"documentation_assistant_v1"
"sql_translator_v2_3"
"code_reviewer_production"

// Bad
"adapter1"
"test"
"my_lora"
```

### 2. Version Management

```cpp
// Semantic versioning
metadata.version = "v1.2.3";
// Major.Minor.Patch

// Version tags
metadata.tags["environment"] = "production";
metadata.tags["base_model_version"] = "2.0";
```

### 3. Training Data Validation

```cpp
bool validateTrainingData(const TrainingData& data) {
    // Minimum samples
    if (data.samples.size() < 100) {
        spdlog::warn("Training data has fewer than 100 samples");
        return false;
    }
    
    // Check for empty samples
    for (const auto& sample : data.samples) {
        if (sample.input.empty() || sample.output.empty()) {
            spdlog::error("Found empty sample");
            return false;
        }
    }
    
    // Check for duplicates
    std::set<std::string> seen;
    for (const auto& sample : data.samples) {
        if (seen.count(sample.input)) {
            spdlog::warn("Duplicate input found: {}", sample.input);
        }
        seen.insert(sample.input);
    }
    
    return true;
}
```

### 4. Resource Management

```cpp
// Use RAII for adapter lifecycle
class ScopedAdapter {
public:
    ScopedAdapter(
        std::shared_ptr<LoRAOrchestrator> orchestrator,
        const std::string& adapter_id
    ) : orchestrator_(orchestrator), adapter_id_(adapter_id) {
        orchestrator_->loadAdapter(adapter_id_);
    }
    
    ~ScopedAdapter() {
        orchestrator_->unloadAdapter(adapter_id_);
    }
    
private:
    std::shared_ptr<LoRAOrchestrator> orchestrator_;
    std::string adapter_id_;
};

// Usage
{
    ScopedAdapter adapter(orchestrator, "my_adapter");
    // Use adapter
} // Automatically unloaded
```

### 5. Error Handling and Logging

```cpp
void trainWithErrorHandling(
    const std::string& adapter_id,
    const TrainingData& data
) {
    try {
        // Validate data first
        if (!validateTrainingData(data)) {
            throw std::invalid_argument("Invalid training data");
        }
        
        // Start training
        spdlog::info("Starting training for adapter: {}", adapter_id);
        auto job_id = orchestrator_->createAdapter(
            adapter_id, "llama-2-7b", data, params
        );
        
        // Log job ID
        spdlog::info("Training job started: {}", job_id);
        
    } catch (const std::exception& e) {
        spdlog::error("Training failed: {}", e.what());
        // Send notification, rollback, etc.
        throw;
    }
}
```

---

## Advanced Topics

### Multi-Model Support

```cpp
// Train adapters for different base models
for (const auto& model : {"llama-2-7b", "mistral-7b", "llama-2-13b"}) {
    std::string adapter_id = fmt::format("doc_assistant_{}", model);
    orchestrator_->createAdapter(adapter_id, model, data, params);
}
```

### Adapter Fusion

```cpp
// Combine multiple adapters
std::vector<std::string> adapters = {
    "documentation_assistant",
    "code_reviewer",
    "sql_translator"
};

auto fused_adapter = multi_lora_manager_->fuseAdapters(
    adapters,
    "multi_task_assistant"
);
```

### A/B Testing

```cpp
// Compare adapter versions
std::vector<std::string> versions = {"v1.0", "v1.1", "v2.0"};
std::vector<std::string> test_queries = loadTestQueries();

for (const auto& version : versions) {
    float accuracy = evaluateAdapter(
        fmt::format("doc_assistant_{}", version),
        test_queries
    );
    spdlog::info("Version {} accuracy: {}", version, accuracy);
}
```

### Distributed Training

```cpp
// Train on multiple GPUs
LoRAHyperparameters params;
params.distributed = true;
params.num_gpus = 4;
params.strategy = "data_parallel";

orchestrator_->createAdapter("large_adapter", "llama-2-70b", data, params);
```

---

## Troubleshooting

### Common Issues

#### 1. Adapter Not Found

**Error**: `AdapterNotFoundException`

**Solution**:
```cpp
// Verify adapter exists
auto adapters = orchestrator_->listAdapters();
if (std::find(adapters.begin(), adapters.end(), adapter_id) == adapters.end()) {
    std::cerr << "Adapter does not exist. Available: ";
    for (const auto& a : adapters) std::cerr << a << " ";
    std::cerr << "\n";
}
```

#### 2. Training Fails

**Error**: `TrainingFailedException`

**Solution**:
```cpp
// Check training logs
auto job = orchestrator_->getJobStatus(job_id);
spdlog::error("Training error: {}", job.error_message);

// Common causes:
// - Insufficient training data
// - Invalid hyperparameters
// - Out of memory
// - Model compatibility issues
```

#### 3. Out of Memory

**Error**: CUDA out of memory

**Solution**:
```cpp
// Reduce batch size
params.batch_size = 16;  // or lower

// Enable gradient checkpointing
params.use_gradient_checkpointing = true;

// Use quantization
params.quantization = "int8";
```

#### 4. Slow Inference

**Solution**:
```cpp
// Enable caching
manager_config.max_cache_size = 20;

// Pin frequently used adapters
multi_lora_manager_->pinAdapter("doc_assistant");

// Use quantized adapters
params.quantization = "int4";
```

---

## Next Steps

- Read the [themis_help_lora User Guide](THEMIS_HELP_LORA_USER_GUIDE.md)
- Explore [Training Guide](LORA_TRAINING_GUIDE.md)
- See [Integration Examples](LORA_INTEGRATION_EXAMPLES.md)
- Review [API Reference](../../../LORA_AQL_REFERENCE.md)

---

**Last Updated**: 2026-04-06  
**Version**: 1.0
