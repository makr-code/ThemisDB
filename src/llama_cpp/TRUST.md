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
Canonical_Sources: "src/llama_cpp/llama_cpp_plugin.cpp; src/llama_cpp/llama_cpp_plugin_validation_gates.cpp; include/llm/llama_wrapper.h; src/llm/llama_wrapper.cpp"
Related_Milestone: "DOC-WEEKLY-2026-44"
---

# llama_cpp Trust Boundaries

For security reviewers, integrators and operators. This is a source-grounded boundary description, not a claim that the plugin supplies complete authentication, authorization, model attestation or prompt-injection protection.

> Exact model backend not verifiable from final document text alone.

## Trust boundaries and implemented controls

| Boundary / risk | Implemented behavior | Remaining owner or limitation |
|---|---|---|
| Caller prompts, RAG documents and model output | Passed to generation/context assembly; an optional `PolicyFn` can reject generation and RAG dispatch. | The callback is unset by default and is not built-in identity authentication, authorization, prompt sanitization, output safety or tenant isolation. Integrators own policy enforcement and output handling. |
| Model files and runtime parser | `LlamaWrapper` receives the configured model path when compiled with `THEMIS_LLM_ENABLED`. A separate exported validator checks path existence, size ≥100 MiB and `GGUF` magic. | The inspected `loadModel()` path does not call that validator. Header/magic checks do not establish provenance or parser safety. The wrapper's file parser remains an attack surface; deploy trusted artifacts and keep dependencies/builds reviewed. |
| Optional model digest | Plugin's `verify_model_digest` plus `expected_model_digest` comparison fails closed on mismatch when expected value is nonempty. | `computeFileDigest()` is FNV-64, explicitly a CI-safe placeholder, not SHA-256 or cryptographic authenticity. Do not use it as a production signature or supply it from an untrusted source. |
| LoRA import bytes | Rejects payloads under 8 bytes, over 2 GiB, or without GGUF magic before possible wrapper delegation. | This is format/size screening only. No signer, digest, provenance, malware scan or semantic adapter validation is implemented here. Stub acceptance after the checks does not load weights. |
| LoRA path registration | `loadLoRA()` stores/replaces `{id,path,scale}` in a registry. | It does not load/verify weights through the wrapper in this plugin implementation; do not represent registry membership as active adapter state. |
| Resource exhaustion | Validation helper has a coarse GPU memory estimate; generation exposes request cancellation pre-check; caller can install policy. | No module-level rate limiter or authentication control. The model-size estimate is not a runtime memory guarantee, and cancellation check here is before dispatch. Callers/deployments must enforce quotas, backpressure and timeouts. |
| Diagnostics and callbacks | Stats expose operational counters and model metadata; callback exceptions are caught in generation path. | Avoid logging prompts, secrets or sensitive file locations. Stats may contain model path/id; restrict access at the host/API layer. |

## Authentication, authorization and secrets

`LlamaCppPlugin` does not authenticate callers, authorize tenants, load credentials, or perform secret/key management in the reviewed interface. `tenant_id` is a request field, not by itself an access-control boundary. `PolicyFn` is an optional hook and may be set to nullptr; callers must install and correctly maintain authorization policy before routing sensitive requests. Rate limiting and admission control belong to the serving/API layer.

Treat model and adapter paths as deployment-controlled inputs. Limit file permissions and filesystem exposure outside this module; protect model artifacts and digest/configuration sources in the deployment pipeline. Do not assume model IDs or returned stats are secret-free.

## Incident handling

For suspected malicious model/adapter, unexpected output, unauthorized inference or resource abuse:

1. Contain at the host/API/deployment boundary; stop accepting affected requests if policy permits.
2. Preserve sanitized logs, build flags, config provenance, artifact identity and incident timestamps. Restrict access to any prompt/model data required for investigation.
3. Do not rely on GGUF magic or FNV-64 as proof of authenticity. Compare against trusted artifact records/signatures maintained by the deployment.
4. Escalate artifact/parser concerns to security and model-runtime owners; authorization incidents to the policy/API owner. Follow root [`../../SECURITY.md`](../../SECURITY.md) for reporting and coordinated response.
5. Restore only a previously approved artifact/configuration using the deployment rollback process. The plugin does not provide an atomic rollback API.

See [`RUNBOOK.md`](RUNBOOK.md) for operational triage and [`SECURITY.md`](SECURITY.md) for existing module security notes, which are older and must be reconciled with this live-source description before relying on them.

## Related module documents

[`RESEARCH.md`](RESEARCH.md) · [`MODELING.md`](MODELING.md) · [`EMBEDDING.md`](EMBEDDING.md) · [`IMPLEMENTATION.md`](IMPLEMENTATION.md) · [`API.md`](API.md) · [`MODULE_GUIDE.md`](MODULE_GUIDE.md) · [`RUNBOOK.md`](RUNBOOK.md) · [`OPERATIONS.md`](OPERATIONS.md)

Also see [`README.md`](README.md), [`ARCHITECTURE.md`](ARCHITECTURE.md), [`ROADMAP.md`](ROADMAP.md), [`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md), [`SECURITY.md`](SECURITY.md), and [`../../include/llama_cpp/README.md`](../../include/llama_cpp/README.md).
