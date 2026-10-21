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

# LLM Module Runbook

AI provenance note: Exact model backend not verifiable from final document text alone.

This runbook covers daily operational diagnosis and recovery for the LLM module. It is grounded in the runtime code in `src/llm`, especially `embedded_llm.cpp`, `model_loader.cpp`, `llm_plugin_manager.cpp`, and `model_router.cpp`.

## Primary symptoms

Common issues include:

- model load failures or checksum mismatches
- the active plugin cannot be resolved
- a prompt is blocked or unexpectedly redacted
- routing selects an unexpected model path
- embedding generation returns degraded lower-confidence output
- conversation or generation fails under a degraded backend configuration

## Triage flow

### 1. Confirm model and plugin state

Check:

- whether a model file exists at the configured path
- whether the model checksum metadata matches the installed file
- whether a default LLM plugin is registered
- whether the chosen plugin can actually load the intended model

The implementation in `LLMPluginManager::loadModel()` and `LazyModelLoader::verifyModelChecksum()` makes these the first checks to validate.

### 2. Inspect prompt-policy state

If the request is blocked or sanitized unexpectedly, inspect:

- the configured `PromptPolicy` rules
- whether a regex block rule is matching the request
- whether the redaction rule is changing a dangerous or sensitive sequence

`PromptPolicy::apply()` is the fast path to verify the actual decision.

### 3. Confirm route selection

If the wrong backend or model is selected, inspect:

- `ModelRouter` rules
- `priority` ordering
- prompt regexes and metadata tags
- whether route selection is using `ANY` or `ALL` semantics

This is where wrong model selection usually becomes visible.

### 4. Check degraded runtime paths

If generation still fails after plugins and routes are valid, check whether the system is in a degraded mode:

- fallback embedding vector generation
- backend not fully initialized
- load path or config mismatch
- model loaded without required integrity metadata

This is the next layer of diagnosis when the runtime is partly available but not fully healthy.

## Recovery actions

### Safe recovery for a model-load issue

1. confirm the configured model path is valid and readable
2. confirm the plugin exists and is defaulted or explicitly selected
3. verify checksum policy and model metadata
4. reload or replace the incorrect model artifact
5. re-run a focused generation or embedding test

### Safe recovery for a prompt-policy issue

1. identify the offending rule
2. verify whether the pattern is too broad or the request is legitimately unsafe
3. narrow or adjust the rule scope in a controlled change
4. validate the sanitized or blocked outcome against a small test vector

### Safe recovery for route mis-selection

1. inspect route ordering and priority values
2. tighten the regex conditions or metadata tags
3. verify the target model ID actually exists in the active plugin set
4. confirm the operator's expected request class matches the route logic

## Escalation triggers

Escalate to a maintainer if:

- model integrity checks fail repeatedly on valid files
- a plugin cannot be registered or selected under normal startup
- prompt safety rules unexpectedly block legitimate operational traffic
- the same route configuration causes repeated incorrect model choice
- degraded embedding fallback is being used in a production path that expects model quality

## Operational notes

The LLM module treats degraded behavior as real and diagnosable rather than as a hidden success. When in doubt, the correct operator action is to confirm the model, the plugin, and the policy boundary before changing business logic.

## Conclusion

The runbook should emphasize explicit diagnosis. The LLM runtime is safest when operators can trace a failure to a single boundary: model integrity, plugin selection, route matching, or prompt policy enforcement.
