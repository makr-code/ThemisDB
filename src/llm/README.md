> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# LLM Module

<!-- Status: current | validated: 2026-04-09 | Primary: src/llm/ | Secondary: docs/de/llm/ -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../docs/de/llm/README.md -->

LLM interaction storage and chain-of-thought feature implementation for ThemisDB.

## Module Purpose

Implements LLM interaction storage and chain-of-thought features for ThemisDB. Provides two inference engines: AsyncInferenceEngine (lightweight async wrapper for single LLM plugin) and InferenceEngineEnhanced (enterprise multi-model engine with context caching, batch processing, and load balancing).

## Subsystem Scope

**In scope:** Async inference request management, priority queue, context caching (KV-cache reuse), dynamic batching, multi-model load balancing, InferenceHandle for request tracking.

**Out of scope:** LLM model weights and serving (external), prompt template management (handled by prompt_engineering module), RAG pipeline orchestration (handled by rag module).

## Relevant Interfaces

- `async_inference_engine.cpp` — lightweight async inference wrapper
- `inference_engine_enhanced.cpp` — enterprise multi-model engine
- `include/llm/inference_handle.h` — async request handle
- `llm_api_handler.cpp` (server) — API integration point

## Current Delivery Status

**Maturity:** 🟢 Production-ready (v1.16.0) — Both inference engines operational; streaming SSE output, OpenAI-compatible adapter, speculative decoding, LoRA hot-loading, model quantization pipeline, and request deduplication cache are all complete.

<!-- Status: current | validated: 2026-04-09 | commit: 04e9e7d -->

## Architecture Overview

ThemisDB provides **two distinct inference engines** serving different purposes:

### 1. AsyncInferenceEngine (Simple Async Wrapper)
- **Purpose**: Lightweight async wrapper for **single** LLM plugin
- **Use Case**: Simple API endpoints, background inference tasks
- **Features**:
  - Non-blocking request submission
  - Priority queue management
  - Worker thread pool
  - Backpressure handling
- **Location**: `src/llm/async_inference_engine.cpp`
- **Usage**: Server API handlers (`src/server/llm_api_handler.cpp`)

### 2. InferenceEngineEnhanced (Enterprise Features)
- **Purpose**: Advanced multi-model engine with optimization features
- **Use Case**: RAG systems, production deployments, high-throughput scenarios
- **Features**:
  - **Context Caching**: KV-cache reuse for faster inference
  - **Batch Processing**: Dynamic batching for improved throughput
  - **Load Balancing**: Multi-model request distribution
  - **Request Queuing**: Priority scheduling with timeouts
- **Location**: `src/llm/inference_engine_enhanced.cpp`
- **Usage**: RAG integration (`src/rag/llm_integration.cpp`)

### Shared Components

#### InferenceHandle
- **Purpose**: Common handle for tracking async inference requests
- **Location**: `include/llm/inference_handle.h`
- **Features**:
  - Blocking wait for results (`get()`)
  - Non-blocking status check (`ready()`)
  - Best-effort cancellation (`cancel()`)

## Architecture Decision: Why Two Engines?

Initially, this appeared to be code duplication. Investigation revealed:

1. **Different Abstraction Levels**:
   - AsyncInferenceEngine: Simple async wrapper (single model)
   - InferenceEngineEnhanced: Enterprise orchestrator (multi-model)

2. **Different Use Cases**:
   - Simple API calls → AsyncInferenceEngine
   - Complex RAG pipelines → InferenceEngineEnhanced

3. **Minimal Overlap**:
   - Both implement worker threads (necessary for each)
   - Different queue strategies (priority vs. batch)
   - Different statistics tracking (basic vs. advanced)

## Refactoring (v1.15.0)

**Problem**: InferenceEngineEnhanced included `async_inference_engine.h` but only used `InferenceHandle`

**Solution**: Extracted `InferenceHandle` to separate header
- Created: `include/llm/inference_handle.h`
- Created: `src/llm/inference_handle.cpp`
- Removed unnecessary cross-dependency
- Both engines now depend only on shared handle

This clarifies that both engines are independent implementations serving different needs.

## Components

- LLM interaction storage
- Prompt and response tracking
- Chain-of-thought storage
- Conversation history management

### Grammar-Constrained Generation ✅ IMPLEMENTED

**Status:** Fully implemented with runtime API detection

The LLM module includes complete implementation for grammar-constrained generation (EBNF/GBNF format), which guarantees valid structured outputs. This feature uses **runtime API detection** similar to LoRA adapters.

**How It Works:**
1. On first use, the system detects if llama.cpp has grammar APIs available
2. If available: Full grammar-constrained generation is enabled
3. If not available: System falls back gracefully to unconstrained generation

**Implementation Details:**
```
"Grammar support is unavailable (llama grammar API not present)"
```

**Location:** `Grammar::compile()` in `src/llm/grammar.cpp`

**Dynamic API Loading:** `src/llm/llama_grammar_adapter.cpp`

**Required APIs from llama.cpp:**
- `llama_grammar_init()` - Compile EBNF to grammar
- `llama_grammar_free()` - Free grammar resources
- `llama_grammar_sample()` - Filter tokens by grammar rules
- `llama_grammar_accept()` - Update grammar state after token generation

**Runtime Detection:**
- Uses `themis_llama_grammar_available()` to check API availability
- Automatically activates when llama.cpp has grammar support
- Graceful fallback with informative logging if not available
- No rebuild needed when llama.cpp is updated

**Usage:**
```cpp
// Grammar support is automatically detected and used
Grammar grammar(ebnf_text, "root");
if (grammar.isValid()) {
    // Grammar APIs are available and working
} else {
    // APIs not available, will use unconstrained generation
}
```

**See Also:**
- `docs/GRAMMAR_IMPLEMENTATION_COMPLETE.md` - Full grammar documentation

## Features

- Store LLM interactions and conversations
- Track reasoning chains and intermediate steps
- Support for multi-turn conversations
- Integration with vector search for semantic retrieval

## Documentation

For LLM documentation, see:
- [LLM Architecture](ARCHITECTURE.md)
- [LLM Roadmap](ROADMAP.md)
- [Future Enhancements](FUTURE_ENHANCEMENTS.md)
- [LLM docs (English)](../../docs/llm/)

## Scientific References

1. Vaswani, A., Shazeer, N., Parmar, N., Uszkoreit, J., Jones, L., Gomez, A. N., … Polosukhin, I. (2017). **Attention Is All You Need**. *Advances in Neural Information Processing Systems (NeurIPS)*, 30, 5998–6008. https://arxiv.org/abs/1706.03762

2. Brown, T. B., Mann, B., Ryder, N., Subbiah, M., Kaplan, J., Dhariwal, P., … Amodei, D. (2020). **Language Models are Few-Shot Learners**. *Advances in Neural Information Processing Systems (NeurIPS)*, 33, 1877–1901. https://arxiv.org/abs/2005.14165

3. Wei, J., Wang, X., Schuurmans, D., Bosma, M., Ichter, B., Xia, F., … Zhou, D. (2022). **Chain-of-Thought Prompting Elicits Reasoning in Large Language Models**. *Advances in Neural Information Processing Systems (NeurIPS)*, 35. https://arxiv.org/abs/2201.11903

4. Devlin, J., Chang, M.-W., Lee, K., & Toutanova, K. (2019). **BERT: Pre-training of Deep Bidirectional Transformers for Language Understanding**. *Proceedings of NAACL-HLT 2019*, 4171–4186. https://doi.org/10.18653/v1/N19-1423

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.
