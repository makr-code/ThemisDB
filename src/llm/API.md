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

# LLM Module API Contract

AI provenance note: Exact model backend not verifiable from final document text alone.

This page summarizes the module-level API contract visible in the public LLM headers and the concrete implementation files under `src/llm`.

## Public API surface

The canonical public interfaces are defined in:

- `include/llm/embedded_llm.h`
- `include/llm/model_router.h`
- `include/llm/prompt_policy.h`
- `include/llm/llm_plugin_manager.h`

## `EmbeddedLLM`

`EmbeddedLLM` is the primary entry point for generation and embedding use cases.

Core responsibilities:

- initialize or configure a runtime wrapper around the loaded model
- generate text with a simple or parameterized API
- handle chat formatting and multi-turn message exchange
- generate embeddings and batched embeddings
- expose streaming and SSE generation hooks
- manage a custom generate callback and custom embedding callback
- store a deterministic low-confidence fallback embedding when a real model is not available

Key config fields in `EmbeddedLLM::Config`:

- `model_path`
- `model_id`
- `n_gpu_layers`
- `n_ctx`
- `n_batch`
- `n_threads`
- `enable_caching`
- `enable_streaming`
- `enable_ethical_guidelines`
- `ethical_guidelines_config`

Important semantics:

- the class does not silently hide model load and backend errors;
- it keeps generation and embed callbacks explicit;
- the runtime is designed to stay operational under a degraded configuration instead of crashing immediately.

## `PromptPolicy`

`PromptPolicy` is the prompt-filtering boundary. Its contract is intentionally narrow and explicit:

- `addBlockRule(name, pattern)` rejects matching prompts
- `addRedactRule(name, pattern, replacement)` replaces matching content
- `removeRule(name)` removes a rule
- `ruleCount()` exposes the active count
- `apply(prompt)` returns `PolicyResult`

The `PolicyResult` object contains:

- `allowed`
- `sanitized_prompt`
- `rule_name`
- `reason`

This contract ensures that prompt-safety checks remain visible and testable.

## `ModelRouter`

`ModelRouter` is the model-selection boundary. The contract is defined by `RoutingRule` and `RoutingResult`:

- `RoutingRule` contains the route id, priority, target model, pattern list, metadata tags, and match mode
- `addRule()` validates input before storing the compiled regexes
- `removeRule()` removes by id
- `route(prompt, metadata)` resolves a matching model target
- `ruleCount()` exposes the configured count

`MatchMode::ANY` and `MatchMode::ALL` make the matching semantics explicit. The code ensures invalid route definitions fail at rule configuration time rather than later during request processing.

## `LLMPluginManager`

`LLMPluginManager` owns backend lifecycle and default plugin selection.

Core capabilities:

- `registerPlugin(name, plugin)`
- `unregisterPlugin(name)`
- `getPlugin(name)`
- `getDefaultPlugin()`
- `setDefaultPlugin(name)`
- `listPlugins()`
- `hasPlugin(name)`
- `loadModel(model_id, path)`
- `unloadModel(model_id)`

This API is designed to keep backend selection and model load state explicit. It is also the place where fallback choice between plugin backends is made visible to the rest of the module.

## Contract invariants

The module expects the following invariants to hold:

- rule IDs and target model IDs cannot be empty in the routing layer
- policy rules must compile successfully before they are installed
- a missing/invalid model path is not treated as success
- plugin registration should not accept a null plugin instance
- the embedding path must return a vector shape even when fallback is used

## Failure semantics

The public semantics are intentionally fail-closed:

- invalid rule configuration leads to `std::invalid_argument`
- policy match leads to explicit `PolicyResult::allowed == false`
- plugin lookup failure is surfaced as a missing plugin or invalid default behavior
- model checksum mismatch or invalid load state blocks a model from entering the inference path

## Conclusion

The LLM API is not a broad and ambiguous interface. It is a small, explicit runtime contract centered on generation, prompt safety, routing, plugin selection, and safe degraded behavior. This is the correct shape for a production-oriented inference module.
