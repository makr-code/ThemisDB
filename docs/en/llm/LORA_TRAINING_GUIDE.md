# LoRA Training Guide

**Version:** 1.0  
**Date:** 2026-01-11  
**Status:** Complete

---

## Table of Contents

1. [Introduction](#introduction)
2. [Training Concepts](#training-concepts)
3. [Preparing Training Data](#preparing-training-data)
4. [Training from Documentation](#training-from-documentation)
5. [Training from Feedback](#training-from-feedback)
6. [Hyperparameter Tuning](#hyperparameter-tuning)
7. [Monitoring Training](#monitoring-training)
8. [Best Practices](#best-practices)
9. [Advanced Topics](#advanced-topics)
10. [Troubleshooting](#troubleshooting)

---

## Introduction

### What is LoRA Training?

LoRA (Low-Rank Adaptation) training is a technique for fine-tuning Large Language Models without modifying the base model weights. Instead, it trains small "adapter" layers that modify the model's behavior for specific tasks.

### Benefits of LoRA

✅ **Efficient**: Much faster than full fine-tuning  
✅ **Memory-Friendly**: Requires less GPU memory  
✅ **Reversible**: Can switch between adapters easily  
✅ **Portable**: Adapters are small (typically 10-100 MB)  
✅ **Safe**: Base model remains unchanged

### When to Train

Train a LoRA adapter when you need to:
- Specialize an LLM for a specific domain (e.g., ThemisDB documentation)
- Adapt to custom terminology or writing style
- Learn from user corrections and feedback
- Reduce hallucinations on domain-specific topics

---

## Training Concepts

### Training Workflow

```
┌─────────────────────────────────────────────────────────┐
│                   Training Workflow                      │
├─────────────────────────────────────────────────────────┤
│                                                           │
│  1. Prepare Data                                         │
│     ↓                                                     │
│  2. Configure Hyperparameters                            │
│     ↓                                                     │
│  3. Train Adapter                                        │
│     ↓                                                     │
│  4. Validate Quality                                     │
│     ↓                                                     │
│  5. Deploy to Production                                 │
│     ↓                                                     │
│  6. Collect Feedback                                     │
│     ↓                                                     │
│  7. Retrain (Incremental)                               │
│     ↓                                                     │
│  8. Version & Rollback (if needed)                      │
│                                                           │
└─────────────────────────────────────────────────────────┘
```

### Key Terms

- **Base Model**: The foundation LLM (e.g., Llama-2-7B)
- **Adapter**: Small weights trained via LoRA
- **Rank (r)**: Size of the adapter (higher = more capacity, slower)
- **Alpha (α)**: Scaling factor for adapter influence
- **Epoch**: One complete pass through training data
- **Batch Size**: Number of samples processed together
- **Learning Rate**: How fast the model learns

---

## Preparing Training Data

### Data Format

Training data consists of input-output pairs:

```json
{
  "samples": [
    {
      "input": "How do I enable sharding in ThemisDB?",
      "output": "To enable sharding: CREATE COLLECTION mydata SHARD BY user_id SHARDS 8;",
      "metadata": {
        "source": "official_docs",
        "category": "sharding"
      }
    },
    {
      "input": "What is replication in ThemisDB?",
      "output": "Replication creates copies of data across nodes for fault tolerance...",
      "metadata": {
        "source": "official_docs",
        "category": "replication"
      }
    }
  ]
}
```

### Data Quality Guidelines

#### Quantity

| Data Size | Quality | Expected Results |
|-----------|---------|------------------|
| < 100 samples | Minimum | Basic understanding |
| 100-500 samples | Good | Decent performance |
| 500-1000 samples | Better | Good performance |
| 1000+ samples | Best | Excellent performance |

#### Quality Checklist

✅ **Correctness**: All outputs must be accurate  
✅ **Consistency**: Similar questions get similar answers  
✅ **Diversity**: Cover various topics and question styles  
✅ **Clarity**: Outputs should be clear and well-formatted  
✅ **Relevance**: Stay focused on your domain

### Creating Training Data

#### From Documentation

```cpp
// Example: Extract Q&A pairs from documentation
std::vector<TrainingSample> extractFromDocs(const std::string& docs_path) {
    std::vector<TrainingSample> samples;
    
    // Parse documentation
    auto doc_sections = parseMarkdown(docs_path);
    
    for (const auto& section : doc_sections) {
        // Generate question from section title
        std::string question = generateQuestion(section.title);
        
        // Extract relevant content as answer
        std::string answer = extractAnswer(section.content);
        
        samples.push_back({
            .input = question,
            .output = answer,
            .metadata = {{"source", "docs"}, {"section", section.title}}
        });
    }
    
    return samples;
}
```

#### From User Queries

```cpp
// Example: Extract from query logs
std::vector<TrainingSample> extractFromLogs(const std::string& log_path) {
    std::vector<TrainingSample> samples;
    
    auto logs = parseQueryLogs(log_path);
    
    for (const auto& log : logs) {
        // Only use queries marked as successful
        if (log.feedback == "positive" && log.confidence > 0.8) {
            samples.push_back({
                .input = log.question,
                .output = log.answer,
                .metadata = {{"source", "logs"}, {"confidence", log.confidence}}
            });
        }
    }
    
    return samples;
}
```

#### Manual Curation

```json
// Example: Manually curated Q&A
{
  "samples": [
    {
      "input": "How do I create a graph in ThemisDB?",
      "output": "To create a graph:\n1. CREATE GRAPH mygraph\n2. EDGE COLLECTIONS relationships\n3. FROM COLLECTIONS users TO COLLECTIONS users\n\nExample: CREATE GRAPH social_network EDGE COLLECTIONS friendships FROM COLLECTIONS users TO COLLECTIONS users;",
      "metadata": {
        "curator": "admin",
        "verified": true
      }
    }
  ]
}
```

### Data Validation

```cpp
bool validateTrainingData(const TrainingData& data) {
    // Check minimum samples
    if (data.samples.size() < 100) {
        spdlog::warn("Less than 100 samples - consider adding more");
        return false;
    }
    
    // Check for empty samples
    for (const auto& sample : data.samples) {
        if (sample.input.empty() || sample.output.empty()) {
            spdlog::error("Empty input or output found");
            return false;
        }
        
        // Check length constraints
        if (sample.input.length() > 1000) {
            spdlog::warn("Input too long: {}", sample.input.length());
        }
        if (sample.output.length() > 2000) {
            spdlog::warn("Output too long: {}", sample.output.length());
        }
    }
    
    // Check for duplicates
    std::set<std::string> unique_inputs;
    for (const auto& sample : data.samples) {
        if (unique_inputs.count(sample.input)) {
            spdlog::warn("Duplicate input: {}", sample.input);
        }
        unique_inputs.insert(sample.input);
    }
    
    return true;
}
```

---

## Training from Documentation

### Step-by-Step Process

#### 1. Prepare Documentation Corpus

```cpp
// Load all documentation files
std::vector<std::string> doc_files = {
    "docs/sharding_guide.md",
    "docs/replication_guide.md",
    "docs/aql_reference.md",
    "docs/api_reference.md"
};

TrainingData data;
data.base_model = "llama-2-7b";
data.task_type = "documentation_qa";

for (const auto& file : doc_files) {
    auto samples = extractQAPairsFromDoc(file);
    data.samples.insert(data.samples.end(), samples.begin(), samples.end());
}

spdlog::info("Prepared {} training samples from documentation", data.samples.size());
```

#### 2. Configure Training

```cpp
LoRAHyperparameters params;
params.rank = 8;              // Start with default
params.alpha = 16;            // Typical: 2 * rank
params.learning_rate = 0.0003; // Conservative for documentation
params.epochs = 3;            // Start with 3, increase if needed
params.batch_size = 32;       // Adjust based on GPU memory
params.dropout = 0.1;         // Prevent overfitting
```

#### 3. Start Training

```cpp
auto orchestrator = std::make_shared<LoRAOrchestrator>(config);

std::string job_id = orchestrator->createAdapter(
    "documentation_assistant_v1",
    "llama-2-7b",
    data,
    params
);

spdlog::info("Training job started: {}", job_id);
```

#### 4. Monitor Progress

```cpp
while (true) {
    auto job = orchestrator->getJobStatus(job_id);
    
    std::cout << "Progress: " << (job.progress * 100) << "%\n";
    std::cout << "Status: " << jobStatusToString(job.status) << "\n";
    
    if (job.status == JobStatus::Completed) {
        auto metrics = getTrainingMetrics(job_id);
        std::cout << "Final loss: " << metrics.final_loss << "\n";
        std::cout << "Validation accuracy: " << metrics.validation_accuracy << "\n";
        break;
    }
    
    if (job.status == JobStatus::Failed) {
        std::cerr << "Training failed: " << job.error_message << "\n";
        break;
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(10));
}
```

### Example: Training themis_help_lora

```cpp
void trainThemisHelpLoRA() {
    // 1. Load ThemisDB documentation
    DocumentationLoader loader;
    loader.addDirectory("docs/en/");
    loader.addDirectory("docs/guides/");
    
    auto qa_pairs = loader.extractQAPairs();
    spdlog::info("Extracted {} Q&A pairs", qa_pairs.size());
    
    // 2. Create training data
    TrainingData data;
    data.base_model = "llama-2-7b";
    data.task_type = "documentation_qa";
    data.samples = qa_pairs;
    
    // 3. Split validation set (10%)
    auto [train_data, val_data] = splitData(data, 0.9);
    train_data.validation_samples = val_data.samples;
    
    // 4. Configure hyperparameters
    LoRAHyperparameters params;
    params.rank = 8;
    params.alpha = 16;
    params.learning_rate = 0.0003;
    params.epochs = 5;  // More epochs for documentation
    params.batch_size = 32;
    
    // 5. Train
    auto helper = std::make_shared<ThemisHelpLoRA>(config);
    bool success = helper->trainFromDocumentation();
    
    if (success) {
        spdlog::info("themis_help_lora training completed!");
        
        // 6. Test with sample questions
        testAdapter(helper, {
            "How do I enable sharding?",
            "What is replication?",
            "How to create a graph?"
        });
    }
}
```

---

## Training from Feedback

### Feedback Loop

```
User Query → Answer → Feedback → Store → Accumulate → Retrain
```

### Collecting Feedback

```cpp
class FeedbackCollector {
public:
    void addFeedback(
        const std::string& adapter_id,
        const std::string& question,
        const std::string& answer,
        bool is_positive,
        const std::string& correction = ""
    ) {
        FeedbackItem item;
        item.adapter_id = adapter_id;
        item.question = question;
        item.answer = answer;
        item.is_positive = is_positive;
        item.correction = correction;
        item.timestamp = std::chrono::system_clock::now();
        
        feedback_buffer_.push_back(item);
        
        // Auto-retrain if threshold reached
        if (feedback_buffer_.size() >= config_.feedback_threshold) {
            triggerRetraining(adapter_id);
        }
    }
    
private:
    std::vector<FeedbackItem> feedback_buffer_;
    Config config_;
};
```

### Incremental Training

```cpp
void retrainFromFeedback(const std::string& adapter_id) {
    // 1. Load feedback
    auto feedback = feedback_store_->getFeedback(adapter_id, /*used=*/false);
    spdlog::info("Retraining with {} feedback items", feedback.size());
    
    // 2. Convert to training data
    TrainingData data;
    data.base_model = "llama-2-7b";
    
    for (const auto& item : feedback) {
        if (!item.is_positive && !item.correction.empty()) {
            // Use corrected answer
            data.samples.push_back({
                .input = item.question,
                .output = item.correction,
                .metadata = {{"source", "feedback"}}
            });
        } else if (item.is_positive) {
            // Reinforce good answer
            data.samples.push_back({
                .input = item.question,
                .output = item.answer,
                .metadata = {{"source", "feedback"}, {"reinforced", "true"}}
            });
        }
    }
    
    // 3. Retrain (incremental)
    LoRAHyperparameters params;
    params.rank = 8;
    params.alpha = 16;
    params.learning_rate = 0.0001; // Lower for incremental
    params.epochs = 2;             // Fewer epochs
    
    bool success = orchestrator_->updateAdapter(adapter_id, data);
    
    if (success) {
        // 4. Mark feedback as used
        feedback_store_->markAsUsed(feedback);
        spdlog::info("Retraining completed successfully");
    }
}
```

### Feedback Quality

#### Good Corrections

✅ **Specific and Accurate**
```json
{
  "question": "How do I enable sharding?",
  "wrong_answer": "Use PARTITION BY clause",
  "correction": "Use CREATE COLLECTION mydata SHARD BY user_id SHARDS 8;"
}
```

✅ **Complete Information**
```json
{
  "question": "What is replication?",
  "wrong_answer": "Making copies",
  "correction": "Replication creates multiple copies of data across nodes for fault tolerance. Configure with REPLICATION factor."
}
```

#### Less Useful Corrections

❌ **Too Vague**
```json
{
  "correction": "This is wrong"
}
```

❌ **Subjective**
```json
{
  "correction": "I don't like this answer"
}
```

---

## Hyperparameter Tuning

### Understanding Hyperparameters

#### Rank (r)

- **What it is**: Size of the adapter matrices
- **Default**: 8
- **Impact**: 
  - Higher = More capacity, better accuracy, slower inference
  - Lower = Less capacity, faster, might underfit

| Rank | Use Case | Memory | Speed |
|------|----------|--------|-------|
| 4 | Simple tasks | Low | Fast |
| 8 | **Recommended default** | Medium | Medium |
| 16 | Complex tasks | High | Slower |
| 32+ | Very complex | Very high | Slow |

```cpp
// For simple Q&A
params.rank = 4;

// For documentation (recommended)
params.rank = 8;

// For complex reasoning
params.rank = 16;
```

#### Alpha (α)

- **What it is**: Scaling factor for adapter influence
- **Default**: 16 (typically 2 * rank)
- **Impact**: How much the adapter affects the base model

```cpp
// Conservative (subtle changes)
params.alpha = params.rank;

// Recommended (balanced)
params.alpha = 2 * params.rank;

// Aggressive (strong adaptation)
params.alpha = 4 * params.rank;
```

#### Learning Rate

- **What it is**: How fast the model learns
- **Default**: 0.0003
- **Impact**: Too high = unstable, too low = slow convergence

| Learning Rate | Use Case |
|---------------|----------|
| 0.0001 | Incremental training, fine-tuning |
| **0.0003** | **Recommended default** |
| 0.001 | Fast initial training |
| 0.003+ | Experimental, risky |

```cpp
// Initial training
params.learning_rate = 0.0003;

// Incremental training from feedback
params.learning_rate = 0.0001;
```

#### Epochs

- **What it is**: Number of complete passes through data
- **Default**: 3
- **Impact**: Too few = underfit, too many = overfit

```cpp
// Quick test
params.epochs = 1;

// Recommended
params.epochs = 3;

// More training (large dataset)
params.epochs = 5;

// Be careful with > 10 epochs (overfitting risk)
```

#### Batch Size

- **What it is**: Number of samples processed together
- **Default**: 32
- **Impact**: Larger = faster training, more GPU memory

```cpp
// Limited GPU memory
params.batch_size = 8;

// Recommended (16-24 GB GPU)
params.batch_size = 32;

// Large GPU (40+ GB)
params.batch_size = 64;
```

### Tuning Strategy

#### 1. Start with Defaults

```cpp
LoRAHyperparameters params;
params.rank = 8;
params.alpha = 16;
params.learning_rate = 0.0003;
params.epochs = 3;
params.batch_size = 32;
```

#### 2. Tune One at a Time

```cpp
// Experiment 1: Vary rank
for (int rank : {4, 8, 16}) {
    params.rank = rank;
    params.alpha = 2 * rank;
    auto result = trainAndEvaluate(params);
    results[rank] = result;
}
// Pick best rank

// Experiment 2: Vary learning rate
for (float lr : {0.0001, 0.0003, 0.001}) {
    params.learning_rate = lr;
    auto result = trainAndEvaluate(params);
    results_lr[lr] = result;
}
// Pick best learning rate
```

#### 3. Monitor Metrics

```cpp
void monitorTraining(const std::string& job_id) {
    while (true) {
        auto metrics = getTrainingMetrics(job_id);
        
        // Check for problems
        if (metrics.loss > 10.0) {
            spdlog::warn("Loss too high - reduce learning rate");
        }
        
        if (metrics.loss_not_decreasing) {
            spdlog::warn("Loss plateau - increase learning rate or epochs");
        }
        
        if (metrics.validation_loss > metrics.training_loss * 1.5) {
            spdlog::warn("Overfitting detected - add dropout or reduce epochs");
        }
        
        // Log progress
        spdlog::info("Epoch {}: loss={:.4f}, val_loss={:.4f}", 
                    metrics.epoch, metrics.loss, metrics.validation_loss);
        
        if (metrics.completed) break;
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}
```

---

## Monitoring Training

### Key Metrics

#### Loss

- **Training Loss**: How well model fits training data
- **Validation Loss**: How well model generalizes
- **Goal**: Both should decrease and converge

```cpp
TrainingMetrics metrics = getMetrics(job_id);
std::cout << "Training loss: " << metrics.training_loss << "\n";
std::cout << "Validation loss: " << metrics.validation_loss << "\n";
```

#### Accuracy

- **Validation Accuracy**: Percentage of correct predictions
- **Goal**: > 80% for documentation tasks

```cpp
if (metrics.validation_accuracy < 0.80) {
    spdlog::warn("Accuracy below threshold - consider more training data or higher rank");
}
```

#### Progress

```cpp
std::cout << "Epoch: " << metrics.current_epoch << "/" << metrics.total_epochs << "\n";
std::cout << "Progress: " << (metrics.progress * 100) << "%\n";
std::cout << "ETA: " << metrics.estimated_completion << "\n";
```

### Visualization

```cpp
// Export metrics for visualization
void exportMetrics(const std::string& job_id, const std::string& output_file) {
    auto history = getTrainingHistory(job_id);
    
    nlohmann::json j;
    j["job_id"] = job_id;
    j["metrics"] = nlohmann::json::array();
    
    for (const auto& m : history) {
        j["metrics"].push_back({
            {"epoch", m.epoch},
            {"training_loss", m.training_loss},
            {"validation_loss", m.validation_loss},
            {"learning_rate", m.learning_rate}
        });
    }
    
    std::ofstream out(output_file);
    out << j.dump(2);
}
```

### Grafana Dashboard

ThemisDB includes a Grafana dashboard for LoRA training:

- Training progress
- Loss curves
- GPU utilization
- Memory usage
- Training time estimates

Access at: `http://localhost:3000/d/lora-training`

---

## Best Practices

### 1. Data Preparation

✅ **Use High-Quality Data**
- Verify all training samples for correctness
- Remove duplicates and near-duplicates
- Balance different topics

✅ **Split Validation Set**
```cpp
// Reserve 10-20% for validation
auto [train, val] = splitData(data, 0.9);
train_data.validation_samples = val.samples;
```

✅ **Augment Data**
```cpp
// Generate variations of questions
std::vector<std::string> variations = {
    "How do I enable sharding?",
    "How to enable sharding?",
    "Enable sharding in ThemisDB",
    "Steps to enable sharding"
};
```

### 2. Training Process

✅ **Start Small**
- Use subset of data for initial experiments
- Validate approach before full training

✅ **Monitor Closely**
- Watch for overfitting (val_loss > train_loss)
- Check GPU memory usage
- Monitor training time

✅ **Save Checkpoints**
```cpp
params.enable_checkpoints = true;
params.checkpoint_frequency = 500; // Every 500 steps
```

### 3. Version Management

✅ **Use Semantic Versioning**
```cpp
// Version format: v<major>.<minor>.<patch>
"v1.0.0" // Initial release
"v1.1.0" // New features from feedback
"v1.1.1" // Bug fixes
"v2.0.0" // Major update
```

✅ **Tag Versions**
```cpp
metadata.tags["environment"] = "production";
metadata.tags["training_source"] = "documentation";
metadata.tags["quality"] = "validated";
```

### 4. Testing

✅ **Test Before Deployment**
```cpp
void testAdapter(const std::string& adapter_id) {
    std::vector<std::string> test_questions = {
        "How do I enable sharding?",
        "What is replication?",
        "Configure backup settings"
    };
    
    for (const auto& q : test_questions) {
        auto answer = queryAdapter(adapter_id, q);
        std::cout << "Q: " << q << "\n";
        std::cout << "A: " << answer << "\n";
        std::cout << "---\n";
    }
}
```

### 5. Production Deployment

✅ **A/B Testing**
```cpp
// Deploy gradually
deployStrategy.canary_percentage = 10; // Start with 10% traffic
deployStrategy.rollout_duration = std::chrono::hours(24);
deployStrategy.enable_auto_rollback = true;
```

✅ **Monitor Performance**
```cpp
// Track metrics in production
auto metrics = getProductionMetrics(adapter_id);
if (metrics.success_rate < 0.85) {
    spdlog::warn("Success rate dropped - consider rollback");
}
```

---

## Advanced Topics

### Multi-GPU Training

```cpp
LoRAHyperparameters params;
params.distributed = true;
params.num_gpus = 4;
params.strategy = "data_parallel"; // or "model_parallel"
```

### Mixed Precision Training

```cpp
params.use_fp16 = true; // Faster training, less memory
params.use_gradient_checkpointing = true; // Trade compute for memory
```

### Curriculum Learning

```cpp
// Train on easy examples first, then hard ones
auto sorted_data = sortByCurriculum(data);
for (size_t i = 0; i < sorted_data.size(); i += batch_size) {
    auto batch = getBatch(sorted_data, i, batch_size);
    trainOnBatch(batch);
}
```

### Transfer Learning

```cpp
// Start from another adapter
params.init_from_adapter = "similar_adapter_v1";
auto result = orchestrator->createAdapter("new_adapter", base_model, data, params);
```

---

## Troubleshooting

### Issue: Loss Not Decreasing

**Symptoms**: Training loss stays high or increases

**Solutions**:
```cpp
// Try higher learning rate
params.learning_rate = 0.001;

// Or more epochs
params.epochs = 5;

// Or higher rank
params.rank = 16;
```

### Issue: Overfitting

**Symptoms**: Validation loss >> Training loss

**Solutions**:
```cpp
// Add dropout
params.dropout = 0.2;

// Reduce epochs
params.epochs = 2;

// Add more training data
// or use data augmentation
```

### Issue: Out of Memory

**Symptoms**: CUDA out of memory error

**Solutions**:
```cpp
// Reduce batch size
params.batch_size = 16; // or lower

// Enable gradient checkpointing
params.use_gradient_checkpointing = true;

// Use lower precision
params.use_fp16 = true;

// Reduce rank
params.rank = 4;
```

### Issue: Training Too Slow

**Solutions**:
```cpp
// Increase batch size (if memory allows)
params.batch_size = 64;

// Use mixed precision
params.use_fp16 = true;

// Use multiple GPUs
params.distributed = true;
params.num_gpus = 2;
```

---

## Summary

### Quick Reference

**Default Configuration**:
```cpp
LoRAHyperparameters params;
params.rank = 8;
params.alpha = 16;
params.learning_rate = 0.0003;
params.epochs = 3;
params.batch_size = 32;
params.dropout = 0.1;
```

**Minimum Data**: 100+ samples  
**Recommended Data**: 500-1000 samples  
**Training Time**: 15-60 minutes (depends on data size and GPU)

**Next Steps**:
1. Prepare high-quality training data
2. Start with default hyperparameters
3. Monitor training metrics
4. Test thoroughly before deployment
5. Collect feedback and retrain

---

**Last Updated**: 2026-04-06  
**Version**: 1.0
