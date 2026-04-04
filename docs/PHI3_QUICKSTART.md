# Phi-3-Mini-4k Quick Start

ThemisDB now includes **Phi-3-Mini-4k-Instruct** as the default LLM model with automatic download and native LoRA training support!

## What is Phi-3?

**Phi-3-Mini-4k-Instruct** is Microsoft's efficient small language model:
- ✅ **Only 2.3 GB** (Q4_K_M quantization)
- ✅ **3.8B parameters** - efficient yet capable
- ✅ **MIT License** - free for commercial use
- ✅ **4k context** - perfect for database queries
- ✅ **Grouped Query Attention** - modern efficient architecture

## Quick Start

### Option 1: CLI Tool (Recommended)

Use the `themis-model` CLI tool to download models (similar to Ollama):

```bash
# Download Phi-3 model
themis-model pull phi3:mini-4k

# List downloaded models
themis-model list

# Start server
themis-server --config config.yaml
```

**Output:**
```
Pulling model: phi3:mini-4k
Destination: models/default

▐████████████████████████████████░░░░░░░▌ 65.2% (1.50 GB / 2.30 GB) 14.2 MB/s
```

See [THEMIS_MODEL_CLI.md](en/llm/THEMIS_MODEL_CLI.md) for complete CLI documentation.

### Option 2: Auto-Download on First Use

Simply enable LLM in your configuration - the model downloads automatically:

```yaml
# config.yaml
llm:
  enabled: true
  auto_download: true
  model_path: "models/default/phi-3-mini-4k-instruct-q4.gguf"
```

Start the server - Phi-3 downloads on first start:

```bash
./themis-server --config config.yaml
```

**Output:**
```
[INFO] Model not found: models/default/phi-3-mini-4k-instruct-q4.gguf
[INFO] Starting auto-download...
[INFO] Download progress: 15.2% (350 MB / 2300 MB)
[INFO] Download progress: 65.8% (1513 MB / 2300 MB)
[INFO] ✓ Model downloaded successfully
[INFO] ✓ Phi-3 model loaded and ready
```

### 2. Simple Query

```cpp
#include "llm/embedded_llm.h"

std::string response = THEMIS_LLM_GENERATE(
    "How do I create a vector index in ThemisDB?"
);
```

### 3. Train a LoRA Adapter

Train custom adapters for your domain:

```cpp
#include "llm/lora_framework/lora_training_service.h"

// Load Phi-3 configuration
auto config = LoRATrainingService::Config::fromFile(
    "config/phi3_lora_training.yaml"
);

LoRATrainingService trainer(config);

// Prepare training data
lora::TrainingData data;
data.dataset_name = "my_domain";
// Add samples...

// Train (Phi-3 auto-detected!)
auto result = trainer.trainOnTheFly("my_adapter_v1", data);
```

## Configuration

### Environment Variables

Quick configuration without editing files:

```bash
# GPU acceleration (if available)
export THEMIS_GPU_LAYERS=24  # ~2GB VRAM required

# More CPU threads for faster inference
export THEMIS_THREADS=8

# Disable auto-download (for air-gapped environments)
export THEMIS_DISABLE_AUTO_DOWNLOAD=1

# Custom model directory
export THEMIS_MODEL_DIR=/custom/models
```

### Detailed Configuration

See `config/default_model_config.yaml` for full options:
- Runtime settings (context size, GPU layers, threads)
- Inference parameters (temperature, top_p, top_k)
- Memory limits (RAM, VRAM)
- Monitoring and security

## LoRA Training

Phi-3 is **automatically detected** and configured with optimal settings:

- **Target Modules**: `qkv_proj`, `o_proj`, `gate_up_proj`, `down_proj`
- **Hyperparameters**: rank=16, alpha=32, lr=2e-4
- **Architecture**: Grouped Query Attention (GQA)

Training is **fast and efficient**:
- 50 samples: ~3-5 min (CPU), ~1-2 min (GPU)
- 100 samples: ~5-8 min (CPU), ~2-3 min (GPU)

## Examples

Check out the examples:

```bash
# Basic query example
./examples/phi3_query_example

# With custom prompt
./examples/phi3_query_example "Explain AQL queries"

# LoRA training example
./examples/phi3_lora_training_example
```

## Performance

### Inference Speed

| Hardware | Tokens/sec | First Token |
|----------|-----------|-------------|
| CPU (4 threads) | 8-12 | 500-800ms |
| CPU (8 threads) | 15-20 | 400-600ms |
| NVIDIA RTX 3060 | 45-60 | 150-250ms |
| Apple M1 Pro | 25-35 | 250-350ms |

### Memory Usage

| Configuration | RAM | VRAM |
|--------------|-----|------|
| CPU-only | ~2.5 GB | - |
| GPU (24 layers) | ~1.0 GB | ~2.0 GB |

## Documentation

📚 **[Complete Guide](docs/en/llm/PHI3_INTEGRATION.md)**

Topics covered:
- Why Phi-3?
- Configuration options
- Usage examples
- LoRA training guide
- Performance tuning
- Troubleshooting
- Advanced topics

## Troubleshooting

### Model Download Fails

**Check Ollama is running:**
```bash
curl http://localhost:11434/api/tags
```

**Try alternative endpoint:**
```bash
export THEMIS_OLLAMA_ENDPOINT=http://127.0.0.1:11434
```

### Out of Memory

**Reduce memory usage:**
```yaml
runtime:
  context_size: 2048  # Reduce from 4096
  n_gpu_layers: 0     # Disable GPU
```

### Slow Inference

**Enable GPU acceleration:**
```bash
export THEMIS_GPU_LAYERS=24
```

**Increase threads:**
```bash
export THEMIS_THREADS=8
```

## What's Included

This integration includes:

✅ **Configuration Files**
- `config/default_model_config.yaml` - Phi-3 runtime settings
- `config/phi3_lora_training.yaml` - LoRA training config
- `config/ai_ml/llm/models.yaml` - Model registry entry

✅ **Auto-Download**
- `src/llm/model_downloader.cpp` - HTTP/Ollama download
- Progress tracking and resume support
- SHA256 checksum verification

✅ **Server Integration**
- `src/main_server.cpp` - Auto-download on startup
- Environment variable overrides
- Graceful fallback handling

✅ **LoRA Training**
- Phi-3 architecture detection
- Optimized target modules (GQA)
- Pre-configured hyperparameters

✅ **Examples**
- `examples/phi3_query_example.cpp` - Basic queries
- `examples/phi3_lora_training_example.cpp` - Training adapters

✅ **Documentation**
- `docs/en/llm/PHI3_INTEGRATION.md` - Complete guide
- Performance benchmarks
- Troubleshooting tips

✅ **Tests**
- `tests/test_phi3_integration.cpp` - Unit tests
- Configuration validation
- Download functionality

## Next Steps

1. **Start the server** - Model downloads automatically
2. **Try a query** - Use examples or embedded LLM API
3. **Train an adapter** - Customize for your domain
4. **Monitor performance** - Check inference speed and memory

## Support

- 📖 [Full Documentation](docs/en/llm/PHI3_INTEGRATION.md)
- 💬 [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- 🐛 [Report Issues](https://github.com/makr-code/ThemisDB/issues)
- 📧 Email: support@themisdb.io

---

**Note:** Phi-3-Mini-4k-Instruct is licensed under MIT by Microsoft. ThemisDB integration code follows the project's main license.
