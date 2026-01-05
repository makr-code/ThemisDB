# Vision Support Quick Start Guide

This guide shows how to use ThemisDB's vision support for multi-modal LLM inference (text + images).

## Prerequisites

1. **Build with LLM support enabled:**
   ```bash
   cmake -DTHEMIS_ENABLE_LLM=ON ..
   ```

2. **Download required models:**
   - LLaVA model (GGUF format): https://huggingface.co/cjpais/llava-v1.6-mistral-7b-gguf
   - CLIP vision encoder: `mmproj-model-f16.gguf` from same repo

## Configuration

Enable vision support in your ThemisDB configuration:

```cpp
#include "llm/llama_wrapper.h"

themis::llm::LlamaWrapper::Config config;
config.enable_vision = true;
config.clip_model_path = "/path/to/mmproj-model-f16.gguf";
config.vision_threads = 4;
config.preload_vision = true;

themis::llm::LlamaWrapper wrapper(config);
```

## Basic Usage

### Load LLaVA Model

```cpp
wrapper.loadModel("/path/to/llava-v1.6-mistral-7b.Q4_K_M.gguf");
```

### Single Image Query

```cpp
themis::llm::VisionRequest request;
request.text_prompt = "What's in this image?";
request.image_path = "/path/to/image.jpg";
request.max_tokens = 256;
request.temperature = 0.7f;

auto response = wrapper.generateVision(request);

if (response.success) {
    std::cout << "Response: " << response.text << std::endl;
    std::cout << "Tokens: " << response.tokens_generated << std::endl;
    std::cout << "Time: " << response.inference_time_ms << "ms" << std::endl;
}
```

### Multiple Images Query

```cpp
themis::llm::VisionRequest request;
request.text_prompt = "Compare these two images";
request.image_paths = {"/path/to/image1.jpg", "/path/to/image2.jpg"};
request.max_tokens = 512;

auto response = wrapper.generateVision(request);
```

## Advanced Options

### Generation Parameters

```cpp
request.max_tokens = 512;           // Maximum tokens to generate
request.temperature = 0.7f;         // Sampling temperature (0.0-1.0)
request.top_p = 0.9f;               // Nucleus sampling
request.top_k = 40;                 // Top-k sampling
request.use_image_start_end = true; // Add <image> tokens
request.image_token = "<image>";    // Custom image token
```

### Image Encoding from Memory

```cpp
// Load image into memory
std::vector<uint8_t> image_data = loadImageBytes("photo.jpg");

// Encode directly (if you have direct VisionEncoder access)
themis::llm::VisionEncoder encoder("/path/to/mmproj-model-f16.gguf");
auto embeddings = encoder.encodeImageData(image_data);
```

## Performance Characteristics

- **Latency:** +300-500ms for image encoding (GPU)
- **Memory:** +1.2GB VRAM for CLIP encoder
- **Throughput:** Can batch multiple images for efficiency

## Use Cases

### 1. Visual Question Answering
```cpp
request.text_prompt = "What color is the car in this image?";
request.image_path = "/uploads/car.jpg";
```

### 2. Image Captioning
```cpp
request.text_prompt = "Describe this image in detail";
request.image_path = "/uploads/photo.jpg";
request.max_tokens = 150;
```

### 3. Document Analysis
```cpp
request.text_prompt = "Extract the text and describe the layout of this document";
request.image_path = "/uploads/document.jpg";
request.max_tokens = 1024;
```

### 4. Comparative Analysis
```cpp
request.text_prompt = "What are the main differences between these two products?";
request.image_paths = {"/uploads/product1.jpg", "/uploads/product2.jpg"};
```

## Error Handling

```cpp
auto response = wrapper.generateVision(request);

if (!response.success) {
    std::cerr << "Vision inference failed: " << response.error_message << std::endl;
    
    // Common errors:
    // - "Vision support not enabled"
    // - "No model loaded"
    // - "Image file not found"
    // - "Failed to encode image"
}
```

## Testing

Run vision support tests:

```bash
# Unit tests
ctest -R test_llm_vision_encoder

# Integration tests (requires models)
export THEMIS_LLAVA_MODEL=/path/to/llava-model.gguf
export THEMIS_CLIP_MODEL=/path/to/mmproj-model.gguf
export THEMIS_TEST_IMAGE=/path/to/test-image.jpg
ctest -R test_llm_vision_integration
```

## Current Limitations

⚠️ **Important:** The current implementation provides the architecture and foundation for vision support. Full multi-modal inference with actual image embedding injection into the LLM context is pending complete llama.cpp integration.

**What works:**
- ✅ Image encoding with CLIP
- ✅ Vision request/response structures
- ✅ Multi-modal prompt formatting
- ✅ Integration with LlamaWrapper API

**What's pending:**
- ⏳ Full image embedding injection into llama_batch
- ⏳ True multi-modal inference (currently text-only with vision prompt)

See [VISION_SUPPORT_IMPLEMENTATION.md](../en/llm/VISION_SUPPORT_IMPLEMENTATION.md) for complete technical details.

## Troubleshooting

### "CLIP model not found"
- Verify `clip_model_path` points to valid GGUF file
- Download from: https://huggingface.co/cjpais/llava-v1.6-mistral-7b-gguf

### "Vision support not enabled"
- Ensure `config.enable_vision = true`
- Rebuild with `-DTHEMIS_ENABLE_LLM=ON`

### Out of Memory
- Reduce `config.n_gpu_layers` to offload less to GPU
- Use CPU-only CLIP: reduce image resolution

### Poor Image Quality
- Use higher resolution images (336x336 or 448x448)
- Consider multi-tile processing for very large images

## References

- [Full Implementation Guide](../en/llm/VISION_SUPPORT_IMPLEMENTATION.md)
- [LLaVA Paper](https://arxiv.org/abs/2304.08485)
- [CLIP Paper](https://arxiv.org/abs/2103.00020)
- [llama.cpp CLIP Example](https://github.com/ggerganov/llama.cpp/tree/master/examples/llava)
