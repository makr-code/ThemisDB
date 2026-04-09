# LLM Module Headers

<!-- Status: current | validated: 2026-04-09 | Primary: include/llm/ | Secondary: docs/de/llm/ -->
<!-- Links: ../../src/llm/README.md · FUTURE_ENHANCEMENTS.md · ../../docs/de/llm/README.md -->

This directory contains header files (.h, .hpp) for the llm module.

## Purpose

Public interfaces and declarations for LLM inference, model management, and optimization features.

## Key Components

### Inference Engines

- **`inference_handle.h`**: Shared handle for tracking async inference requests (NEW in v1.15.0)
- **`async_inference_engine.h`**: Simple async wrapper for single LLM plugin
- **`inference_engine_enhanced.h`**: Advanced multi-model engine with caching, batching, and load balancing

### Model Management

- **`llama_wrapper.h`**: Core llama.cpp plugin implementing ILLMPlugin interface
- **`llm_plugin_interface.h`**: Base interface for all LLM plugins
- **`llm_plugin_manager.h`**: Registry and lifecycle management for plugins
- **`model_loader.h`**: Model loading and validation

### Optimization Features

- **`llm_prefix_cache.h`**: Context caching for faster inference
- **`paged_kv_cache.h`**: Paged attention KV cache
- **`continuous_batch_scheduler.h`**: Dynamic batching for improved throughput

### LoRA Fine-tuning

- **`multi_lora_manager.h`**: Multi-LoRA adapter management
- **`lora_router.h`**: Automatic adapter routing

## Architecture Note

ThemisDB provides **two independent inference engines** serving different needs:

1. **AsyncInferenceEngine**: Lightweight, single-model, for simple API calls
2. **InferenceEngineEnhanced**: Enterprise-grade, multi-model, with advanced features

See `../../src/llm/README.md` for detailed architecture documentation.

## Implementation

See `../../src/llm/` for the implementation code.

## Documentation

See `../../docs/src/llm/` for detailed module documentation.
