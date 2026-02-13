# Phi-3-Mini-4k Integration Guide

## Overview

ThemisDB integrates Microsoft's Phi-3-Mini-4k-Instruct as the default LLM model, providing out-of-the-box AI capabilities with automatic model download, native LoRA training support, and optimized configuration for production deployment.

## Table of Contents

- [Why Phi-3?](#why-phi-3)
- [Features](#features)
- [Quick Start](#quick-start)
- [Configuration](#configuration)
- [Usage Examples](#usage-examples)
- [LoRA Training](#lora-training)
- [Performance](#performance)
- [Troubleshooting](#troubleshooting)
- [Advanced Topics](#advanced-topics)

## Why Phi-3?

Phi-3-Mini-4k-Instruct is the ideal default model for ThemisDB:

### Size & Efficiency
- **Only 2.3 GB** (Q4_K_M quantization) - reasonable for distribution
- **3.8 billion parameters** - efficient yet capable
- **4k context window** - suitable for most queries

### Performance
- **Microsoft-trained** with excellent reasoning capabilities
- **Superior performance** for technical/database tasks
- **Modern architecture** with Grouped Query Attention (GQA)

### Licensing
- **MIT License** - fully compatible with commercial use
- No restrictions on deployment or fine-tuning

### Architecture Highlights
- **Grouped Query Attention (GQA)** for efficient inference
- **32 attention heads** with 8 key-value heads (4:1 ratio)
- **3072 hidden size** with 8192 intermediate size
- **32 hidden layers** for deep understanding

## Features

### Automatic Model Download
- Downloads on first use via Ollama
- Progress tracking with ETA
- Resume capability for interrupted downloads
- Checksum verification (optional)

### Native LoRA Training
- Phi-3 specific target modules (qkv_proj, o_proj, gate_up_proj, down_proj)
- Optimized hyperparameters (rank=16, alpha=32, lr=2e-4)
- Auto-detection of Phi-3 architecture
- Mixed precision training (FP16)

### Production Ready
- Lazy loading (loads on first request)
- Memory-efficient caching
- CPU-only default (GPU optional)
- Configurable threading (default: 4 threads)

## Quick Start

### 1. Basic Setup

The Phi-3 model is configured as the default in `config/default_model_config.yaml`. No additional configuration is required for basic usage.

```yaml
default_llm:
  model_id: "phi-3-mini-4k-default"
  auto_download: true
  runtime:
    context_size: 4096
    n_gpu_layers: 0  # CPU-only by default
    threads: 4
```

### 2. First Query

The model will automatically download on first use:

```cpp
#include "llm/embedded_llm.h"

int main() {
    // Initialize with default model (auto-downloads if needed)
    themis::llm::EmbeddedLLMManager::instance().initialize();
    
    // Simple query
    std::string response = THEMIS_LLM_GENERATE("What is ThemisDB?");
    std::cout << response << std::endl;
    
    return 0;
}
```

### 3. Monitor Download Progress

On first use, you'll see download progress:

```
[INFO] Phi-3 model not found. Starting auto-download...
[INFO] Model: Phi-3-Mini-4k-Instruct
[INFO] Size: 2.3 GB
[INFO] Downloading: 15.2% (350 MB / 2300 MB) - Speed: 12.5 MB/s
[INFO] Downloading: 32.8% (755 MB / 2300 MB) - Speed: 14.2 MB/s
...
[INFO] ✓ Phi-3 model downloaded successfully
[INFO] ✓ Phi-3 model loaded and ready
```

## Configuration

### Environment Variables

Override configuration without editing files:

```bash
# Disable auto-download (useful for air-gapped environments)
export THEMIS_DISABLE_AUTO_DOWNLOAD=1

# Custom model storage location
export THEMIS_MODEL_DIR=/custom/path/to/models

# GPU acceleration
export THEMIS_GPU_LAYERS=24  # Offload all layers to GPU

# CPU threads
export THEMIS_THREADS=8

# Context size
export THEMIS_CONTEXT_SIZE=8192

# Ollama endpoint
export THEMIS_OLLAMA_ENDPOINT=http://custom-ollama:11434
```

### Configuration File

Edit `config/default_model_config.yaml` for persistent changes:

```yaml
default_llm:
  # Download settings
  auto_download: true
  ollama:
    model_name: "phi3:mini-4k"
    endpoint: "http://localhost:11434"
  
  # Runtime settings
  runtime:
    context_size: 4096
    n_gpu_layers: 24      # GPU acceleration (0 = CPU-only)
    threads: 8            # CPU threads
    batch_size: 512
    
    memory:
      max_ram_mb: 8192    # Maximum RAM usage
      max_vram_mb: 2048   # Maximum VRAM usage (if GPU enabled)
  
  # Inference settings
  inference:
    temperature: 0.7
    top_p: 0.95
    top_k: 40
    repeat_penalty: 1.1
    max_tokens: 2048
```

### GPU Acceleration

Enable GPU acceleration for faster inference:

```yaml
runtime:
  n_gpu_layers: 24  # Offload all 24 layers to GPU
  
  memory:
    max_vram_mb: 2048  # Requires ~2GB VRAM for Q4_K_M
```

Supported backends:
- **CUDA** (NVIDIA GPUs)
- **Metal** (Apple Silicon)
- **Vulkan** (AMD GPUs via ROCm)

## Usage Examples

### Example 1: Simple Query

```cpp
#include "llm/embedded_llm.h"

std::string response = THEMIS_LLM_GENERATE(
    "How do I create a vector index in ThemisDB?"
);
```

### Example 2: With Parameters

```cpp
#include "llm/llama_wrapper.h"

themis::llm::InferenceRequest request;
request.prompt = "Explain graph queries in ThemisDB";
request.max_tokens = 512;
request.temperature = 0.7;
request.top_p = 0.95;

auto response = llama_wrapper.generate(request);
std::cout << response.text << std::endl;
```

### Example 3: Streaming Response

```cpp
#include "llm/embedded_llm.h"

THEMIS_LLM().generateStreaming(
    "Summarize this document...",
    [](const std::string& token) {
        std::cout << token << std::flush;
    }
);
```

### Example 4: Embeddings

```cpp
#include "llm/embedded_llm.h"

std::vector<float> embedding = THEMIS_LLM_EMBED(
    "semantic search query"
);

// Use for vector similarity search
```

## LoRA Training

### Overview

Train custom LoRA adapters on Phi-3 for domain-specific tasks without retraining the entire model.

### Configuration

Phi-3 LoRA training is configured in `config/phi3_lora_training.yaml`:

```yaml
phi3_lora_training:
  base_model:
    model_id: "phi-3-mini-4k-default"
  
  # Phi-3 specific target modules
  target_modules:
    - "qkv_proj"      # Combined Q/K/V (Phi-3 GQA)
    - "o_proj"        # Output projection
    - "gate_up_proj"  # Combined gate/up (MLP)
    - "down_proj"     # Down projection
  
  hyperparameters:
    rank: 16
    alpha: 32.0
    learning_rate: 0.0002
    batch_size: 4
    num_epochs: 3
```

### Training Example

```cpp
#include "llm/lora_framework/lora_training_service.h"

// Load configuration
auto config = loadPhi3TrainingConfig();
LoRATrainingService training_service(config);

// Prepare training data
themis::llm::lora::TrainingData data;
data.dataset_name = "themis_help";

themis::llm::lora::TrainingDataSample sample;
sample.input = "How to create a vector index?";
sample.output = "Use CREATE VECTOR INDEX idx ON collection(field) WITH METRIC 'cosine'.";
data.samples.push_back(sample);

// Train adapter
auto result = training_service.trainOnTheFly("themis_help_v1", data);

if (result.success) {
    std::cout << "Training completed!" << std::endl;
    std::cout << "Final loss: " << result.final_loss << std::endl;
    std::cout << "Accuracy: " << result.validation_accuracy * 100 << "%" << std::endl;
}
```

### Auto-Detection

The training service automatically detects Phi-3 and configures appropriate settings:

```cpp
// Detected automatically:
// - Phi-3 in model path or adapter ID
// - Target modules set to Phi-3 GQA architecture
// - Hyperparameters optimized for Phi-3
```

### Training Metrics

Monitor training progress:

```cpp
training_service.registerCallback([](const TrainingMetrics& metrics) {
    std::cout << "Epoch: " << metrics.current_epoch 
              << " | Loss: " << metrics.current_loss
              << " | LR: " << metrics.learning_rate << std::endl;
});
```

## Performance

### Inference Speed

| Hardware | Tokens/sec | Latency (first token) |
|----------|-----------|----------------------|
| CPU (4 threads) | 8-12 | 500-800 ms |
| CPU (8 threads) | 15-20 | 400-600 ms |
| NVIDIA RTX 3060 | 45-60 | 150-250 ms |
| NVIDIA RTX 4090 | 100-140 | 80-120 ms |
| Apple M1 Pro | 25-35 | 250-350 ms |
| Apple M2 Max | 40-55 | 180-280 ms |

### Memory Usage

| Configuration | RAM | VRAM |
|--------------|-----|------|
| CPU-only (Q4_K_M) | ~2.5 GB | - |
| GPU (24 layers) | ~1.0 GB | ~2.0 GB |
| With 4k context | +512 MB | +512 MB |

### LoRA Training Performance

| Dataset Size | Training Time (CPU) | Training Time (GPU) |
|-------------|-------------------|-------------------|
| 50 samples | ~3-5 min | ~1-2 min |
| 100 samples | ~5-8 min | ~2-3 min |
| 500 samples | ~20-30 min | ~8-12 min |
| 1000 samples | ~40-60 min | ~15-25 min |

*CPU: Intel i7-12700K (8 cores), GPU: NVIDIA RTX 3060 (12GB)*

## Troubleshooting

### Model Download Fails

**Problem:** Download fails with connection error

**Solutions:**
1. Check Ollama is running: `curl http://localhost:11434/api/tags`
2. Try alternative endpoint: `export THEMIS_OLLAMA_ENDPOINT=http://127.0.0.1:11434`
3. Check network connectivity
4. Verify disk space (need ~2.5 GB free)

### Out of Memory

**Problem:** Process crashes with OOM error

**Solutions:**
1. Reduce context size: `export THEMIS_CONTEXT_SIZE=2048`
2. Disable GPU: `export THEMIS_GPU_LAYERS=0`
3. Reduce batch size in config: `batch_size: 256`
4. Close other applications

### Slow Inference

**Problem:** Token generation is too slow

**Solutions:**
1. Enable GPU: `export THEMIS_GPU_LAYERS=24`
2. Increase threads: `export THEMIS_THREADS=8`
3. Enable flash attention in config: `enable_flash_attention: true`
4. Reduce context size if not needed

### LoRA Training Fails

**Problem:** Training fails with error

**Solutions:**
1. Verify base model exists and is accessible
2. Check training data format (input/output pairs)
3. Reduce batch size if OOM: `batch_size: 2`
4. Enable gradient checkpointing for lower memory
5. Verify GGUF model file is not corrupted

### Model Not Found

**Problem:** "Model not found" error despite download

**Solutions:**
1. Check model path in config: `config/default_model_config.yaml`
2. Verify file exists: `ls -lh models/default/phi-3-mini-4k-instruct-q4.gguf`
3. Check file permissions
4. Try manual download: see [Manual Installation](#manual-installation)

## Advanced Topics

### Manual Installation

If auto-download fails, manually install the model:

```bash
# Using Ollama
ollama pull phi3:mini-4k

# Find model file
ls ~/.ollama/models/

# Copy to ThemisDB models directory
mkdir -p models/default
cp ~/.ollama/models/blobs/sha256-* models/default/phi-3-mini-4k-instruct-q4.gguf
```

### Custom Model Variants

Use different Phi-3 variants:

```yaml
# 128k context version
ollama:
  model_name: "phi3:mini-128k"

runtime:
  context_size: 131072  # 128k context
```

### Distributed Training

Train LoRA adapters across multiple GPUs or nodes:

```yaml
phi3_lora_training:
  hardware:
    multi_gpu:
      enabled: true
      strategy: "ddp"  # DistributedDataParallel
      num_gpus: 4
```

### QLoRA Training

For memory-constrained environments, use QLoRA (quantized LoRA):

```yaml
phi3_lora_training:
  qlora:
    enabled: true
    quantization_type: "nf4"
    block_size: 64
    double_quantization: true
```

### Custom Target Modules

Override Phi-3 target modules for experimentation:

```yaml
phi3_lora_training:
  target_modules:
    - "qkv_proj"
    - "o_proj"
    # Add custom modules
```

### Model Quantization

Different quantization levels for size/quality tradeoff:

| Quantization | Size | Quality | Speed |
|-------------|------|---------|-------|
| Q4_K_M | 2.3 GB | Good | Fast |
| Q5_K_M | 2.8 GB | Better | Moderate |
| Q8_0 | 4.1 GB | Best | Slower |

### Benchmarking

Run performance benchmarks:

```bash
# Inference benchmark
./benchmarks/bench_phi3_inference

# Training benchmark
./benchmarks/bench_phi3_lora_training
```

## References

- [Phi-3 Model Card](https://huggingface.co/microsoft/Phi-3-mini-4k-instruct)
- [Phi-3 Technical Report](https://arxiv.org/abs/2404.14219)
- [LoRA Paper](https://arxiv.org/abs/2106.09685)
- [ThemisDB Documentation](https://themisdb.io/docs)
- [llama.cpp Repository](https://github.com/ggerganov/llama.cpp)

## Support

- **Issues:** [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- **Discussions:** [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- **Email:** support@themisdb.io

## License

Phi-3-Mini-4k-Instruct is licensed under the MIT License by Microsoft.
ThemisDB integration code is licensed under the project's main license.
