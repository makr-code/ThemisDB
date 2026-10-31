---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_Sources: "src/llama_cpp/llama_cpp_plugin.cpp; src/llm/llama_wrapper.cpp; include/llm/llama_wrapper.h; include/llm/llm_plugin_interface.h"
Related_Milestone: "DOC-WEEKLY-2026-44"
---

# llama_cpp Embedding Path

This is the implementation and evaluation guide for maintainers using `LlamaCppPlugin::embed()`. It distinguishes backend vectors from test fallbacks; it reports no embedding-quality result.

> Exact model backend not verifiable from final document text alone.

## Actual method and outputs

When the plugin's loaded state is false, `embed(text)` returns an empty vector. With `THEMIS_LLM_ENABLED` and a non-null wrapper, it calls `LlamaWrapper::embed(text)`. The wrapper checks that a model and context exist, tokenizes the input, evaluates a llama batch with `llama_decode`, reads `llama_get_embeddings`, uses `llama_model_n_embd` for the vector dimension, and L2-normalizes nonzero vectors. Wrapper exceptions are caught by the plugin.

If wrapper embedding fails or is unavailable, the plugin tries an injected `EmbedFn`. A non-empty callback result is returned as supplied; an empty result or thrown exception falls through. The final fallback is a 384-element all-zero vector. This is a deterministic contract fallback, not a semantic embedding. The plugin does not validate callback vector dimension, finite values or normalization.

`LlamaWrapper::Config::enable_embeddings` defaults to false in [`../../include/llm/llama_wrapper.h`](../../include/llm/llama_wrapper.h), and the plugin's `loadModel()` does not set it while constructing the wrapper config. Because wrapper embedding requires embeddings to be available from the active model/context, verify the real backend configuration rather than assuming `supports_embeddings` means the path is operational.

## Evaluation and reproducibility

No embedding model, vector dimension, accuracy score or benchmark result is claimed here. For a real-backend evaluation:

1. Record build flags, model artifact checksum/identity, model metadata, embedding configuration and deployment hardware.
2. Run a fixed, versioned set of input texts; record whether each result is real, injected or fallback. Current `embed()` does not expose a backend-origin flag, so arrange an external diagnostic or test harness that can distinguish the path.
3. Check nonempty output, consistent dimension, finite values, and L2 norm (allowing a zero vector only when explicitly testing zero handling).
4. Measure latency and throughput under a fixed workload; report repeated-run distributions and warm/cold conditions.
5. For semantic utility, evaluate a documented labeled retrieval dataset with a selected metric such as Recall@k or MRR. Include corpus construction and query splits; do not infer quality from vector shape or normalization.

The module focused test suite is linked from [`RESEARCH.md`](RESEARCH.md). The llama_cpp soak test is simulation-only, and its operation-rate assertion is not embedding throughput evidence. The repository performance target mapping is in [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md); confirm that the mapped benchmark really exercises the plugin and active model before using it.

## Failure and edge cases

- A not-loaded plugin returns an empty vector.
- A loaded plugin with no active wrapper and no usable injected callback returns 384 zeros. This behavior can produce plausible-shaped but semantically invalid vectors; callers must reject/identify it for production indexing.
- A wrapper exception does not propagate through this plugin's `embed()`; it falls through to callback or zeros.
- A callback returning a dimension different from previously indexed vectors is accepted unchanged and may cause downstream vector-store incompatibility.
- `getCapabilities().supports_embeddings` is always true, including configurations that fall back.

## Related module documents

[`RESEARCH.md`](RESEARCH.md) · [`MODELING.md`](MODELING.md) · [`IMPLEMENTATION.md`](IMPLEMENTATION.md) · [`API.md`](API.md) · [`MODULE_GUIDE.md`](MODULE_GUIDE.md) · [`RUNBOOK.md`](RUNBOOK.md) · [`OPERATIONS.md`](OPERATIONS.md) · [`TRUST.md`](TRUST.md)

Also see [`README.md`](README.md), [`ARCHITECTURE.md`](ARCHITECTURE.md), [`ROADMAP.md`](ROADMAP.md), [`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md), [`SECURITY.md`](SECURITY.md), and [`../../include/llama_cpp/README.md`](../../include/llama_cpp/README.md).
