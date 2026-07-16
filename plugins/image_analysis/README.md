# Image Analysis Plugins for ThemisDB

## Status Overview

This directory contains image analysis plugins that enable ThemisDB to process and analyze image data.

### Production Status
- ✅ **ONNX CLIP Plugin** - Production-ready, actively used

### Planned Plugins
- 📋 llama.cpp Vision Plugin - Planned
- 📋 OpenCV DNN Plugin - Planned
- 📋 Stable Diffusion Plugin - Planned

---

## Overview

Image analysis plugins enable ThemisDB to process and analyze image data in parallel with LLM operations. Each plugin implements the `IImageAnalysisBackend` interface and can provide capabilities such as:

For integrated plugins, canonical implementation code lives under `src/`, public API headers live under `include/plugins/`, and this directory primarily serves as documentation, manifests, and compatibility CMake entry points.

- **Image Embeddings**: Generate semantic vectors for similarity search (CLIP, etc.)
- **Image Captioning**: Generate text descriptions (LLaVA, BLIP-2, etc.)
- **Object Detection**: Detect and localize objects (YOLO, Faster R-CNN, etc.)
- **Image Segmentation**: Segment images into regions (SAM, Mask R-CNN, etc.)
- **Image Generation**: Generate images from text (Stable Diffusion, DALL-E, etc.)
- **Visual QA**: Answer questions about images (LLaVA, etc.)

## Available Plugins

### 1. ONNX CLIP Plugin (`onnx_clip/`) ✅

**Status**: ✅ Production-Ready

CLIP-based image embedding generation using ONNX Runtime.

**Public API:** `include/plugins/image_analysis/onnx_clip_plugin.h`  
**Implementation:** `src/onnx_clip/`  
**Legacy compatibility path:** `plugins/image_analysis/onnx_clip/`

**Features**:
- CLIP ViT-B/32 and ViT-L/14 support
- Multiple backends: CPU, CUDA, DirectML, TensorRT
- Batch processing
- 512-dimensional embeddings (default)

**Configuration**:
```yaml
image_analysis:
  plugins:
    - name: onnx_clip
      enabled: true
      config:
        model_path: "./models/clip-vit-base-patch32.onnx"
        backend: "AUTO"  # AUTO, CPU, CUDA, DIRECTML, TENSORRT
        embedding_dim: 512
```

**Usage**:
```cpp
auto& manager = ImageAnalysisManager::instance();
manager.loadPlugin("onnx_clip", config);

// Generate embedding
EmbeddingResult result = manager.generateEmbedding(image_bytes);
```

### 2. llama.cpp Vision Plugin (`llamacpp_vision/`) 📋

**Status**: 📋 Planned

Vision-language models (LLaVA) using llama.cpp.

**Features**:
- LLaVA 1.5/1.6 support
- Image captioning
- Visual question answering
- Unified memory with LLM system
- GGML quantization (4-bit, 5-bit, 8-bit)

### 3. OpenCV DNN Plugin (`opencv_dnn/`) 📋

**Status**: 📋 Planned

CPU-optimized image analysis using OpenCV DNN module.

**Features**:
- ResNet, MobileNet, YOLO support
- CPU fallback for systems without GPU
- Lightweight and fast
- Object detection and classification

### 4. Stable Diffusion ONNX Plugin (`stable_diffusion_onnx/`) 📋

**Status**: 📋 Planned

Image generation using Stable Diffusion via ONNX Runtime.

**Features**:
- Text-to-image generation
- Image-to-image transformation
- ControlNet support
- Multiple sampling methods
- GPU-accelerated (CUDA, DirectML, TensorRT)

---

## Plugin Architecture

```
┌─────────────────────────────────────────────────────┐
│           ThemisDB Core                             │
│  ┌────────────────────────────────────────────┐    │
│  │   ImageAnalysisManager                      │    │
│  │   (Plugin Discovery & Loading)              │    │
│  └────────────┬────────────────────────────────┘    │
│               │                                      │
│  ┌────────────┴────────────────────────────────┐    │
│  │     IImageAnalysisBackend Interface         │    │
│  └────┬───────┬───────┬──────────┬────────────┘    │
│       │       │       │          │                  │
│  ┌────▼──┐ ┌─▼────┐ ┌▼──────┐ ┌─▼────────┐        │
│  │ ONNX  │ │llama │ │OpenCV │ │ Stable   │        │
│  │CLIP   │ │.cpp  │ │ DNN   │ │Diffusion │        │
│  │Plugin │ │Vision│ │Plugin │ │  Plugin  │        │
│  └───────┘ └──────┘ └───────┘ └──────────┘        │
└─────────────────────────────────────────────────────┘
```

Integrated production plugins are built from `src/*`; legacy plugin subdirectories under `plugins/image_analysis/*` are retained for compatibility and documentation.

## Plugin Naming Convention

Plugin DLLs must follow this naming pattern:

```
Windows: themis_image_<name>.dll
Linux:   themis_image_<name>.so
macOS:   themis_image_<name>.dylib
```

Examples:
- `themis_image_onnx_clip.dll`
- `themis_image_llamacpp_vision.so`
- `themis_image_stable_diffusion.dylib`

## Creating a New Plugin

### 1. Create Plugin Sources

```bash
mkdir src/my_image_plugin
mkdir include/plugins/image_analysis
```

### 2. Implement Plugin Interface

```cpp
// include/plugins/image_analysis/my_plugin.h
#include "plugins/image_analysis_interface.h"

class MyImagePlugin : public IImageAnalysisBackend {
public:
    PluginInfo getInfo() const override {
        return {
            .name = "MyPlugin",
            .version = "1.0.0",
            .description = "My custom image analysis plugin",
            .capabilities = {
                .supports_embedding = true,
                .supported_backends = {BackendType::CPU, BackendType::CUDA}
            }
        };
    }
    
    bool initialize(const PluginConfig& config, BackendType backend) override {
        // Load model, allocate resources
        return true;
    }
    
    EmbeddingResult generateEmbedding(
        const std::vector<uint8_t>& image_data,
        const ImageMetadata* metadata
    ) override {
        // Your implementation here
    }
    
    // ... other methods
};

```

### 3. Create CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(themis_image_my_plugin)

add_library(themis_image_my_plugin SHARED
    my_plugin.cpp
)

target_include_directories(themis_image_my_plugin PRIVATE
    ${THEMIS_ROOT_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(themis_image_my_plugin PRIVATE
    # Your dependencies
)

install(TARGETS themis_image_my_plugin
    LIBRARY DESTINATION lib/themis/plugins
    RUNTIME DESTINATION lib/themis/plugins
)
```

The plugin export macro should stay in the implementation translation unit, not in the public header.

### 4. Build Plugin

```bash
cd build
cmake ..
cmake --build . --target themis_image_my_plugin
```

### 5. Install Plugin

Copy the built DLL/SO to ThemisDB's plugin directory:

```bash
# Linux
cp build/plugins/image_analysis/my_plugin/themis_image_my_plugin.so \
   /usr/local/lib/themis/plugins/

# Windows
copy build\plugins\image_analysis\my_plugin\themis_image_my_plugin.dll ^
     C:\Program Files\ThemisDB\plugins\
```

## Configuration

Plugins are configured in `config/image_analysis.yaml`:

```yaml
image_analysis:
  # Plugin directory to scan
  plugin_directory: "./plugins"
  
  # Auto-load plugins on startup
  auto_load: true
  
  # Default plugin for operations
  default_plugin: "onnx_clip"
  
  # Plugin-specific configurations
  plugins:
    - name: onnx_clip
      enabled: true
      config:
        model_path: "./models/clip-vit-base-patch32.onnx"
        backend: "AUTO"
        
    - name: llamacpp_vision
      enabled: false
      config:
        model_path: "./models/llava-v1.5-7b-q4_0.gguf"
        clip_model_path: "./models/mmproj-model-f16.gguf"
        backend: "CUDA"
```

## Usage Examples

### Generate Image Embedding

```cpp
#include "plugins/image_analysis_manager.h"

// Load image
std::vector<uint8_t> image_data = loadImageFile("photo.jpg");

// Generate embedding
auto& manager = ImageAnalysisManager::instance();
EmbeddingResult result = manager.generateEmbedding(image_data);

if (result.success) {
    // Store embedding in vector index
    storeInVectorIndex(result.embedding);
}
```

### Parallel Execution with LLM

```cpp
// Process image and text in parallel
auto image_future = std::async(std::launch::async, [&]() {
    return manager.generateEmbedding(image_data);
});

// LLM processes text while image is being analyzed
auto llm_response = llm_engine.process(text_prompt);

// Wait for image analysis
EmbeddingResult image_result = image_future.get();

// Combine results
combinedRAGQuery(image_result.embedding, llm_response);
```

### Image Captioning

```cpp
CaptionResult caption = manager.generateCaption(image_data, 50);

if (caption.success) {
    std::cout << "Caption: " << caption.caption << std::endl;
    std::cout << "Confidence: " << caption.confidence << std::endl;
}
```

### Object Detection

```cpp
DetectionResult detection = manager.detectObjects(image_data, 0.5f);

for (const auto& box : detection.detections) {
    std::cout << "Detected: " << box.label 
              << " (confidence: " << box.confidence << ")" << std::endl;
}
```

## Testing

Each plugin should include unit tests:

```cpp
// test_my_plugin.cpp
#include <gtest/gtest.h>
#include "my_plugin.h"

TEST(MyPluginTest, Initialization) {
    MyImagePlugin plugin;
    PluginConfig config;
    EXPECT_TRUE(plugin.initialize(config, BackendType::CPU));
}

TEST(MyPluginTest, GenerateEmbedding) {
    MyImagePlugin plugin;
    // ... test embedding generation
}
```

## Performance Benchmarks

Expected performance for reference implementation (ONNX CLIP):

| Backend | Hardware | Inference Time | Throughput |
|---------|----------|----------------|------------|
| CPU | Intel i7-12700K | ~150ms | 6-7 img/s |
| CUDA | RTX 3090 | ~30ms | 30-35 img/s |
| DirectML | RTX 3060 | ~45ms | 20-25 img/s |
| TensorRT | RTX 3090 | ~20ms | 45-50 img/s |

## Dependencies

Common dependencies for image analysis plugins:

- **ONNX Runtime**: Cross-platform inference engine
- **OpenCV**: Image processing (optional, for preprocessing)
- **stb_image**: Lightweight image loading (alternative to OpenCV)
- **nlohmann/json**: JSON configuration parsing

Install via vcpkg:

```bash
vcpkg install onnxruntime opencv4 nlohmann-json
```

## Security Considerations

1. **Plugin Verification**: All plugins should be cryptographically signed
2. **Sandboxing**: Consider running plugins in separate processes
3. **Resource Limits**: Set memory and compute time limits
4. **Input Validation**: Validate image data before processing

## Troubleshooting

### Plugin Not Loading

**Symptom**: `Failed to load plugin: themis_image_my_plugin.dll`

**Solutions**:
1. Check file exists in plugin directory
2. Verify file permissions
3. Check dependencies are installed (use `ldd` on Linux, `Dependencies.exe` on Windows)
4. Check plugin API version matches core

### Performance Issues

**Symptom**: Slow inference times

**Solutions**:
1. Use GPU backend (CUDA, DirectML) instead of CPU
2. Enable batch processing for multiple images
3. Use quantized models (INT8, FP16)
4. Pre-warm the model with dummy input

### Out of Memory

**Symptom**: `Failed to allocate memory for model`

**Solutions**:
1. Use smaller model (e.g., CLIP ViT-B/32 instead of ViT-L/14)
2. Reduce batch size
3. Enable quantization
4. Close other GPU applications

## Contributing

See [CONTRIBUTING.md](../../CONTRIBUTING.md) for guidelines on contributing new plugins.

## References

- [Image Analysis C++ Libraries](../../docs/plugins/IMAGE_ANALYSIS_CPP_LIBRARIES.md)
- [Plugin Development Guide](../../docs/development/plugin_development.md)
- [ONNX Runtime Documentation](https://onnxruntime.ai/)
- [CLIP Paper](https://arxiv.org/abs/2103.00020)
- [LLaVA Paper](https://arxiv.org/abs/2304.08485)
- [Stable Diffusion](https://github.com/Stability-AI/stablediffusion)
