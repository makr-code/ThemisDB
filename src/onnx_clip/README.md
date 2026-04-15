<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# ThemisDB ONNX CLIP Plugin

**Version:** 0.0.1  
**Status:** 🟢 Production-Ready  
**Last Updated:** 2026-04-06  
**Module Path:** `src/onnx_clip/`  
**Namespace:** `themis::plugins::image`

---

## Module Purpose

The ONNX CLIP plugin generates image embeddings from raw image bytes using OpenAI CLIP
models exported to ONNX format. It implements the `IImageAnalysisBackend` interface and
is compatible with the ThemisDB vector index for multi-modal similarity search.

Two CLIP model variants are supported:

| Model | Embedding Dim | Description |
|-------|--------------|-------------|
| `ViT-B/32` | 512 | Base model — lower latency, smaller memory footprint |
| `ViT-L/14` | 768 | Large model — higher accuracy, larger memory footprint |

The plugin supports four execution backends:

| Backend | Constant | Description |
|---------|----------|-------------|
| `CPU` | `BackendType::CPU` | ONNX Runtime CPU execution provider |
| `CUDA` | `BackendType::CUDA` | NVIDIA GPU via CUDA execution provider |
| `DirectML` | `BackendType::DirectML` | Windows GPU via DirectML execution provider |
| `TensorRT` | `BackendType::TensorRT` | NVIDIA TensorRT optimised execution |
| `AUTO` | `BackendType::AUTO` | Selects best available backend at runtime |

---

## Component Table

| File | Class / Role |
|------|-------------|
| `onnx_clip_plugin.h` | `ONNXClipPlugin` — `IImageAnalysisBackend` implementation with pImpl |
| `CMakeLists.txt` | Build configuration; links ONNX Runtime and OpenCV |

---

## Quick-Start Example

```cpp
#include "onnx_clip_plugin.h"
#include <fstream>

// 1. Create and initialise the plugin
themis::plugins::image::ONNXClipPlugin plugin;

PluginConfig config;
config["model_path"] = "models/clip_vit_b32.onnx";
config["model_variant"] = "ViT-B/32";

plugin.initialize(config, BackendType::CUDA);  // or AUTO

// 2. Load image bytes
std::ifstream f("photo.jpg", std::ios::binary);
std::vector<uint8_t> image(std::istreambuf_iterator<char>(f), {});

// 3. Generate embedding
auto result = plugin.generateEmbedding(image);
if (result.ok) {
    // result.embedding is a std::vector<float> of dim 512 (ViT-B/32)
    // Store in ThemisDB vector index for similarity search
}

// 4. Batch processing
auto batch_results = plugin.generateEmbeddingBatch({image1, image2, image3});

// 5. Text embedding (for cross-modal similarity search)
auto text_result = plugin.generateTextEmbedding("a photo of a cat");
// text_result.embedding is compatible with image embeddings

// 6. Health and stats
plugin.warmup();
bool healthy = plugin.healthCheck();
auto stats = plugin.getStatistics();  // JSON: calls, avg_latency_ms, backend_name, max_batch_size, etc.
```

---

## Plugin Registration

```cpp
// Registered automatically via the THEMIS_IMAGE_PLUGIN macro:
THEMIS_IMAGE_PLUGIN(themis::plugins::image::ONNXClipPlugin)
```

---

## Configuration Keys

| Key | Required | Default | Description |
|-----|----------|---------|-------------|
| `model_path` | Yes | — | Path to ONNX model file |
| `model_variant` | No | `ViT-B/32` | `"ViT-B/32"` or `"ViT-L/14"` |
| `model.embedding_dim` | No | `512` | Embedding dimension (512 for ViT-B/32, 768 for ViT-L/14) |
| `max_batch_size` | No | 16 (CPU) / 64 (GPU) | Maximum images per sub-batch call |
| `num_threads` | No | 4 | CPU thread count (CPU backend) |
| `gpu_device_id` | No | 0 | GPU device index (CUDA/TensorRT) |

---

## See Also

- `ARCHITECTURE.md` — pImpl design, inference pipeline
- `ROADMAP.md` — implementation phases and feature backlog
- `SECURITY.md` — model integrity and input validation
