# LLM Module

LLM interaction storage and chain-of-thought feature implementation for ThemisDB.

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
- `docs/LLM_IMPLEMENTATION_COMPLETE.md` - LLM implementation status (100%)
- `docs/LLM_CORE_STATUS_MASTER.md` - Master status document

## Features

- Store LLM interactions and conversations
- Track reasoning chains and intermediate steps
- Support for multi-turn conversations
- Integration with vector search for semantic retrieval

## Documentation

For LLM documentation, see:
- [LLM Interaction Store](../../docs/src/llm/llm_interaction_store.cpp.md)
- [Chain of Thought Storage](../../docs/chain_of_thought_storage.md)
