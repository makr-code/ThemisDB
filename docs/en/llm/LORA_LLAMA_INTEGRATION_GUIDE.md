# LoRA Training with llama.cpp Base Models - Usage Guide

## Overview

This guide explains how to fine-tune Large Language Models (LLMs) using LoRA (Low-Rank Adaptation) with frozen base models loaded via llama.cpp. This approach enables efficient fine-tuning with minimal memory overhead.

## Table of Contents

1. [Quick Start](#quick-start)
2. [Architecture](#architecture)
3. [Prerequisites](#prerequisites)
4. [Configuration](#configuration)
5. [Data Preparation](#data-preparation)
6. [Training](#training)
7. [Inference](#inference)
8. [Examples](#examples)
9. [Troubleshooting](#troubleshooting)

## Quick Start

### Minimal Example

```cpp
#include "llm/lora_framework/base_model_adapter.h"
#include "llm/lora_framework/data_loader.h"

using namespace themis::llm::lora;

// 1. Setup model configuration
LoRAEnhancedModel::Config config;
config.base_model_path = "models/llama-2-7b.gguf";
config.lora_config.rank = 16;
config.lora_config.alpha = 32.0f;
config.target_modules = {"attention.wq", "attention.wv"};

// 2. Initialize model
LoRAEnhancedModel model(config);
if (!model.initialize()) {
    // Handle error
}

// 3. Load training data
auto tokenizer = std::make_shared<SimpleTokenizer>();
DataLoader loader(tokenizer);
auto samples = data_utils::createToyDataset(100);
loader.loadFromSamples(samples);

// 4. Train (simplified)
while (loader.hasNext()) {
    auto batch = loader.getNextBatch();
    // Forward pass, compute loss, backward pass, optimize
}

// 5. Export adapter
auto weights = model.exportLoRAWeights();
// Save weights...
```

## Architecture

### System Components

```
┌────────────────────────────────────────────────────────┐
│                 LoRA Training Pipeline                  │
├────────────────────────────────────────────────────────┤
│                                                          │
│  1. Base Model Loading (BaseModelAdapter)              │
│     ├─ GGUF file parsing                               │
│     ├─ Architecture detection                          │
│     └─ Layer identification                            │
│                                                          │
│  2. LoRA Injection (LoRAEnhancedModel)                 │
│     ├─ Create LoRA adapters                            │
│     ├─ Inject into target layers                       │
│     └─ Freeze base model weights                       │
│                                                          │
│  3. Data Processing (DataLoader)                       │
│     ├─ Tokenization                                    │
│     ├─ Batch creation                                  │
│     └─ Padding and formatting                          │
│                                                          │
│  4. Training Loop (LoRATrainingService)                │
│     ├─ Forward: base_output + lora_output             │
│     ├─ Loss computation                                │
│     ├─ Backward: only through LoRA                     │
│     └─ Optimizer step                                  │
│                                                          │
│  5. Checkpoint & Export                                │
│     ├─ Save LoRA weights                               │
│     ├─ Export metadata                                 │
│     └─ (Optional) Merge with base                      │
│                                                          │
└────────────────────────────────────────────────────────┘
```

### LoRA Formula

**Forward Pass:**
```
h' = h + ΔW · h
where ΔW = (B @ A) * (α / r)

h: Input from base model
B: (in_dim, rank) trainable matrix
A: (rank, out_dim) trainable matrix  
α: Scaling factor
r: LoRA rank
```

**Backward Pass:**
```
Only compute gradients w.r.t. B and A
Base model weights remain frozen
```

## Prerequisites

### Required Files

1. **Base Model (GGUF format)**
   - Llama 2: 7B, 13B, 70B
   - Llama 3: 8B, 70B
   - Mistral: 7B
   - CodeLlama
   - Download from HuggingFace or other sources

2. **Training Dataset**
   - Format: JSONL, Alpaca, ShareGPT, or plain text
   - Minimum: 10-100 samples for testing
   - Recommended: 1,000+ samples for fine-tuning
   - Production: 10,000-100,000 samples

### System Requirements

**Minimum:**
- CPU: 4+ cores
- RAM: 16 GB (for 7B models)
- Storage: 10 GB (model + data + checkpoints)

**Recommended:**
- CPU: 8+ cores or GPU (CUDA/Metal/Vulkan)
- RAM: 32 GB
- Storage: 50 GB SSD

**Memory Usage:**
```
Model          Precision  Base Size  LoRA (r=16)  Total
-------------------------------------------------------------
Llama-2-7B     FP16       ~14 GB     ~50 MB       ~14 GB
Llama-2-7B     Q4_K_M     ~4 GB      ~50 MB       ~4 GB
Llama-2-13B    FP16       ~26 GB     ~80 MB       ~26 GB
Llama-2-70B    Q4_K_M     ~40 GB     ~200 MB      ~40 GB
```

## Configuration

### LoRA Rank Selection

**Rank 4-8: Quick Experiments**
- Parameters: ~10-20 MB
- Training time: Fast (~30 min for 1K samples)
- Quality: Basic adaptation
- Use case: Testing, prototyping

**Rank 16-32: Standard Fine-tuning (Recommended)**
- Parameters: ~50-100 MB
- Training time: Medium (~2 hours for 10K samples)
- Quality: Good adaptation
- Use case: Most production scenarios

**Rank 64-128: High-Quality Fine-tuning**
- Parameters: ~200-400 MB
- Training time: Slow (~8 hours for 50K samples)
- Quality: Excellent adaptation
- Use case: Critical applications, complex tasks

### Target Module Selection

**Query + Value (Q+V)**
```yaml
target_modules: ["attention.wq", "attention.wv"]
```
- Fastest, minimal parameters
- Good for simple tasks

**All Attention (Q+K+V+O)**
```yaml
target_modules: ["attention.wq", "attention.wk", "attention.wv", "attention.wo"]
```
- Balanced speed and quality
- **Recommended** for most cases

**Attention + MLP**
```yaml
target_modules: 
  - "attention.wq"
  - "attention.wk"
  - "attention.wv"
  - "attention.wo"
  - "feed_forward.w1"
  - "feed_forward.w2"
  - "feed_forward.w3"
```
- Highest quality
- Slower training, more parameters

### Alpha Scaling

**Rule of Thumb:** `alpha = 2 * rank`
- Rank 8 → Alpha 16
- Rank 16 → Alpha 32
- Rank 32 → Alpha 64

## Data Preparation

### Alpaca Format

```json
[
  {
    "instruction": "What is the capital of France?",
    "input": "",
    "output": "The capital of France is Paris."
  },
  {
    "instruction": "Translate the following to Spanish",
    "input": "Hello, how are you?",
    "output": "Hola, ¿cómo estás?"
  }
]
```

### ShareGPT Format

```json
[
  {
    "conversations": [
      {"from": "human", "value": "What is 2+2?"},
      {"from": "gpt", "value": "2+2 equals 4."}
    ]
  }
]
```

### JSONL Format

```jsonl
{"instruction": "Add numbers", "input": "", "output": "Result: 42"}
{"instruction": "Another task", "input": "", "output": "Done"}
```

### Creating a Toy Dataset

```cpp
// Generate synthetic data for testing
auto samples = data_utils::createToyDataset(100);

// Or create custom samples
std::vector<InstructionDataSample> samples;
InstructionDataSample sample;
sample.instruction = "Explain quantum computing";
sample.input = "";
sample.output = "Quantum computing uses quantum mechanics...";
samples.push_back(sample);
```

## Training

### Complete Training Pipeline

```cpp
#include "llm/lora_framework/base_model_adapter.h"
#include "llm/lora_framework/data_loader.h"
#include "llm/lora_framework/lora_training_service.h"

// 1. Configure model
LoRAEnhancedModel::Config model_config;
model_config.base_model_path = "models/llama-2-7b.gguf";
model_config.lora_config.rank = 16;
model_config.lora_config.alpha = 32.0f;
model_config.lora_config.learning_rate = 3e-4f;
model_config.target_modules = {"attention.wq", "attention.wk", 
                                "attention.wv", "attention.wo"};

// 2. Initialize model
LoRAEnhancedModel model(model_config);
if (!model.initialize()) {
    std::cerr << "Failed to initialize model" << std::endl;
    return;
}

// 3. Load data
auto tokenizer = std::make_shared<SimpleTokenizer>();
DataLoaderConfig data_config;
data_config.batch_size = 4;
data_config.max_sequence_length = 2048;
data_config.shuffle = true;

DataLoader loader(tokenizer, data_config);
if (!loader.loadFromFile("data/train.json")) {
    std::cerr << "Failed to load dataset" << std::endl;
    return;
}

// 4. Setup optimizer
SGDOptimizer optimizer(model_config.lora_config.learning_rate);
optimizer.add_parameters(model.getTrainableParameters());

// 5. Training loop
const int num_epochs = 3;
for (int epoch = 0; epoch < num_epochs; ++epoch) {
    loader.reset();
    float epoch_loss = 0.0f;
    int step = 0;
    
    while (loader.hasNext()) {
        auto batch = loader.getNextBatch();
        
        // Forward pass (simplified - actual implementation would be more complex)
        // Tensor output = model.forward(input);
        // float loss = compute_loss(output, labels);
        
        // Backward pass
        // Tensor grad = compute_gradient(loss);
        // model.backward(grad);
        
        // Optimizer step
        optimizer.step();
        optimizer.zero_grad();
        
        // Log progress
        if (step % 10 == 0) {
            spdlog::info("Epoch {}/{}, Step {}, Loss: {:.4f}", 
                        epoch + 1, num_epochs, step, epoch_loss / (step + 1));
        }
        
        step++;
    }
}

// 6. Export trained adapter
auto weights = model.exportLoRAWeights();
// Save to file...
```

### Training Hyperparameters

**Learning Rate:**
- Start: 3e-4 (0.0003)
- Range: 1e-5 to 1e-3
- Adjust based on loss curve

**Batch Size:**
- Small models (7B): 4-8
- Large models (70B): 1-2
- Depends on available memory

**Epochs:**
- Quick test: 1 epoch
- Standard: 3-5 epochs
- Avoid overfitting: monitor validation loss

## Inference

### Using Trained LoRA Adapter

```cpp
// Load base model
BaseModelAdapter base;
base.loadModel("models/llama-2-7b.gguf");

// Load LoRA adapter
LoRAEnhancedModel model(config);
model.initialize();

// Import trained weights
std::unordered_map<std::string, std::pair<Tensor, Tensor>> weights;
// Load weights from file...
model.importLoRAWeights(weights);

// Run inference
// ... (similar to standard llama.cpp inference)
```

## Examples

### Example 1: Instruction Following

```cpp
// Train model to follow instructions
auto samples = {
    {"Explain gravity", "", "Gravity is a force that attracts objects..."},
    {"Write a haiku", "", "Code flows like water\nBugs hide in shadows deep\nDebug and rejoice"},
    // More samples...
};
```

### Example 2: Domain Adaptation (Medical)

```cpp
// Fine-tune for medical terminology
auto medical_data = {
    {"What is hypertension?", "", "Hypertension is high blood pressure..."},
    {"Explain diabetes", "", "Diabetes is a metabolic disorder..."},
    // More medical samples...
};
```

### Example 3: Code Generation

```cpp
// Fine-tune for code generation
auto code_samples = {
    {"Write a Python function to sort a list", "", 
     "def sort_list(lst):\n    return sorted(lst)"},
    {"Create a C++ class for a stack", "",
     "template<typename T>\nclass Stack { ... }"},
    // More code samples...
};
```

## Troubleshooting

### Common Issues

**1. Out of Memory**
- Reduce batch size
- Use quantized base model (Q4_K_M)
- Decrease sequence length
- Lower LoRA rank

**2. Loss Not Decreasing**
- Check learning rate (try 1e-4 or 1e-3)
- Verify data format and quality
- Increase LoRA rank
- Add more target modules

**3. Model File Not Found**
- Verify GGUF file path
- Check file permissions
- Ensure model is downloaded completely

**4. Slow Training**
- Enable GPU offloading (if available)
- Reduce batch size for CPU
- Use smaller base model
- Decrease LoRA rank

**5. Quality Issues**
- Increase training data size
- Train for more epochs
- Use higher LoRA rank
- Add more target modules
- Check data quality

### Debug Tips

```cpp
// Enable verbose logging
spdlog::set_level(spdlog::level::debug);

// Check model loading
if (!base.isLoaded()) {
    spdlog::error("Base model failed to load");
}

// Verify layer mapping
auto layers = base.getAdaptableLayers();
for (const auto& layer : layers) {
    spdlog::debug("Found layer: {} ({}x{})", 
                 layer.name, layer.in_features, layer.out_features);
}

// Monitor parameter count
spdlog::info("Base params: {}", model.getBaseModelParameterCount());
spdlog::info("LoRA params: {}", model.getLoRAParameterCount());
```

## Performance Tips

### Memory Optimization
- Use quantized base models (Q4, Q5, Q8)
- Gradient accumulation for larger effective batch size
- Mixed precision training (future feature)

### Speed Optimization
- GPU offloading for base model
- Parallel data loading
- Compiled optimizers (future feature)

### Quality Optimization
- Larger LoRA rank (64-128)
- More training data
- Learning rate scheduling
- Validation set for early stopping

## Next Steps

1. **Test with toy dataset** - Verify training works
2. **Prepare real dataset** - 1K+ high-quality samples
3. **Run baseline training** - Standard settings (rank=16)
4. **Evaluate quality** - Generate samples, check outputs
5. **Optimize hyperparameters** - Tune for your use case
6. **Scale up** - Larger models, more data

## References

- [LoRA Paper](https://arxiv.org/abs/2106.09685)
- [Llama 2 Paper](https://arxiv.org/abs/2307.09288)
- [llama.cpp](https://github.com/ggerganov/llama.cpp)
- [Alpaca Dataset](https://github.com/tatsu-lab/stanford_alpaca)
