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
Canonical_Sources: "src/llama_cpp/llama_cpp_plugin.cpp; src/llama_cpp/llama_cpp_plugin_validation_gates.cpp; src/llama_cpp/llama_cpp_registrar.cpp; tests/integration/test_llama_cpp_soak.cpp"
Related_Milestone: "DOC-WEEKLY-2026-44"
---

# llama_cpp Incident Runbook

For an operator or on-call engineer diagnosing this module. These steps use only implemented diagnostics and caller/deployment actions; no module-level restart command or alert threshold is assumed.

> Exact model backend not verifiable from final document text alone.

## First response

1. Record deployment/build flags, configured `llm.model_path`, model artifact identity, recent deployment/config changes, request trace/span IDs and error text. Do not place prompts or model contents in routine incident logs.
2. Query plugin `getMemoryStats()` and `getPerformanceStats()` if the host exposes them. The plugin returns `model_loaded`, `model_id`, `lora_count`, optional wrapper memory/runtime metadata, and monotonic inference/error/stream-retry counters. It does not expose latency histograms.
3. Review process logs around load, validation, CUDA, RAG and inference errors. The validator uses `THEMIS_ERROR/WARN/INFO`; plugin RAG paths log assembled context counts/budget. Logging availability depends on build configuration.
4. Reproduce in a controlled environment with the same build mode/config and a non-sensitive test prompt. Use focused tests only to isolate contract paths; they do not prove production model readiness.

## Symptoms and recovery

| Symptom | Diagnosis | Recovery and boundary |
|---|---|---|
| Generation reports “model not loaded” or returns backend error | Confirm `loadModel()`/registrar config, `THEMIS_LLM_ENABLED`, path readability, artifact and wrapper load logs. `loadModel()==true` can coexist with no live wrapper after a caught load failure. | Correct the configured model path/build or restore the last known-good model/config and reinitialize through the normal owner. Do not treat `isModelLoaded()` alone as proof. |
| Output resembles `[stub:…]` or generation fails without model | Check `THEMIS_LLAMA_CPP_STUB_MODE`, wrapper availability, and whether `GenerateFn` was intentionally injected. | Disable test-only stub use in production build/deployment; use a verified LLM-enabled build/model. Do not use stub output for user-facing inference. |
| Embeddings are empty, all zeros, or incompatible in dimension | Empty means plugin not marked loaded. For loaded state inspect real wrapper/model capability; plugin catches wrapper errors, then tries `EmbedFn`, then returns 384 zeros. | Restore/verify an embedding-capable wrapper/model or a controlled embedding callback. Before continuing indexing, confirm vector dimension and reject zero/fallback output. Avoid mixing dimensions in an index. See [`EMBEDDING.md`](EMBEDDING.md). |
| Requests denied | Inspect the installed `PolicyFn` owner and its denial reason. A missing policy callback means this plugin adds no authorization decision. | Correct upstream policy/config only after authorized review. Do not bypass the caller's policy to clear an incident. |
| LoRA import rejected | Check minimum 8-byte size, 2 GiB maximum and leading `GGUF` bytes. | Re-stage a trusted, correctly identified adapter. The byte checks are not authenticity validation; see [`TRUST.md`](TRUST.md). |
| RAG output is truncated or lacks context | Check `RAGContext` document count, context override, response budget, request token limit, and `rag_mode`/slot metadata. | Adjust caller budgets or retrieved inputs, then re-run with a bounded test. The plugin assembles context; it does not certify retrieval relevance. |
| CUDA requested but CPU path used | Check compile flags, `THEMIS_CUDA_DISABLED=1`, warnings, and wrapper runtime metadata if present. The validation helper's compiled-in device check is a placeholder. | Use CPU operation only if its capacity is acceptable; otherwise move to a verified compatible build/host. Do not infer GPU availability from `n_gpu_layers` or validator success. |

## Safe rollback and escalation

Rollback means restoring the prior known-good model artifact and deployment configuration through the deployment system, then reinitializing the plugin using its owning server/plugin manager. The module does not provide an atomic deployment rollback API. `loadModel()` replaces its state under a mutex and may clear/fail the wrapper before returning a fallback-loaded state; do not assume a failed replacement preserves the previous active model. If the previous artifact/config cannot be proven safe, keep inference disabled and escalate to the model/runtime owner.

Escalate with sanitized logs, build flags, model identity/checksum source, config keys (redact sensitive paths if required), stats snapshots and exact reproduction/test results. Escalate policy denials to the policy/API owner, model-loader errors to the LLM/runtime owner, and suspicious model/adapter artifacts to security response.

## Related module documents

[`RESEARCH.md`](RESEARCH.md) · [`MODELING.md`](MODELING.md) · [`EMBEDDING.md`](EMBEDDING.md) · [`IMPLEMENTATION.md`](IMPLEMENTATION.md) · [`API.md`](API.md) · [`MODULE_GUIDE.md`](MODULE_GUIDE.md) · [`OPERATIONS.md`](OPERATIONS.md) · [`TRUST.md`](TRUST.md)

Also see [`README.md`](README.md), [`ARCHITECTURE.md`](ARCHITECTURE.md), [`ROADMAP.md`](ROADMAP.md), [`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md), [`SECURITY.md`](SECURITY.md), and [`../../include/llama_cpp/README.md`](../../include/llama_cpp/README.md).
