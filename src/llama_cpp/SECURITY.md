---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
Source_Level: "level1"
SOT_Domain: "module-security"
Canonical_Sources: "src/llama_cpp/llama_cpp_plugin.cpp; src/llama_cpp/llama_cpp_plugin_validation_gates.cpp; include/llama_cpp/llama_cpp_plugin.h"
---

# Security — llama_cpp Plugin

This page describes security behavior implemented by the `llama_cpp` plugin and
the responsibilities left to its callers and deployment. The live implementation
and public interface are authoritative; this document does not certify a model
artifact or replace the project [vulnerability reporting policy](../../SECURITY.md).

> Exact model backend not verifiable from final document text alone.

## Scope and trust boundaries

The plugin accepts caller-controlled prompts, RAG context, request metadata,
configuration, model paths, and LoRA payloads. When compiled with
`THEMIS_LLM_ENABLED`, it delegates model loading and inference to `LlamaWrapper`
and the linked llama.cpp runtime. The plugin does not authenticate callers,
authorize tenants, sanitize prompts, validate model provenance, or provide
output-safety filtering.

| Input or boundary | Implemented behavior | Security limit / responsibility |
|---|---|---|
| Inference and RAG requests | An optional `PolicyFn` is checked before backend dispatch in `generate()`; RAG dispatches through `generate()`. A denied request returns `success=false` and the supplied denial reason. | No policy is installed by default. This callback is an integration hook, not built-in authentication, authorization, prompt-injection defense, or tenant isolation. The serving layer must enforce identity, access policy, quotas, and output handling. |
| Model file | With `THEMIS_LLM_ENABLED`, a non-empty path is passed to `LlamaWrapper`. An opt-in digest comparison is available through `verify_model_digest` and a non-empty `expected_model_digest`. | `computeFileDigest()` uses FNV-64, not a cryptographic hash or signature. If the expected digest is missing or empty, no comparison is made. The normal `loadModel()` implementation does not call the separate exported model validator. Deploy only trusted artifacts and verify provenance outside this plugin. |
| LoRA payload | `importLoRA()` rejects payloads shorter than 8 bytes, larger than 2 GiB, or without the `GGUF` magic prefix before delegating to a live wrapper. | These are size and format checks, not authenticity or safety validation. In stub/no-wrapper mode, validated bytes return success without loading adapter weights. |
| LoRA path registration | `loadLoRA()` replaces/adds an in-memory `{id, path, scale}` record; `unloadLoRA()` removes a matching record. | Registry membership does not mean weights were loaded or activated by this plugin. Control and verify adapter loading at the actual backend boundary. |
| Stats and diagnostics | Stats include plugin/model identifiers, loaded state, LoRA count, and inference/error/retry counters; a live wrapper may add memory/runtime metadata. | Model identifiers can contain the configured path. Restrict stats and logs at the host/API layer and avoid placing prompts, secrets, or sensitive paths in routine logs. |
| Resource consumption | `generate()` checks a supplied cancellation token before dispatch. The separate validator contains a coarse GPU-memory estimate and CUDA helper. | The plugin has no request rate limiter or authentication control. The validator is not invoked by the normal `loadModel()` path; its GPU check is not hardware attestation or a capacity guarantee. Enforce admission control, resource limits, and timeouts in the serving/deployment layer. |

## Implemented safeguards and their limits

- The optional policy callback is copied under the plugin mutex and invoked
  before inference. A missing callback means no plugin-level policy decision.
- `generate()` rejects an unloaded state and a cancellation token already set
  before backend dispatch. This pre-dispatch check does not guarantee
  cancellation of work after inference starts.
- Plugin lifecycle and selected shared state use a mutex, while performance
  counters are atomic. Do not infer that every public method or backend operation
  is independently synchronized; follow the implementation's per-method locking.
- The standalone validation helper checks a non-empty model path for existence,
  at least 100 MiB, and `GGUF` magic. It also contains timeout constants and
  memory/CUDA checks, but those checks are not wired into the standard plugin
  load or inference path. A passing helper result would not establish artifact
  provenance or parser safety.
- Wrapper initialization/load exceptions in `loadModel()` are caught. The
  plugin can then set `model_loaded_` and return `true` with no live wrapper.
  Treat successful `loadModel()` or `isModelLoaded()` alone as insufficient
  evidence of usable model inference.
- In the fallback path, `embed()` may return a 384-dimensional zero vector.
  Such a vector is not a semantic embedding and must not silently enter a
  production vector index.

## Deployment requirements

1. Restrict model and adapter paths to deployment-controlled locations and
   protect artifacts and configuration through the deployment pipeline.
2. Verify artifact identity using a trusted cryptographic digest/signature
   mechanism outside the plugin; do not use FNV-64 or GGUF magic as proof of
   authenticity.
3. Install and test the caller's policy gate before routing sensitive requests.
   Keep authentication, authorization, tenant isolation, rate limiting,
   backpressure, and request timeouts at the service boundary.
4. Confirm a real-backend inference probe and embedding behavior before
   admitting traffic. Do not treat capability flags, stub output, callback
   output, or counters as model-readiness evidence.
5. Restrict access to stats and logs because model identifiers may expose local
   paths. Sanitize diagnostics before sharing them during incident response.

## Incident handling

For suspected malicious model/adapter content, unauthorized inference, unsafe
output, or resource abuse:

1. Contain requests at the serving or deployment boundary; do not bypass policy
   to restore service.
2. Preserve sanitized logs, build flags, configuration provenance, artifact
   identity, and timestamps. Limit access to any prompt or model data needed for
   investigation.
3. Compare artifacts with trusted deployment records. Magic bytes and the
   plugin's FNV-64 check do not prove authenticity.
4. Escalate authorization incidents to the policy/API owner, and artifact or
   parser concerns to security and model-runtime owners. Follow the project
   [security reporting policy](../../SECURITY.md).
5. Restore only a previously approved artifact/configuration using the
   deployment's rollback procedure. The plugin does not provide atomic
   promotion or rollback.

For symptom-oriented diagnosis and recovery boundaries, see
[`RUNBOOK.md`](RUNBOOK.md). Detailed trust-boundary and incident guidance is in
[`TRUST.md`](TRUST.md); implementation behavior is summarized in
[`IMPLEMENTATION.md`](IMPLEMENTATION.md).

## Related module documents

[`README.md`](README.md) · [`ARCHITECTURE.md`](ARCHITECTURE.md) ·
[`API.md`](API.md) · [`TRUST.md`](TRUST.md) · [`RUNBOOK.md`](RUNBOOK.md) ·
[`OPERATIONS.md`](OPERATIONS.md) · [`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md)
