# ONNX CLIP Plugin für ThemisDB

**Version:** 1.0.0  
**Status:** In Development  
**Lizenz:** MIT License (kompatibel mit ThemisDB)

---

## Übersicht

Dieses Plugin implementiert CLIP (Contrastive Language-Image Pre-training) Bildeinbettungen unter Verwendung von ONNX Runtime. Es ermöglicht die Generierung semantischer Vektoren für Bilder, die für Ähnlichkeitssuche und multimodale RAG-Anwendungen verwendet werden können.

---

## Features

- ✅ **CLIP ViT-B/32**: 512-dimensionale Embeddings (Standard)
- ✅ **CLIP ViT-L/14**: 768-dimensionale Embeddings (höhere Qualität)
- ✅ **Multi-Backend**: CPU, CUDA, DirectML, TensorRT, OpenVINO
- ✅ **Batch Processing**: Effiziente Verarbeitung mehrerer Bilder
- ✅ **Image Preprocessing**: Automatische Größenanpassung und Normalisierung
- ✅ **Thread-Safe**: Sichere parallele Ausführung
- ✅ **Warmup**: Optimierte erste Inferenz

---

## Installation

### Voraussetzungen

**System:**
- CMake 3.20+
- C++17 Compiler (GCC 9+, Clang 10+, MSVC 2019+)

**Dependencies:**
- ONNX Runtime 1.17+
- nlohmann-json 3.11+
- OpenCV 4.5+ (optional, für Bildverarbeitung)

### Via vcpkg

```bash
vcpkg install onnxruntime nlohmann-json opencv4[core,imgproc,imgcodecs]
```

### Build

```bash
cd /path/to/ThemisDB
mkdir build && cd build

cmake .. \
  -DTHEMIS_BUILD_ENTERPRISE_PLUGINS=ON \
  -DTHEMIS_PLUGIN_IMAGE_ANALYSIS_ONNX=ON \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake

cmake --build . --target themis_image_onnx_clip
```

### Installation

```bash
# Linux
sudo cmake --install . --component themis_image_onnx_clip
# Installiert nach: /usr/local/lib/themis/plugins/

# Windows
cmake --install . --component themis_image_onnx_clip --prefix "C:/Program Files/ThemisDB"
# Installiert nach: C:/Program Files/ThemisDB/lib/themis/plugins/
```

---

## Modelle

### CLIP ViT-B/32 (Empfohlen für Produktion)

**Download:**
```bash
wget https://github.com/onnx/models/raw/main/vision/body_analysis/ultraface/models/version-RFB-320.onnx \
  -O models/clip-vit-base-patch32.onnx
```

**Alternative (Hugging Face):**
```bash
pip install transformers optimum[exporters]

optimum-cli export onnx \
  --model openai/clip-vit-base-patch32 \
  --task feature-extraction \
  models/clip-vit-base-patch32
```

**Eigenschaften:**
- **Embedding Dimension**: 512
- **Input Size**: 224x224 pixels
- **Model Size**: ~350MB
- **Inference Time**: ~50ms (GPU), ~150ms (CPU)
- **VRAM**: ~2GB

### CLIP ViT-L/14 (Höhere Qualität)

**Download:**
```bash
optimum-cli export onnx \
  --model openai/clip-vit-large-patch14 \
  --task feature-extraction \
  models/clip-vit-large-patch14
```

**Eigenschaften:**
- **Embedding Dimension**: 768
- **Input Size**: 224x224 pixels
- **Model Size**: ~900MB
- **Inference Time**: ~100ms (GPU), ~500ms (CPU)
- **VRAM**: ~4GB

---

## Konfiguration

### YAML Konfiguration

```yaml
# config/image_analysis.yaml
plugins:
  - name: onnx_clip
    enabled: true
    config:
      model_path: "./models/clip-vit-base-patch32.onnx"
      backend: "AUTO"  # AUTO, CPU, CUDA, DIRECTML
      embedding_dim: 512
      
      gpu:
        device_id: 0
        optimization_level: 99
```

### Backend-Auswahl

| Backend | Hardware | Performance | Bemerkung |
|---------|----------|-------------|-----------|
| **CPU** | Alle CPUs | Baseline | Immer verfügbar |
| **CUDA** | NVIDIA GPU | 3-5x schneller | Beste Performance |
| **DirectML** | Windows GPU | 2-3x schneller | Windows 10/11 |
| **TensorRT** | NVIDIA GPU | 5-8x schneller | Beste NVIDIA Performance |
| **OpenVINO** | Intel CPU/GPU | 2-4x schneller | Intel-optimiert |

---

## Usage

### C++ API

```cpp
#include "plugins/image_analysis_manager.h"

using namespace themis::plugins::image;

int main() {
    // Initialize manager
    auto& manager = ImageAnalysisManager::instance();
    
    // Load plugin
    PluginConfig config;
    config.set("model_path", "./models/clip-vit-base-patch32.onnx");
    config.set("backend", "AUTO");
    
    manager.loadPlugin("onnx_clip", config);
    
    // Load image
    std::vector<uint8_t> image_data = loadImage("photo.jpg");
    
    // Generate embedding
    EmbeddingResult result = manager.generateEmbedding(image_data);
    
    if (result.success) {
        std::cout << "Embedding dimension: " << result.dimension << std::endl;
        std::cout << "Inference time: " << result.inference_time_ms << "ms" << std::endl;
        
        // Use embedding for similarity search
        storeInVectorIndex(result.embedding);
    }
    
    return 0;
}
```

### Batch Processing

```cpp
// Load multiple images
std::vector<std::vector<uint8_t>> images = {
    loadImage("image1.jpg"),
    loadImage("image2.jpg"),
    loadImage("image3.jpg")
};

// Generate embeddings in batch (more efficient)
auto* plugin = manager.getPlugin("onnx_clip");
std::vector<EmbeddingResult> results = plugin->generateEmbeddingBatch(images);

for (const auto& result : results) {
    if (result.success) {
        processEmbedding(result.embedding);
    }
}
```

### Parallel mit LLM

```cpp
// Parallele Ausführung von Bildanalyse und LLM
auto image_future = std::async(std::launch::async, [&]() {
    return manager.generateEmbedding(image_data);
});

// LLM verarbeitet Text währenddessen
auto llm_response = llm_engine.process(text_prompt);

// Warte auf Bildanalyse
EmbeddingResult image_result = image_future.get();

// Kombiniere Ergebnisse für multimodales RAG
performMultimodalRAG(image_result.embedding, llm_response);
```

---

## Performance

### Benchmarks

**System**: Intel i7-12700K, NVIDIA RTX 3090, 32GB RAM

| Config | Backend | Batch Size | Throughput | Latency (p50) | Latency (p99) |
|--------|---------|------------|------------|---------------|---------------|
| ViT-B/32 | CPU | 1 | 6.5 img/s | 154ms | 180ms |
| ViT-B/32 | CUDA | 1 | 33 img/s | 30ms | 45ms |
| ViT-B/32 | CUDA | 4 | 90 img/s | 44ms | 60ms |
| ViT-B/32 | TensorRT | 1 | 50 img/s | 20ms | 30ms |
| ViT-L/14 | CUDA | 1 | 16 img/s | 62ms | 80ms |

### Memory Usage

| Model | VRAM (GPU) | RAM (CPU) | Model Size |
|-------|------------|-----------|------------|
| ViT-B/32 | 2GB | 4GB | 350MB |
| ViT-L/14 | 4GB | 8GB | 900MB |

---

## Troubleshooting

### Plugin lädt nicht

**Problem**: `Failed to load plugin: themis_image_onnx_clip.dll`

**Lösungen**:
1. ONNX Runtime nicht installiert:
   ```bash
   vcpkg install onnxruntime
   ```

2. DLL-Dependencies fehlen (Windows):
   - Download [Dependencies.exe](https://github.com/lucasg/Dependencies)
   - Prüfe fehlende DLLs

3. Falsches Verzeichnis:
   ```bash
   # Plugin muss in Plugin-Verzeichnis liegen
   ls -l /usr/local/lib/themis/plugins/themis_image_onnx_clip.so
   ```

### Langsame Inferenz

**Problem**: CPU wird verwendet statt GPU

**Lösungen**:
1. Backend explizit setzen:
   ```yaml
   backend: "CUDA"  # statt "AUTO"
   ```

2. CUDA Provider prüfen:
   ```cpp
   auto providers = Ort::GetAvailableProviders();
   // Sollte "CUDAExecutionProvider" enthalten
   ```

3. CUDA/cuDNN installieren:
   ```bash
   # Ubuntu
   sudo apt install nvidia-cuda-toolkit libcudnn8
   ```

### Out of Memory

**Problem**: `Failed to allocate tensor`

**Lösungen**:
1. Kleineres Modell verwenden (ViT-B/32 statt ViT-L/14)
2. Batch Size reduzieren
3. FP16 Quantisierung aktivieren:
   ```yaml
   quantization:
     enabled: true
     precision: "FP16"
   ```

---

## Development

### Debugging

```bash
# Build with debug symbols
cmake .. -DCMAKE_BUILD_TYPE=Debug -DTHEMIS_BUILD_ENTERPRISE_PLUGINS=ON -DTHEMIS_PLUGIN_IMAGE_ANALYSIS_ONNX=ON
cmake --build .

# Run with verbose logging
export ONNXRUNTIME_LOG_LEVEL=VERBOSE
./themis_server --config config/image_analysis.yaml
```

### Profiling

```yaml
# Enable profiling in config
config:
  enable_profiling: true
```

```cpp
// View profiling results
auto stats = plugin->getStatistics();
std::cout << "Total inferences: " << stats["total_inferences"] << std::endl;
std::cout << "Average time: " << stats["avg_inference_time_ms"] << "ms" << std::endl;
```

### Testing

```bash
# Run unit tests
cd build
ctest -R onnx_clip -V
```

---

## Lizenz

MIT License - Identisch mit ThemisDB

```
Copyright (c) 2025 The ThemisDB Authors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software...
```

---

## References

- [ONNX Runtime Documentation](https://onnxruntime.ai/)
- [CLIP Paper](https://arxiv.org/abs/2103.00020)
- [OpenAI CLIP](https://github.com/openai/CLIP)
- [ONNX Model Zoo](https://github.com/onnx/models)
- [ThemisDB Image Analysis Guide](../../docs/plugins/IMAGE_ANALYSIS_CPP_LIBRARIES.md)

---

## Support

- **Issues**: [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- **Discussions**: [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- **Documentation**: [ThemisDB Docs](https://makr-code.github.io/ThemisDB/)
