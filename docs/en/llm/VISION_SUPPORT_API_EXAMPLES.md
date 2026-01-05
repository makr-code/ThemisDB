# Vision Support API Examples

Complete examples showing how to integrate vision support into your applications.

## Table of Contents
- [C++ API Examples](#c-api-examples)
- [HTTP API Examples](#http-api-examples)
- [Error Handling](#error-handling)
- [Best Practices](#best-practices)

## C++ API Examples

### Example 1: Simple Image Description

```cpp
#include "llm/llama_wrapper.h"
#include <iostream>

int main() {
    // Configure LlamaWrapper with vision support
    themis::llm::LlamaWrapper::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 4096;
    config.enable_vision = true;
    config.clip_model_path = "/models/mmproj-model-f16.gguf";
    
    // Create wrapper
    themis::llm::LlamaWrapper wrapper(config);
    
    // Load LLaVA model
    if (!wrapper.loadModel("/models/llava-v1.6-mistral-7b.Q4_K_M.gguf")) {
        std::cerr << "Failed to load model" << std::endl;
        return 1;
    }
    
    // Prepare vision request
    themis::llm::VisionRequest request;
    request.text_prompt = "Describe this image in detail";
    request.image_path = "/path/to/image.jpg";
    request.max_tokens = 256;
    request.temperature = 0.7f;
    
    // Generate response
    auto response = wrapper.generateVision(request);
    
    // Print results
    if (response.success) {
        std::cout << "Description: " << response.text << std::endl;
        std::cout << "Tokens: " << response.tokens_generated << std::endl;
        std::cout << "Inference time: " << response.inference_time_ms << "ms" << std::endl;
        std::cout << "Image encoding: " << response.image_encoding_time_ms << "ms" << std::endl;
    } else {
        std::cerr << "Error: " << response.error_message << std::endl;
        return 1;
    }
    
    return 0;
}
```

### Example 2: Multi-Image Comparison

```cpp
#include "llm/llama_wrapper.h"
#include <vector>
#include <string>

void compareImages(themis::llm::LlamaWrapper& wrapper,
                   const std::vector<std::string>& image_paths,
                   const std::string& question) {
    themis::llm::VisionRequest request;
    request.text_prompt = question;
    request.image_paths = image_paths;
    request.max_tokens = 512;
    request.temperature = 0.7f;
    
    auto response = wrapper.generateVision(request);
    
    if (response.success) {
        std::cout << "Analysis:" << std::endl;
        std::cout << response.text << std::endl;
        std::cout << std::endl;
        std::cout << "Processing time: " << response.inference_time_ms << "ms" << std::endl;
    } else {
        std::cerr << "Comparison failed: " << response.error_message << std::endl;
    }
}

int main() {
    // Setup (omitted for brevity - see Example 1)
    themis::llm::LlamaWrapper wrapper(config);
    wrapper.loadModel("/models/llava-model.gguf");
    
    // Compare multiple images
    std::vector<std::string> products = {
        "/uploads/product_a.jpg",
        "/uploads/product_b.jpg"
    };
    
    compareImages(wrapper, products, "What are the key differences between these products?");
    
    return 0;
}
```

### Example 3: Visual Question Answering with Fallback

```cpp
#include "llm/llama_wrapper.h"
#include <optional>

std::optional<std::string> askAboutImage(
    themis::llm::LlamaWrapper& wrapper,
    const std::string& image_path,
    const std::string& question,
    int max_retries = 3
) {
    themis::llm::VisionRequest request;
    request.text_prompt = question;
    request.image_path = image_path;
    request.max_tokens = 256;
    request.temperature = 0.7f;
    
    for (int attempt = 0; attempt < max_retries; ++attempt) {
        auto response = wrapper.generateVision(request);
        
        if (response.success) {
            return response.text;
        }
        
        std::cerr << "Attempt " << (attempt + 1) << " failed: " 
                  << response.error_message << std::endl;
        
        // Wait before retry
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return std::nullopt;
}

int main() {
    // Setup wrapper...
    themis::llm::LlamaWrapper wrapper(config);
    wrapper.loadModel("/models/llava-model.gguf");
    
    // Ask question with retry logic
    auto answer = askAboutImage(
        wrapper,
        "/uploads/medical_scan.jpg",
        "What abnormalities do you see in this scan?"
    );
    
    if (answer) {
        std::cout << "Answer: " << *answer << std::endl;
    } else {
        std::cerr << "Failed after all retries" << std::endl;
    }
    
    return 0;
}
```

### Example 4: Batch Processing Multiple Images

```cpp
#include "llm/llama_wrapper.h"
#include <vector>
#include <filesystem>

struct ImageAnalysis {
    std::string filename;
    std::string description;
    int64_t processing_time_ms;
};

std::vector<ImageAnalysis> analyzeDirectory(
    themis::llm::LlamaWrapper& wrapper,
    const std::string& directory_path
) {
    std::vector<ImageAnalysis> results;
    
    for (const auto& entry : std::filesystem::directory_iterator(directory_path)) {
        if (!entry.is_regular_file()) continue;
        
        std::string ext = entry.path().extension().string();
        if (ext != ".jpg" && ext != ".jpeg" && ext != ".png") continue;
        
        themis::llm::VisionRequest request;
        request.text_prompt = "Describe this image concisely";
        request.image_path = entry.path().string();
        request.max_tokens = 100;
        request.temperature = 0.5f;
        
        auto response = wrapper.generateVision(request);
        
        ImageAnalysis analysis;
        analysis.filename = entry.path().filename().string();
        analysis.description = response.success ? response.text : "Error: " + response.error_message;
        analysis.processing_time_ms = response.inference_time_ms;
        
        results.push_back(analysis);
        
        std::cout << "Processed: " << analysis.filename 
                  << " (" << analysis.processing_time_ms << "ms)" << std::endl;
    }
    
    return results;
}

int main() {
    // Setup...
    themis::llm::LlamaWrapper wrapper(config);
    wrapper.loadModel("/models/llava-model.gguf");
    
    // Process all images in a directory
    auto results = analyzeDirectory(wrapper, "/data/images");
    
    // Print summary
    std::cout << "\n=== Analysis Summary ===" << std::endl;
    for (const auto& result : results) {
        std::cout << result.filename << ": " << result.description << std::endl;
    }
    
    return 0;
}
```

## HTTP API Examples

### Example 5: REST API Endpoint

```cpp
// HTTP server handler for vision requests
#include "llm/llama_wrapper.h"
#include "server/http_server.h"  // Your HTTP framework
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class VisionHandler {
public:
    VisionHandler(themis::llm::LlamaWrapper& wrapper) : wrapper_(wrapper) {}
    
    // POST /api/v1/vision/analyze
    json handleVisionRequest(const json& request_json) {
        json response;
        
        try {
            // Parse request
            themis::llm::VisionRequest request;
            request.text_prompt = request_json.at("prompt").get<std::string>();
            request.max_tokens = request_json.value("max_tokens", 256);
            request.temperature = request_json.value("temperature", 0.7f);
            
            // Handle image (can be path or base64 data)
            if (request_json.contains("image_path")) {
                request.image_path = request_json["image_path"].get<std::string>();
            } else if (request_json.contains("image_data")) {
                // Handle base64 image data
                // (implementation omitted for brevity)
            }
            
            // Generate response
            auto vision_response = wrapper_.generateVision(request);
            
            // Build JSON response
            response["success"] = vision_response.success;
            response["text"] = vision_response.text;
            response["tokens_generated"] = vision_response.tokens_generated;
            response["inference_time_ms"] = vision_response.inference_time_ms;
            response["image_encoding_time_ms"] = vision_response.image_encoding_time_ms;
            response["model"] = vision_response.model_name;
            
            if (!vision_response.success) {
                response["error"] = vision_response.error_message;
            }
            
        } catch (const std::exception& e) {
            response["success"] = false;
            response["error"] = e.what();
        }
        
        return response;
    }
    
private:
    themis::llm::LlamaWrapper& wrapper_;
};

// Usage in HTTP server:
// POST /api/v1/vision/analyze
// {
//   "prompt": "What's in this image?",
//   "image_path": "/uploads/photo.jpg",
//   "max_tokens": 256,
//   "temperature": 0.7
// }
```

## Error Handling

### Example 6: Comprehensive Error Handling

```cpp
#include "llm/llama_wrapper.h"
#include <iostream>
#include <exception>

enum class VisionErrorType {
    SUCCESS,
    MODEL_NOT_LOADED,
    VISION_NOT_ENABLED,
    IMAGE_NOT_FOUND,
    ENCODING_FAILED,
    INFERENCE_FAILED,
    UNKNOWN
};

struct VisionResult {
    VisionErrorType error_type;
    std::string text;
    std::string error_message;
};

VisionResult safeVisionInference(
    themis::llm::LlamaWrapper& wrapper,
    const themis::llm::VisionRequest& request
) {
    VisionResult result;
    result.error_type = VisionErrorType::UNKNOWN;
    
    try {
        // Check if model is loaded
        if (!wrapper.isModelLoaded()) {
            result.error_type = VisionErrorType::MODEL_NOT_LOADED;
            result.error_message = "LLM model not loaded";
            return result;
        }
        
        // Check if image exists
        if (!std::filesystem::exists(request.image_path)) {
            result.error_type = VisionErrorType::IMAGE_NOT_FOUND;
            result.error_message = "Image file not found: " + request.image_path;
            return result;
        }
        
        // Perform inference
        auto response = wrapper.generateVision(request);
        
        if (response.success) {
            result.error_type = VisionErrorType::SUCCESS;
            result.text = response.text;
        } else {
            // Categorize error
            if (response.error_message.find("not enabled") != std::string::npos) {
                result.error_type = VisionErrorType::VISION_NOT_ENABLED;
            } else if (response.error_message.find("encode") != std::string::npos) {
                result.error_type = VisionErrorType::ENCODING_FAILED;
            } else {
                result.error_type = VisionErrorType::INFERENCE_FAILED;
            }
            result.error_message = response.error_message;
        }
        
    } catch (const std::exception& e) {
        result.error_type = VisionErrorType::UNKNOWN;
        result.error_message = std::string("Exception: ") + e.what();
    }
    
    return result;
}

int main() {
    themis::llm::LlamaWrapper wrapper(config);
    wrapper.loadModel("/models/llava-model.gguf");
    
    themis::llm::VisionRequest request;
    request.text_prompt = "Describe this image";
    request.image_path = "/uploads/photo.jpg";
    
    auto result = safeVisionInference(wrapper, request);
    
    switch (result.error_type) {
        case VisionErrorType::SUCCESS:
            std::cout << "Success: " << result.text << std::endl;
            break;
        case VisionErrorType::MODEL_NOT_LOADED:
            std::cerr << "Error: Model not loaded" << std::endl;
            break;
        case VisionErrorType::VISION_NOT_ENABLED:
            std::cerr << "Error: Vision support not enabled" << std::endl;
            break;
        case VisionErrorType::IMAGE_NOT_FOUND:
            std::cerr << "Error: Image not found" << std::endl;
            break;
        case VisionErrorType::ENCODING_FAILED:
            std::cerr << "Error: Image encoding failed" << std::endl;
            break;
        case VisionErrorType::INFERENCE_FAILED:
            std::cerr << "Error: Inference failed" << std::endl;
            break;
        default:
            std::cerr << "Unknown error: " << result.error_message << std::endl;
    }
    
    return 0;
}
```

## Best Practices

### 1. Reuse Wrapper Instances
```cpp
// ✅ Good: Reuse wrapper for multiple requests
themis::llm::LlamaWrapper wrapper(config);
wrapper.loadModel("/models/llava-model.gguf");

for (const auto& image : images) {
    auto response = wrapper.generateVision(request);
    // Process response...
}

// ❌ Bad: Creating new wrapper for each request
for (const auto& image : images) {
    themis::llm::LlamaWrapper wrapper(config);  // Expensive!
    wrapper.loadModel("/models/llava-model.gguf");  // Very expensive!
    // ...
}
```

### 2. Pre-validate Images
```cpp
// ✅ Good: Check before inference
if (!std::filesystem::exists(image_path)) {
    std::cerr << "Image not found" << std::endl;
    return;
}

auto file_size = std::filesystem::file_size(image_path);
if (file_size > 10 * 1024 * 1024) {  // 10MB limit
    std::cerr << "Image too large" << std::endl;
    return;
}

auto response = wrapper.generateVision(request);
```

### 3. Use Appropriate Temperature
```cpp
// For factual/deterministic responses
request.temperature = 0.1f;  // Low temperature

// For creative descriptions
request.temperature = 0.9f;  // High temperature

// General purpose
request.temperature = 0.7f;  // Balanced
```

### 4. Set Reasonable Token Limits
```cpp
// Short captions
request.max_tokens = 50;

// Detailed descriptions
request.max_tokens = 256;

// Document analysis
request.max_tokens = 1024;
```

### 5. Monitor Performance
```cpp
auto response = wrapper.generateVision(request);

// Log performance metrics
if (response.success) {
    logger.info("Vision inference completed",
        {"tokens", response.tokens_generated},
        {"total_time_ms", response.inference_time_ms},
        {"encoding_time_ms", response.image_encoding_time_ms},
        {"generation_time_ms", response.inference_time_ms - response.image_encoding_time_ms}
    );
}
```

## See Also

- [Vision Support Quick Start](VISION_SUPPORT_QUICK_START.md)
- [Full Implementation Guide](VISION_SUPPORT_IMPLEMENTATION.md)
- [LlamaWrapper API Reference](../../api/llm/llama_wrapper.md)
