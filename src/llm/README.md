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
- [Architecture Guide](ARCHITECTURE.md) — component diagrams, engine design, KV-cache and batching internals
- [Security Guide](SECURITY.md) — threat model, path-injection fixes (F1/F2/F3), LoRA trust model
- [Audit Report](AUDIT.md) — S0/S1/S2 findings and resolution status
- [Changelog](CHANGELOG.md) — versioned module history
- [Performance Expectations](PERFORMANCE_EXPECTATIONS.md) — release-gate benchmark targets (TTFT, LoRA hot-load, batch throughput)
- [Roadmap](ROADMAP.md) — implementation status and planned work
- [Future Enhancements](FUTURE_ENHANCEMENTS.md) — long-horizon backlog
- [Public API Headers](../../include/llm/README.md) — full header listing under `include/llm/`
- [Primary Sources (EN)](../../docs/en/llm/PRIMARY_SOURCES.md) — canonical source index
- [Primary Sources (DE)](../../docs/de/llm/PRIMARY_SOURCES.md) — kanonischer Quellenindex

## Configuration Surfaces

### Build-time flags

| Flag | Behavior |
|---|---|
| `THEMIS_ENABLE_CUDA` | Enables CUDA kernel fusion (`kernel_fusion.cu`); CPU fallback when not set |
| `THEMIS_ENABLE_KAFKA` | Not used by llm directly; consumed by downstream modules |

### Runtime configuration knobs

| Surface | Key Fields |
|---|---|
| `AsyncInferenceEngine` | Plugin registration, thread pool size, priority queue depth, timeout |
| `InferenceEngineEnhanced` | Multi-model backends, batch size, KV-cache size, load-balance policy |
| `ModelRouter` | Regex/tag routing rules, fallback model name |
| `GGUFLoader` | Model path (must be within trusted directory; see AUDIT.md F1-1 / F2-1) |
| `AdapterRegistry` | LoRA adapter base path, hot-load callback, `certificate_store` reference |
| `ActiveVRAMAllocator` | VRAM capacity, LRU eviction threshold, CPU spill ratio |
| `TokenQuotaManager` | Per-tenant token budget, refill interval |
| `OpenAICompatAdapter` | Endpoint model-name mapping, streaming mode, function-calling schema |

## Runtime Behavior, Failure Modes, and Limits

- **Path injection** (F1-1/F1-2/F2-1, fixed 2026-04-21): LoRA adapter paths and model IDs must be validated against a trusted directory before passing to libllama. See AUDIT.md and SECURITY.md.
- **Grammar-constrained generation**: gracefully falls back to unconstrained output when llama.cpp grammar APIs are absent; check `grammar.isValid()` before use.
- **Kafka** is not a runtime dependency for the llm module.
- **VRAM OOM**: `ActiveVRAMAllocator` handles OOM via LRU eviction and CPU spilling; callers receive `nullptr` on allocation failure when spill is exhausted.
- **Speculative decoding**: draft/target model pair must be on the same device; target model must have compatible vocabulary.
- **Embedded LLM server**: process-internal; not accessible outside the ThemisDB process; no network listener.

## Usage

## Scientific References

1. Vaswani, A., Shazeer, N., Parmar, N., Uszkoreit, J., Jones, L., Gomez, A. N., … Polosukhin, I. (2017). **Attention Is All You Need**. *Advances in Neural Information Processing Systems (NeurIPS)*, 30, 5998–6008. https://arxiv.org/abs/1706.03762

2. Brown, T. B., Mann, B., Ryder, N., Subbiah, M., Kaplan, J., Dhariwal, P., … Amodei, D. (2020). **Language Models are Few-Shot Learners**. *Advances in Neural Information Processing Systems (NeurIPS)*, 33, 1877–1901. https://arxiv.org/abs/2005.14165

3. Wei, J., Wang, X., Schuurmans, D., Bosma, M., Ichter, B., Xia, F., … Zhou, D. (2022). **Chain-of-Thought Prompting Elicits Reasoning in Large Language Models**. *Advances in Neural Information Processing Systems (NeurIPS)*, 35. https://arxiv.org/abs/2201.11903

4. Devlin, J., Chang, M.-W., Lee, K., & Toutanova, K. (2019). **BERT: Pre-training of Deep Bidirectional Transformers for Language Understanding**. *Proceedings of NAACL-HLT 2019*, 4171–4186. https://doi.org/10.18653/v1/N19-1423

## Public API — Key Classes (`include/llm/`)

| Class / Interface | Header | Description |
|---|---|---|
| `AsyncInferenceEngine` | `include/llm/async_inference_engine.h` | Lightweight async wrapper for single-model inference; priority queue + worker thread pool |
| `InferenceEngineEnhanced` | `include/llm/inference_engine_enhanced.h` | Enterprise multi-model engine: paged KV-cache, continuous batching, load balancing |
| `InferenceHandle` | `include/llm/inference_handle.h` | Async request handle: `get()`, `ready()`, `cancel()` |
| `ModelRouter` | `include/llm/model_router.h` | Regex- and metadata-tag-based request routing to backend models |
| `GGUFLoader` | `include/llm/gguf_loader.h` | GGUF model file validation and loading with memory-mapped tensor access |
| `EmbeddedLLM` | `include/llm/embedded_llm.h` | In-process embedded LLM server |
| `EmbeddedLLMManager` | `include/llm/embedded_llm_manager.h` | Lifecycle manager for embedded LLM instances |
| `AdapterRegistry` | `include/llm/adapter_registry.h` | Runtime LoRA adapter registration and hot-loading (`hotLoad`) |
| `AdapterLoadBalancer` | `include/llm/adapter_load_balancer.h` | Load balancing across LoRA adapter instances |
| `AdapterDeploymentManager` | `include/llm/adapter_deployment_manager.h` | LoRA adapter deployment lifecycle |
| `AdapterCompatibilityValidator` | `include/llm/adapter_compatibility.h` | Validates adapter/model compatibility before loading |
| `AIDecisionAuditor` | `include/llm/ai_decision_auditor.h` | Records and audits AI decisions for compliance |
| `AIOrchestrator` | `include/llm/ai_orchestrator.h` | Multi-model orchestration and ReAct tool-call dispatch |
| `ConstitutionalReasoningEngine` | `include/llm/constitutional_reasoning_engine.h` | Constitutional AI post-generation safety filter |
| `EthicalGuidelinesManager` | `include/llm/ethical_guidelines_manager.h` | Policy rule evaluation over generated output |
| `EthicsAwareConfidenceDetector` | `include/llm/ethics_aware_confidence_detector.h` | Confidence scoring with ethics integration |
| `ActiveVRAMAllocator` | `include/llm/active_vram_allocator.h` | GPU VRAM allocation with OOM recovery, LRU eviction, CPU spilling |
| `AdaptiveVRAMAllocator` | `include/llm/adaptive_vram_allocator.h` | Dynamic VRAM rebalancing across requests |
| `GPUMemoryManager` | `include/llm/gpu_memory_manager.h` | GPU memory lifecycle for LLM inference |
| `ContinuousBatchScheduler` | `include/llm/continuous_batch_scheduler.h` | Continuous batching scheduler for throughput |
| `GGUFConverter` | `include/llm/lora_framework/gguf_converter.h` | Direct Q4_K_M→NF4 and Q8_0→INT8 quantization conversion |
| `AQLTrainParser` | `include/llm/aql_train_parser.h` | AQL `TRAIN` statement parser |
| `DocsAssistant` | `include/llm/docs_assistant.h` | LLM-powered documentation assistant |
| `ByzantineDetector` | `include/llm/byzantine_detector.h` | Byzantine fault detection for distributed inference |
| `DistributedTrainingCoordinator` | `include/llm/distributed_training_coordinator.h` | Distributed fine-tuning coordination |

> For the full list of 172 public headers, see `include/llm/`.

## Usage

The implementation files in this module are compiled into the ThemisDB library. See [`../../include/llm/README.md`](../../include/llm/README.md) for the public API.

**Example: async inference with AsyncInferenceEngine**

```cpp
#include "llm/async_inference_engine.h"

AsyncInferenceEngine engine;
engine.registerPlugin(my_llm_plugin);

auto handle = engine.submitRequest({
    .prompt = "Explain paged attention in two sentences.",
    .max_tokens = 256,
    .priority = InferencePriority::Normal
});

// block until result ready
auto result = handle.get();
std::cout << result.text;
```

**Example: grammar-constrained generation**

```cpp
#include "llm/grammar.h"

Grammar grammar(R"(root ::= "yes" | "no")", "root");
if (grammar.isValid()) {
    // grammar APIs available: token sampling constrained to rule
} else {
    // falls back to unconstrained sampling
}
```

## Troubleshooting

- **LoRA hot-load fails with path error**: ensure the LoRA adapter file is inside the configured trusted directory; see `AdapterRegistry` configuration and AUDIT.md F1-1/F2-1.
- **Grammar returns `isValid() == false`**: llama.cpp was built without grammar API; regenerate/rebuild llama.cpp.
- **OOM on VRAM allocation**: reduce batch size or context window; check `ActiveVRAMAllocator` eviction and spill configuration.
- **OpenAI adapter returns 500**: verify `ModelRouter` routing rules cover the requested model name and that the backend plugin is loaded.
- **Speculative decoding mismatch**: draft and target models must share vocabulary; use `AdapterCompatibilityValidator` before initializing the pair.

## Installation

This module is built as part of ThemisDB.

**Build (Linux):**
```bash
cmake --preset linux-ninja-release
cmake --build --preset linux-ninja-release --target themisdb
```

**Build specific LLM targets:**
```bash
cmake --build --preset linux-ninja-release --target test_inference_engine_enhanced
cmake --build --preset linux-ninja-release --target test_model_router
```

**Run LLM tests:**
```bash
ctest --preset linux-ninja-release -R "test_inference_engine_enhanced|test_model_router|test_openai_compat_adapter|test_gguf_loader|test_grammar_integration|test_lora_security|test_active_vram_allocator|test_llm_integration"
```

See the root `CMakeLists.txt` and `CMakePresets.json` for full build configuration.
