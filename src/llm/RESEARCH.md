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

# LLM Module Research Notes

AI provenance note: Exact model backend not verifiable from final document text alone.

This document captures the engineering assumptions behind the live LLM runtime in `src/llm` rather than a marketing summary. The source of truth is the implementation in:

- `include/llm/embedded_llm.h` — public embedding and generation lifecycle
- `include/llm/model_router.h` — routing and policy selection contract
- `include/llm/prompt_policy.h` — prompt safety and redaction contract
- `src/llm/embedded_llm.cpp` — runtime orchestration and fallback behavior
- `src/llm/model_loader.cpp` — model integrity/loading safety checks
- `src/llm/llm_plugin_manager.cpp` — plugin lifecycle and default selection
- `src/llm/llama_wrapper.cpp` — backend integration and fail-closed validation

## Why this module exists

The LLM subsystem exists to make inference portable, observable, and safe under production pressure. The design is shaped by concrete runtime needs: a model may fail to load, a prompt may violate a policy, a backend may be unavailable, and requests may need concurrency, routing, or adapter hot-swapping without creating undefined behavior.

The module is therefore not just a wrapper around llama.cpp. It adds operational boundaries around model lifecycle, prompt policy, routing rules, adapter selection, and safe fallback. These behaviors are visible in the code rather than only in documentation.

## Research questions the implementation addresses

### 1. Can inference be configured without hidden global state?

`EmbeddedLLM::Config` in `include/llm/embedded_llm.h` packages the key runtime parameters into a single configuration object: model path, GPU layer count, context size, batching, thread count, and optional ethical guidance. The default constructor delegates to a typed config, and `EmbeddedLLM` owns the concrete `LlamaWrapper` instance.

This is important because the module keeps model runtime settings explicit and local to an object instance rather than depending on hidden ambient process state.

### 2. Can model loading fail safely and traceably?

`LazyModelLoader::verifyModelChecksum()` in `src/llm/model_loader.cpp` validates the model digest when an expected checksum is configured. If the checksum is missing and integrity is required, loading is aborted. If the digest mismatches, the load fails before inference starts. This is a fail-closed design, not a permissive fallback.

The same pattern exists in `LlamaWrapper::loadModel()`: invalid loader state, null model/context handles, and missing model conditions are treated as runtime errors instead of silent success.

### 3. Can prompt policies be enforced before execution?

`PromptPolicy` in `include/llm/prompt_policy.h` and `src/llm/prompt_policy.cpp` models allow/block and redact rules as regex-based policy objects. Rules are compiled up front, and `apply()` evaluates the sanitized prompt. In block mode, the first match rejects the request immediately; in redact mode, it strips sensitive content while preserving execution.

This transforms policy enforcement from ad hoc string matching into a consistent, testable safety contract.

### 4. Can model selection adapt without a monolithic switch statement?

`ModelRouter` in `include/llm/model_router.h` and `src/llm/model_router.cpp` supports priority-ordered `RoutingRule` entries, regex prompt matching, metadata tags, and `ANY`/`ALL` match semantics. Route selection is explicit and thread-safe via a mutex-protected rule state.

This design is intentionally narrow: routing is deterministic, auditable, and independent from backend execution.

### 5. Can backend lifecycle be managed across multiple plugins?

`LLMPluginManager` in `include/llm/llm_plugin_manager.h` and `src/llm/llm_plugin_manager.cpp` owns plugin registration, deregistration, default selection, and model loading dispatch. It keeps a default plugin and authenticates model loads against a valid plugin path before invoking the underlying backend.

This makes backend choice and model load behavior composable while keeping the decision path visible to operators.

## Research assumptions and constraints

- The module assumes that model files, backends, and runtime resources can fail at any time; it therefore prefers explicit errors and bounded fallback rather than magical recovery.
- The implementation treats prompt redaction and policy rejection as a first-class boundary. It does not assume the application layer can sanitize everything before it reaches the LLM runtime.
- Embeddings are treated as a distinct runtime capability rather than an optional afterthought. `EmbeddedLLM::embed()` and `buildFallbackEmbedding()` show the intended fallback/no-model path deliberately returns a deterministic vector rather than a null or stale value.
- The module expects concurrency, route configuration, and adapter decisions to be stable enough for operator review. This is why the router and plugin manager guard the state with mutexes and log the activity.

## Trade-offs and failure modes

- Safety and flexibility are intentionally balanced around explicit policy decisions. A hidden relaxation of safety checks would be a regression, not a convenience feature.
- Embedded fallback paths are deterministic but not equivalent to a real model. The fallback embedding is designed to preserve API shape and limited semantics without pretending to provide model quality.
- Model loading and runtime selection are more conservative than a permissive auto-loader. When the system cannot establish a trustworthy model state, it fails rather than continuing with partial confidence.

## Evaluation and validation path

The LLM design is supported by the code paths under `src/llm` and exercised by the module-level validation, benchmark, and release artifacts already present in the repo. The most directly relevant evidence is:

- `src/llm/model_loader.cpp` for integrity verification and load safety
- `src/llm/prompt_policy.cpp` for redact/block behavior
- `src/llm/model_router.cpp` for deterministic route selection
- `src/llm/llm_plugin_manager.cpp` for backend lifecycle and fail-closed selection
- `src/llm/embedded_llm.cpp` for generation, chat, and embedding behavior

## Conclusion

The LLM module is best understood as a safe runtime boundary around model execution, not simply a library wrapper. The core research assumption is that model quality, production safety, and operational clarity must be maintained together; if a component cannot establish trust, the correct behavior is to fail explicitly and log the reason.
