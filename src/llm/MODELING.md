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

# LLM Module Modeling Notes

AI provenance note: Exact model backend not verifiable from final document text alone.

This page describes the runtime model used by the LLM module as implemented in the concrete source files under `src/llm`. It is intentionally grounded in the code rather than in product phrasing.

## Domain model

The LLM module operates around four primary concepts:

1. Model identity and lifecycle
   - `LazyModelLoader` owns the persist-and-load path for model artifacts.
   - `LLMPluginManager` chooses and owns the active backend/plugin.
   - The runtime distinguishes between a model path, a model ID, a plugin, and the actual loaded context.

2. Request shaping and evaluation
   - `EmbeddedLLM::generate()` and `generateWithParams()` build a concrete inference request from prompt text and generation settings.
   - `PromptPolicy::apply()` transforms the prompt before generation or sending it to a backend.

3. Routing and dispatch
   - `RoutingRule` expresses a route by pattern, tag, priority, and match mode.
   - `ModelRouter::route()` resolves a target model based on the prompt and metadata.

4. Runtime safety and fallback
   - Policy enforcement may reject or redact a prompt.
   - The model loader may reject or fail a checksum check before inference starts.
   - The embedding path has a deterministic fallback vector when no model context is available.

## Runtime invariants

The documented invariants in the code are explicit:

- A model load must not proceed without a valid model path and a trusted plugin.
- A prompt with an active block rule cannot pass through the safety layer.
- An invalid regex in a routing rule is rejected during configuration, not later in the request path.
- A checksum mismatch is a hard failure when integrity is required.
- A missing or failed model path is not treated as a silently successful inference.

These are operational invariants: they shape failure handling and production safety.

## Model selection and routing model

`ModelRouter` acts as a priority-ordered policy engine. Each `RoutingRule` contains:

- `id`
- `priority`
- `target_model_id`
- `prompt_patterns`
- `metadata_tags`
- `match_mode` (`ANY` or `ALL`)
- `description`

The evaluation logic in `src/llm/model_router.cpp` checks prompt regexes and metadata tags, then returns a matching route when the relevant rule criteria are met. Rules are sorted by descending priority so higher-priority routes win deterministically.

This means the module intentionally models route resolution as a small, stateless decision layer instead of a hidden condition tree embedded in the backend.

## Prompt policy model

`PromptPolicy` is a compact rule evaluation service:

- `addBlockRule()` creates a deny rule
- `addRedactRule()` creates a sanitize rule
- `apply()` returns a `PolicyResult` with `allowed`, `sanitized_prompt`, `rule_name`, and `reason`

The model is straightforward but important: the input prompt is mutated in a copy, and policy decisions are explicit. Redactions do not silently lose evidence; they track the rule that triggered and the resulting reason.

This is the foundational safety policy model in the LLM runtime.

## Embedding model

The embedding path is defined in `EmbeddedLLM::embed()` and the internal helper `buildFallbackEmbedding()` in `src/llm/embedded_llm.cpp`.

The fallback path is intentionally deterministic:

- it normalizes the input text into a 64-dim vector,
- weights positions and character values into the vector,
- normalizes the vector to unit length,
- returns a vector even when no model is loaded.

This is not a substitute for semantic quality; it is a stable shape-preserving fallback that prevents crashes and keeps callers operating under a known API contract.

## Model lifecycle model

The operational lifecycle in the module follows a strict sequence:

1. configure an `EmbeddedLLM` or backend plugin
2. validate model path and integrity metadata
3. resolve the active plugin
4. load the model into the backend
5. route requests and apply prompt policy
6. perform inference or embedding generation
7. unload or replace the model when the lifecycle changes

The direct source evidence for this is in `src/llm/embedded_llm.cpp`, `src/llm/llm_plugin_manager.cpp`, and `src/llm/model_loader.cpp`. The code aims to keep lifecycle transitions explicit and observable, not implicit.

## Safety and fallback behavior

The module's modeling follows a fail-closed posture:

- invalid config or invalid regex becomes an error, not a hidden default
- checksum verification blocks unsafe model loads
- block rules deny request execution immediately
- fallback embedding is deterministic and low-confidence, not a claim of semantic fidelity
- default plugin and route selection is deterministic and logged

This is a practical model for production workloads where operator trust matters more than permissive best-effort behavior.

## Operational consequences

This modeling matters because it gives the runtime a limited but explicit contract:

- a request is either allowed, redacted, or blocked
- a model is either valid and loaded, or it is rejected
- a route is either resolved to a target, or it fails by explicit absence
- an embedding call always returns a vector shape, even in degraded conditions

The strength of the module lies in this explicitness. It makes the runtime easier to instrument, test, and debug.
