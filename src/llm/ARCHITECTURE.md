---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown; Claude (WP H snapshot provenance section)"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References:
  - "README.md"
  - "API.md"
  - "../../include/llm/embedded_llm.h"
  - "../../include/llm/async_inference_engine.h"
  - "../../include/llm/llm_plugin_interface.h"
  - "../../include/llm/model_router.h"
  - "../../include/llm/prompt_policy.h"
  - "../../include/llm/streaming_handler.h"
  - "../../include/llm/lora_framework/training_snapshot.h"
  - "../../include/llm/lora_framework/adapter_provenance.h"
  - "../../include/storage/mvcc_snapshot_pins.h"
  - "embedded_llm.cpp"
  - "async_inference_engine.cpp"
  - "llm_plugin_manager.cpp"
  - "../server/llm_api_handler.cpp"
Related_Issue: "#7167 (closes #7091)"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# LLM Module Architecture

## Scope, boundaries, and ownership

`src/llm/` owns model-backed inference runtime surfaces: embedded and asynchronous requests, model/backend selection, prompt policy, streaming response helpers, model/adapter lifecycle, and inference resource support. The module does not own client protocol endpoints, search ranking, or durable database storage.

## Components and public interfaces

| Component | Public interface | Source-backed responsibility |
|---|---|---|
| Embedded inference | [`EmbeddedLLM`](../../include/llm/embedded_llm.h) | Configure model/runtime access and expose generation/embedding callbacks and methods. |
| Async execution | [`AsyncInferenceEngine`](../../include/llm/async_inference_engine.h), [`InferenceHandle`](../../include/llm/inference_handle.h) | Submit/manage asynchronous requests and represent request completion/cancellation state. |
| Backend and routing | [`ILLMPlugin`](../../include/llm/llm_plugin_interface.h), [`LLMPluginManager`](../../include/llm/llm_plugin_manager.h), [`ModelRouter`](../../include/llm/model_router.h) | Register/select backend plugins and route model requests. |
| Prompt and stream formatting | [`PromptPolicy`](../../include/llm/prompt_policy.h), [`StreamingHandler`](../../include/llm/streaming_handler.h) | Apply prompt rules; helper formats SSE/chunk strings and adapts a callback to a supplied sink. |
| Embedded stream callbacks | [`EmbeddedLLM`](../../include/llm/embedded_llm.h) | Pass generated text through callbacks and optionally format callback values as SSE. |
| Runtime resources | [`SharedWorkerPool`](../../include/llm/shared_worker_pool.h), [`TokenQuotaManager`](../../include/llm/token_quota_manager.h), [`GPU memory manager`](../../include/llm/gpu_memory_manager.h) | Schedule work and expose token/GPU resource controls. |

## Request and event flow

1. A caller submits through an embedded or asynchronous inference interface.
2. Where wired, prompt policy and routing determine whether and where the request proceeds; async execution uses worker-pool and request-management components.
3. The selected engine/plugin performs inference. `EmbeddedLLM::generateStreaming` supplies a callback to the inference request; `generateStreamingSSE` formats callback values through `LlamaWrapper::formatStreamTokenAsSSE`.
4. Backend status and request completion are returned through the corresponding synchronous result, handle, or callback.

See [`embedded_llm.cpp`](embedded_llm.cpp), [`async_inference_engine.cpp`](async_inference_engine.cpp), [`llm_plugin_manager.cpp`](llm_plugin_manager.cpp), and [`model_router.cpp`](model_router.cpp). The sequence is configurable; not every request traverses every listed component.

The HTTP route `GET /api/v1/llm/stream` is owned by
`LLMApiHandler::handleStreamInference`. It collects SSE events in a string and
returns a `http::string_body` response after generation completes; it does not
use `StreamingHandler` or write each event to the network as generated. The
route currently accepts `prompt`, `request_id`, and `max_tokens` query
parameters; `max_tokens` defaults to 512 and accepted values are capped at
4096. No dedicated streaming configuration or transport cancellation/
backpressure contract was verified.

The same handler exposes OpenAI-compatible `POST /v1/chat/completions`
streaming when the request body sets `stream: true`. It assigns an inference
callback that appends OpenAI-compatible SSE chunks, then returns the buffered
complete response with a final chunk and `[DONE]`; inference exceptions return
JSON errors. This path also does not write chunks incrementally to the network.
`AsyncInferenceEngine` cancellation/deadline controls apply to requests
submitted through that engine; these synchronous HTTP streaming paths do not
submit through it.

## Failure domains and edge cases

- Invalid prompt-rule regex/configuration is rejected when rules are installed.
- Missing model/plugin, model-load errors, and backend inference errors are distinct from successful empty output.
- Async queue, deadline, cancellation, and resource exhaustion affect request completion; callers must inspect the result/status.
- `EmbeddedLLM` contains a fallback embedding helper; that behavior does not imply a general fallback for text generation or backend failures.

## LoRA training-data snapshot provenance (WP H)

Every LoRA/AdaLoRA adapter trained through
`LoRAOrchestrator::createAdapterFromSnapshot()` or
`LoRAOrchestrator::regenerateAdapter()` carries a signed, hash-only
reference to the exact MVCC snapshot of its training data. The training data
itself is never written into adapter metadata.

- **Snapshot source and guarantee**
  (`include/llm/lora_framework/training_snapshot.h`).
  - Records live in `MVCCStore` under `lora_ds:<collection>:<record_key>`.
    Each value is a JSON sample (`input`/`output`/`context`); an empty value
    is a tombstone.
  - `TrainingSnapshotReader::capture()` takes one fresh HLC tick T, pins T,
    then reads every key at T: the newest version with `ts <= T`, in
    ascending key order, up to `limit` records.
  - AQL reads (`LoRADatasetLoader::load`) see live data and cannot be pinned
    to T, so they are not used for snapshot-bound jobs.
  - A concurrent `MVCCStore::put` whose timestamp is ≤ T but whose write
    lands after the read is **not prevented**. Before the adapter is signed,
    the orchestrator re-verifies the snapshot, so such a write fails the job
    instead of producing a wrong reference.
- **Fingerprint.**
  - Merkle root (SHA-256) over leaves
    `0x00‖len‖collection‖len‖key‖u64be(version_ts)‖sha256(value bytes)`,
    sorted by key. Inner nodes are `SHA256(0x01‖L‖R)`; an odd node is
    promoted unchanged.
  - The fingerprint also includes `record_count`, the canonical selector JSON
    and its SHA-256, `split_seed` and `holdout_fraction`.
- **GC retention** (`include/storage/mvcc_snapshot_pins.h`).
  - `MVCCSnapshotPinRegistry` stores leases durably under
    `__mvcc_snapshot_pin__:<lease>`.
  - When the registry is attached through
    `MVCCStore::setSnapshotPinRegistry()` (the HTTP server does this), all
    GC paths keep the version visible at every pinned timestamp:
    `gcVersionsBefore`/`gcAllBefore`, `MvccCleanupHandler`,
    `MVCCChainPruner::pruneKey`.
  - Victim selection (`MVCCStore::selectGcVictims`) runs under the registry's
    shared GC guard. `acquire()` takes that guard exclusively, so a pin and a
    concurrent GC cannot race.
  - A pin cannot bring back versions that were already pruned.
  - The lease of a successful job is kept and recorded in the artifact
    (`snapshot_lease_id`); `releaseSnapshotPin()` frees it.
- **Metadata binding** (`include/llm/lora_framework/adapter_provenance.h`).
  - The `themis.training.snapshot.*` keys are `collection`, `hlc_ts`,
    `query`, `query_sha256`, `record_count`, `merkle_root`, `split_seed`,
    `holdout_fraction` and `pinned`.
  - These keys are written to the safetensors `__metadata__`, then signed in
    C++.
  - The `themis.provenance.*` keys are `canonical_json`, `canonical_sha256`,
    `signature` (hex), `key_id` and `alg`. The canonical JSON covers every
    `themis.*` key except `themis.provenance.*`, including
    `themis.adapter.sha256_tensors`, as a string map with sorted keys and no
    whitespace.
  - The GGUF is produced afterwards by the existing converter, which passes
    all `themis.*` keys through. The canonical payload is embedded because
    GGUF stores some keys as u32/f32.
- **Verification.**
  - Safetensors import verifies the signature and the tensor hash
    (`AdapterSignaturePolicy::require_signed_imports`).
  - GGUF hot-load checks the signature and that the GGUF KV exactly matches
    the payload, via `MultiLoRAManager::setAdapterMetadataVerifier`
    (`require_signed_hot_load`).
  - All policy flags default to `true` (fail closed). An invalid signature is
    always rejected.
  - `regenerateAdapter(source, {base_model, ...})` first verifies the source
    adapter. It then re-reads the data at `hlc_ts` under a new pin and
    recomputes the Merkle root. It aborts **before training** with
    `SnapshotUnavailable` (data pruned) or `FingerprintMismatch`, and has no
    fallback.
- **Known gaps.**
  - GGUF tensor bytes are not covered by the signature; only the safetensors
    tensor hash is signed, and the manifest records `gguf_sha256`.
  - Adapters trained from inline/AQL datasets carry no snapshot reference.

## Explicit non-goals

- Owning HTTP/gRPC/WebSocket endpoint behavior.
- Owning search fusion/ranking or RAG retrieval candidate generation.
- Owning durable storage semantics for state/cache backends.
- Guaranteeing GPU availability, distributed inference, or a specific performance envelope for every build.

## Verified cross-module references

| Boundary | Evidence and link |
|---|---|
| Search reranking | Search exposes an injected LLM reranker/backend interface; see [search architecture](../search/ARCHITECTURE.md) and [`search/llm_reranker.h`](../../include/search/llm_reranker.h). |
| Storage-backed state | LLM sources include storage interfaces for selected persisted state/model paths; see [storage architecture](../storage/ARCHITECTURE.md). |
| Query integration | Query has LLM/LoRA function interfaces; this is a caller integration, not ownership of the query engine. See [query architecture](../query/ARCHITECTURE.md). |

## References

- [Module README](README.md) · [API contract](API.md) · [module roadmap](ROADMAP.md)
- [`llm_api_contract.h`](../../include/llm/llm_api_contract.h) · [`embedded_llm.h`](../../include/llm/embedded_llm.h) · [`llm_plugin_interface.h`](../../include/llm/llm_plugin_interface.h)
