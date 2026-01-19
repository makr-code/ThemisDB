# Loading Models from ThemisDB - Usage Examples

This guide demonstrates how to use the new `loadModelFromThemisDB()` functionality to load LLM models directly from ThemisDB's blob storage.

## Overview

Models can now be stored in and loaded from ThemisDB without requiring filesystem access. This enables:
- **Native AI/LLM integration**: Models live alongside your data
- **Encrypted model storage**: End-to-end encryption with key versioning
- **Automatic blob tiering**: Small models inline, large models in external storage (S3/Azure/Filesystem)
- **Seamless deployment**: No need to manage model files separately

## Example: Basic Model Loading

```cpp
#include "llm/llama_wrapper.h"
#include "llm/llm_model_storage.h"

// Initialize LlamaWrapper
llm::LlamaWrapper::Config llama_config;
llama_config.n_gpu_layers = 32;
llama_config.n_ctx = 8192;

auto llama = std::make_shared<llm::LlamaWrapper>(llama_config);

// Load model from ThemisDB (no filesystem path needed!)
bool loaded = llama->loadModelFromThemisDB(
    "mistral-7b-instruct",  // model_id
    model_storage,           // LLMModelStorage instance
    blob_manager,            // BlobStorageManager instance
    nullptr,                 // No encryption
    {}                       // Default config
);

if (loaded) {
    // Use the model for inference
    llm::InferenceRequest request;
    request.prompt = "Explain what ThemisDB is.";
    request.max_tokens = 100;
    
    auto response = llama->generate(request);
    std::cout << response.generated_text << "\n";
}
```

## See Also

- [LLM Module Documentation](../llm/README.md)
- [Blob Storage Architecture](../storage/blob_storage.md)
