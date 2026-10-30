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
Canonical_Sources: "src/llama_cpp/llama_cpp_plugin.cpp; src/llama_cpp/llama_cpp_plugin_validation_gates.cpp; tests/integration/test_llama_cpp_soak.cpp"
Related_Milestone: "DOC-WEEKLY-2026-44"
---

# llama_cpp Operations

For service owners operating the plugin in a ThemisDB deployment. This document separates module telemetry from host/deployment responsibilities and from simulated test evidence.

> Exact model backend not verifiable from final document text alone.

## Readiness and runtime signals

Before enabling traffic, verify the deployed build has the intended `THEMIS_LLM_ENABLED` setting, model path/artifact, configuration, policy integration and a successful real-backend inference probe. The plugin's `loadModel()` success and `model_loaded` field are not sufficient readiness signals: caught wrapper initialization/load failures may leave the wrapper absent while plugin state is set loaded.

Available plugin observability:

- `getPerformanceStats()`: `inference_count`, `error_count`, `stream_retry_count`.
- `getMemoryStats()`: plugin name, loaded flag, model id and LoRA registry count; wrapper-backed metadata may include memory estimates and runtime GPU offload/tensor assignment hints.
- Inference responses may contain latency, token and trace/span fields depending on the selected backend.
- Logs include validation and RAG assembly messages when the relevant logger is enabled.

No plugin-level latency histogram, queue-depth metric, rate limiter, alert, or operator action endpoint is implemented in the reviewed source. The deployment/API layer must provide admission control, service health probes, resource monitoring and alerting. Counter deltas are not request latency or model throughput.

## Capacity and benchmarking

Set `n_ctx`/`context_length`, `n_gpu_layers`, `n_batch` and `n_threads` against actual model and hardware constraints; plugin-level GPU validation estimates are not a capacity guarantee. Measure warm and cold load, request latency distribution, generated tokens/s, memory and failure behavior with a real model on representative hardware.

[`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md) lists mapped release targets, but benchmark sources/configuration and real-versus-stub mode must be checked before interpreting them. The integration soak [`../../tests/integration/test_llama_cpp_soak.cpp`](../../tests/integration/test_llama_cpp_soak.cpp) models a queue, always-success loader and acceleration fallback in local stub classes. Its 1,000 operations/s acceptance threshold is not real LLM throughput, model-load reliability, or GPU evidence.

For controlled operational test execution, see [`MODULE_GUIDE.md`](MODULE_GUIDE.md). For incident-specific diagnosis and rollback boundaries, see [`RUNBOOK.md`](RUNBOOK.md).

## Change and deployment boundary

Promote model/config/build changes only with an identified artifact, verified live-backend probe, relevant focused tests, and a deployment-owned rollback plan. The module has no atomic model promotion/rollback mechanism, and a failed replacement should not be assumed to leave an earlier wrapper active. Keep previous approved artifacts/configuration under the deployment's normal versioning and access controls.

If signals disagree (for example `model_loaded=true` but model inference fails), treat the backend as unavailable, remove or hold traffic at the caller/deployment layer, gather sanitized diagnostics, and escalate to the LLM runtime owner rather than interpreting capability flags as proof of readiness.

## Related module documents

[`RESEARCH.md`](RESEARCH.md) · [`MODELING.md`](MODELING.md) · [`EMBEDDING.md`](EMBEDDING.md) · [`IMPLEMENTATION.md`](IMPLEMENTATION.md) · [`API.md`](API.md) · [`MODULE_GUIDE.md`](MODULE_GUIDE.md) · [`RUNBOOK.md`](RUNBOOK.md) · [`TRUST.md`](TRUST.md)

Also see [`README.md`](README.md), [`ARCHITECTURE.md`](ARCHITECTURE.md), [`ROADMAP.md`](ROADMAP.md), [`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md), [`SECURITY.md`](SECURITY.md), and [`../../include/llama_cpp/README.md`](../../include/llama_cpp/README.md).
