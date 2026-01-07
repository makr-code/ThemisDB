---
name: "🟢 Implement Vision Support (Phase 3.3)"
about: Add multi-modal capabilities - Image understanding with LLaVA
title: "[P3] Implement Vision Support - Multi-Modal LLM (Text + Images)"
labels: ["priority: medium-low", "type: feature", "component: llm", "phase-3", "multi-modal"]
assignees: []
---

## Priority
🟢 **MEDIUM-LOW** - P3 (Phase 3, most complex feature)

## Overview

Implement vision support to enable **multi-modal LLMs** (text + images) using LLaVA and other vision-language models. This adds image understanding, visual Q&A, and document analysis capabilities.

**New Capabilities:**
- Visual Question Answering
- Image captioning
- Document understanding with diagrams
- Multi-modal RAG
- Integration with existing image analysis plugin

## Depends On

- ⚠️ **Blocked by Issue #1** (Fix Compilation)
- 🟢 **Optional:** Issue #2, #3 (Phase 1 & 2 - can proceed in parallel)
- 🟢 **Optional:** Issue #4, #5 (Other Phase 3 features - independent)

## Documentation

📄 **Complete implementation guide available:**
`docs/en/llm/VISION_SUPPORT_IMPLEMENTATION.md` (15.2KB)

## Feature Requirements

### Supported Models

Implement support for popular vision-language models:

1. **LLaVA** (recommended)
   - LLaVA-v1.5 and LLaVA-v1.6
   - Mistral and Vicuna variants

2. **Other Models** (optional)
   - MiniGPT-4
   - Qwen-VL
   - BakLLaVA

### Components

1. **CLIP Vision Encoder**
   - Image encoding to embeddings
   - Preprocessing (resize, normalize)

2. **Multi-Modal Projector**
   - Map image embeddings to LLM space

3. **LLaVA Decoder**
   - Process text + image for generation

## Implementation Tasks

### 1. CLIP Integration (include/llm/vision_encoder.h)

- [ ] Add CLIP dependency
  ```cmake
  # CMakeLists.txt
  find_package(clip REQUIRED)
  target_link_libraries(themisdb clip)
  ```

- [ ] Create VisionEncoder class
  ```cpp
  class VisionEncoder {
  public:
      VisionEncoder(const std::string& clip_model_path);
      ~VisionEncoder();
      
      std::vector<float> encodeImage(const std::string& image_path);
      std::vector<std::vector<float>> encodeImageBatch(
          const std::vector<std::string>& image_paths
      );
      
      int getEmbeddingDim() const;
      int getNumTokens() const;  // e.g., 576 for 24x24 patches
      
  private:
      clip_ctx* clip_ctx_ = nullptr;
  };
  ```

### 2. Image Preprocessing (src/llm/vision_encoder.cpp)

- [ ] Implement image loading
  ```cpp
  clip_image_u8 loadImage(const std::string& path);
  ```

- [ ] Implement preprocessing
  ```cpp
  clip_image_f32 preprocessImage(
      const clip_image_u8& img,
      int resolution = 336
  );
  ```

- [ ] Implement encoding
  ```cpp
  std::vector<float> encodeToEmbeddings(const clip_image_f32& img);
  ```

### 3. LlamaWrapper Vision Support (src/llm/llama_wrapper.{h,cpp})

- [ ] Add VisionEncoder member
  ```cpp
  class LlamaWrapper {
  private:
      std::unique_ptr<VisionEncoder> vision_encoder_;
      // ...
  };
  ```

- [ ] Add generateVision() method
  ```cpp
  std::string generateVision(
      const std::string& text_prompt,
      const std::string& image_path,
      const GenerationParams& params
  );
  
  std::string generateVision(
      const std::string& text_prompt,
      const std::vector<std::string>& image_paths,  // Multiple images
      const GenerationParams& params
  );
  ```

- [ ] Implement image embedding injection
  ```cpp
  void injectImageEmbeddings(
      std::vector<llama_token>& tokens,
      int position,
      const std::vector<float>& embeddings
  );
  ```

### 4. Configuration (config/)

- [ ] Add vision configuration
  ```yaml
  multimodal:
    vision:
      enabled: false  # Disabled by default
      model_type: "llava"
      clip_model_path: "/models/mmproj-model-f16.gguf"
      image_resolution: 336
      num_image_tokens: 576
      max_images_per_request: 4
      preload_clip: true
      clip_n_gpu_layers: 32
  ```

- [ ] Update llm_config.example.yaml
- [ ] Update llm_config.production.yaml

### 5. HTTP API Integration

- [ ] Add vision endpoint
  ```cpp
  POST /api/v1/llm/generate-vision
  Content-Type: multipart/form-data
  
  {
      "model": "llava-v1.6-mistral-7b",
      "prompt": "What's in this image?",
      "image": <binary data>,
      "max_tokens": 256
  }
  ```

- [ ] Support multiple images
  ```cpp
  POST /api/v1/llm/generate-vision
  {
      "prompt": "What are the differences?",
      "images": ["image1.jpg", "image2.jpg"]
  }
  ```

### 6. CLI Interface

- [ ] Add `--image` flag
  ```bash
  themisdb llm generate \
    --prompt "What's in this image?" \
    --image photo.jpg \
    --model llava-v1.6-mistral-7b
  ```

- [ ] Support multiple images
  ```bash
  themisdb llm generate \
    --prompt "Compare these images" \
    --image photo1.jpg \
    --image photo2.jpg \
    --model llava-v1.6-mistral-7b
  ```

### 7. Integration with Image Analysis Plugin

- [ ] Create unified workflow
  ```cpp
  ImageAnalysisResult analyzeImageFull(const std::string& image_path) {
      ImageAnalysisResult result;
      
      // 1. Rule-based (fast, structured)
      result.objects = detectObjectsYOLO(image_path);
      result.faces = detectFaces(image_path);
      result.text = extractTextOCR(image_path);
      
      // 2. Vision LLM (slow, semantic)
      if (vision_enabled) {
          result.caption = visionLLM->generate(
              "Describe this image", image_path
          );
      }
      
      return result;
  }
  ```

- [ ] Update image analysis config
  ```yaml
  image_analysis:
    enabled: true
    detection:
      objects: true
      faces: true
      text: true
    
    vision_llm:  # NEW
      enabled: true
      model: "llava-v1.6-mistral-7b"
      use_for:
        - captioning
        - vqa
        - scene_understanding
      fallback_to_rules: true
  ```

### 8. Model Download Scripts

- [ ] Create download script
  ```bash
  # scripts/download_vision_models.sh
  
  # LLaVA v1.6 Mistral 7B
  wget https://huggingface.co/.../llava-v1.6-mistral-7b.Q4_K_M.gguf
  wget https://huggingface.co/.../mmproj-model-f16.gguf
  
  # Verify checksums
  sha256sum -c checksums.txt
  ```

### 9. Testing

- [ ] Unit tests for VisionEncoder
- [ ] Integration tests for vision generation
- [ ] Test multiple images per request
- [ ] Test image analysis workflow
- [ ] Visual Q&A accuracy tests
- [ ] Performance benchmarks

## Expected Performance

### Latency

| Operation | Time | Notes |
|-----------|------|-------|
| Image loading | 50ms | Local file |
| CLIP encoding | 300ms | GPU |
| Projection | 100ms | |
| Inference | 2500ms | Longer due to image tokens |
| **Total** | **2950ms** | +950ms vs text-only (+47%) |

### Memory

| Component | VRAM |
|-----------|------|
| LLM (Mistral-7B) | 4.5 GB |
| CLIP encoder | 1.2 GB |
| Projection layer | 300 MB |
| Image buffers | 200 MB |
| **Total** | **6.2 GB** | +1.7 GB vs text-only (+38%) |

### Accuracy

| Task | Metric | Score |
|------|--------|-------|
| VQAv2 | Accuracy | 78.5% |
| COCO Captions | CIDEr | 113.5 |
| Visual Reasoning | Accuracy | 72.3% |

## Acceptance Criteria

### Functional
- [ ] Can load vision-enabled models (LLaVA)
- [ ] Can process single images
- [ ] Can process multiple images
- [ ] Produces correct embeddings
- [ ] Generates relevant responses
- [ ] Integrates with image analysis plugin

### Quality
- [ ] VQAv2 accuracy > 75% ✅
- [ ] Captions are descriptive and accurate ✅
- [ ] Handles various image types (photos, diagrams, charts) ✅

### Performance
- [ ] Latency overhead < 50% vs text-only ✅
- [ ] Memory overhead < 40% ✅
- [ ] Batch processing works (4+ images) ✅

### Documentation
- [ ] API documentation with examples
- [ ] Model download guide
- [ ] Integration guide
- [ ] Performance tuning guide

## Deliverables

- [ ] VisionEncoder implementation
- [ ] LlamaWrapper vision integration
- [ ] HTTP API support
- [ ] CLI support
- [ ] Image analysis plugin integration
- [ ] Model download scripts
- [ ] Test suite
- [ ] Benchmarks
- [ ] User documentation
- [ ] Example applications

## Estimated Effort

**Time:** 4-6 weeks
**Complexity:** High (CLIP integration, multi-modal prompts, testing)
**Dependencies:** Issue #1 (compilation)

## Use Cases

1. **Visual Q&A**
   - Answer questions about uploaded images
   - Document understanding

2. **Image Captioning**
   - Generate alt text for accessibility
   - Automated image descriptions

3. **Document Analysis**
   - Extract information from diagrams
   - Understand charts and graphs

4. **Multi-Modal RAG**
   - Retrieve both text and images
   - Answer based on visual context

5. **Visual Search**
   - Find similar images
   - Product matching

## Related Issues

- Depends on: #1 (Fix Compilation)
- Enhances: Existing image analysis plugin
- Related: #4 (RoPE Scaling - independent)
- Related: #5 (Grammar Constraints - independent)

## References

- Docs: `docs/en/llm/VISION_SUPPORT_IMPLEMENTATION.md`
- LLaVA paper: https://arxiv.org/abs/2304.08485
- CLIP paper: https://arxiv.org/abs/2103.00020
- llama.cpp CLIP: https://github.com/ggerganov/llama.cpp/tree/master/examples/llava
- Models: https://huggingface.co/models?other=llava

## Success Criteria

| Metric | Target | Status |
|--------|--------|--------|
| VQAv2 Accuracy | > 75% | ⏳ |
| Caption Quality (CIDEr) | > 110 | ⏳ |
| Latency Overhead | < 50% | ⏳ |
| Memory Overhead | < 40% | ⏳ |
| Multi-Image Support | 4+ images | ⏳ |

All metrics met = ✅ Feature Complete

## Model Requirements

### Recommended Model

**LLaVA-v1.6-Mistral-7B**
- Model file: llava-v1.6-mistral-7b.Q4_K_M.gguf (~4.4 GB)
- Projector: mmproj-model-f16.gguf (~1.7 GB)
- Total download: ~6.1 GB

### Download Links

```bash
# Main model
https://huggingface.co/cjpais/llava-v1.6-mistral-7b-gguf

# Projector
https://huggingface.co/cjpais/llava-v1.6-mistral-7b-gguf/mmproj-model-f16.gguf
```

## Notes

This is the **most complex Phase 3 feature** but adds unique multi-modal capabilities. Should be implemented after RoPE Scaling (#4) and Grammar Constraints (#5) are complete.

Integration with existing image analysis plugin provides strong synergy - rule-based analysis for structure, vision LLM for semantics.
