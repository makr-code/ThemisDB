---
Author: "ThemisDB Team"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Team"
---

# LLM Module Implementation Notes

AI provenance note: Exact model backend not verifiable from final document text alone.

This document describes how the implementation is organized in the live LLM module. Its content is grounded in actual sources under `src/llm` and the public contracts in `include/llm`.

## Implementation scope

The LLM module is centered on a small number of core responsibilities:

- model lifecycle (`model_loader.cpp`, `llm_plugin_manager.cpp`, `llama_wrapper.cpp`)
- inference and generation (`embedded_llm.cpp`, `llama_wrapper.cpp`)
- prompt safety (`prompt_policy.cpp`, `llm_security_utils.cpp`)
- routing decisions (`model_router.cpp`)
- adapter and load balancing (`lora_router.cpp`)
- cache and runtime state (`llm_response_cache.cpp`, `llm_prefix_cache.cpp`, `kv_cache_buffer.cpp`)

## Core design pattern

The module favors explicit object ownership and narrow runtime responsibilities:

- `EmbeddedLLM` owns the primary runtime configuration and a `LlamaWrapper`
- `LLMPluginManager` owns plugin registration and default plugin selection
- `ModelRouter` owns route configuration and route matching
- `PromptPolicy` owns regex-based safety rules
- `LazyModelLoader` owns file validation and checksum enforcement

This design keeps state transitions traceable and minimizes hidden global runtime state.

## Lifecycle and failure handling

The call chain shown by the code is intentionally conservative:

1. configure model path and runtime settings
2. validate model integrity if checksum metadata exists
3. resolve plugin/backends with `LLMPluginManager`
4. load model into the active wrapper
5. enforce prompt policy before generation is attempted
6. return a concrete result or a fail-closed error path

Concrete evidence:

- `LazyModelLoader::verifyModelChecksum()` rejects mismatches or missing checksum when required
- `LLMPluginManager::loadModel()` validates model path input and plugin existence before loading
- `PromptPolicy::apply()` rejects block-rule matches immediately and redacts other patterns
- `EmbeddedLLM` falls back to a deterministic embedding vector when the model path is unavailable or insufficient

## Implementation details that matter

### Model loading

`model_loader.cpp` handles checksum normalization, integrity enforcement, and a cautious load path. The code distinguishes between:

- `require_model_integrity` being set
- expected checksum present or absent
- explicit error logging and phase-specific failure

This is a strong operational signal that the runtime assumes integrity failures should stop execution.

### Prompt safety

`prompt_policy.cpp` compiles regex rules once and then uses them repeatedly. This is simple but important: policy errors surface during configuration time rather than during request time. That improves debuggability and avoids partial runtime misconfiguration.

### Routing

`model_router.cpp` keeps route sorting and matching deterministic. It validates required id/target info and rejects empty rule definitions. The implementation keeps route thread-safety by locking the rule list while adding or removing rules.

### Embeddings

The embedding path in `embedded_llm.cpp` uses a thread-safe cache map with `cache_mutex_` and a deterministic fallback vector generator. This prevents brittle null states and provides a stable shape even when model loading is unavailable.

## Edge cases the implementation is explicit about

- invalid regex in a routing or policy rule
- missing or invalid model checksum
- zero or missing plugin registration
- empty or invalid prompt policy rule definitions
- no default plugin during load dispatch
- model or context pointer null after load attempts

The code treats these as real failure conditions rather than default states to hide behind.

## Why this implementation is production-oriented

The module is designed to expose operational truth rather than hide complexity. The public interfaces are small, state is localized to owning objects, and failure paths are visible through logs and status/error semantics.

The production-oriented design is visible in several places:

- explicit logging in plugin registration and model load failures
- deterministic route priority ordering
- checksum validation before model execution
- prompt redaction and block decisions with reason strings
- degraded embedding output as a defined fallback, not an implicit bug

## Conclusion

The implementation is deliberately explicit about runtime contracts, failure conditions, and safe degraded behavior. It is not a permissive or “best effort” wrapper; it is structured to be explainable, observable, and safe under real deployment conditions.
