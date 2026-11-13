---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-11-13"
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

## Database-native hybrid direction (design target; 2026-11-13)

ThemisDB prioritizes smaller local models and decentralized processing near
the data. Its llama.cpp integration is native inference, not merely a remote
provider adapter. This direction does not restrict every deployment to a fixed
parameter count or assume that a smaller model always matches a larger one.
Task quality must be measured for the selected model, quantization, hardware,
retrieval corpus, and context budget.

**Current evidence:** [`LlamaWrapper`](../../include/llm/llama_wrapper.h)
invokes llama.cpp and performs model-vocabulary tokenization;
[`generateRAG()`](llama_wrapper.cpp) uses the shared
[`prepareRAGRequest()`](../../include/llm/rag_helpers.h).
[`AgenticRAG`](../../include/rag/agentic_rag.h),
[`ReActAgent`](../../include/aql/aql_agent.h), and
[`AIOrchestrator`](../../include/llm/ai_orchestrator.h) already expose iterative
retrieval/tool/mode surfaces. Their presence does not establish uniform hard
budgets, safe execution of arbitrary tools, durable orchestration, or isolation.
RAG packing still uses estimated tokens; see the
[current retrieval/generation contract](../../docs/rag/API.md).

The following allocation is a proposal, not newly implemented behavior:

| Layer | Responsibility and authority |
|---|---|
| Database execution | QueryEngine/storage own plans, reads, indexes, authorization integration, and transaction commits. A model-generated query or tool call is untrusted input. |
| Native intelligence execution | Local llama.cpp generation/embeddings, prompt assembly, approved model/LoRA selection, and bounded retrieval/refinement/tool loops may run with the database. All consume explicit shared resource budgets. |
| Optional process/application coordination | Long-lived jobs, approval waits, cross-service effects, and recovery need their own durable state/authorization contract. They may be integrated or external; neither placement grants database privileges. |
| Framework interoperability | Python/.NET adapters expose supported operations without importing LangChain or another framework into core builds. |

### Proposed request lifecycle and safety boundaries

1. Authenticate and authorize at the owning boundary; resolve tenant, permitted
   collections/tools, locality, model/adapter, and finite request/session budgets.
2. Execute typed retrieval through QueryEngine and the common RAG provider;
   preserve source identity, revision and any supported read-consistency scope.
3. Rank/deduplicate evidence and budget the fully rendered prompt using the
   selected generation model's tokenizer, including templates, tools/history,
   output reservation and a safety margin.
4. Perform local inference. A bounded controller may propose another retrieval,
   select an allowlisted tool, or select an approved local model/LoRA; the
   trusted executor validates schema, arguments, permissions and remaining
   aggregate budget before each operation.
5. Return attributable output, an explicit insufficient-evidence outcome, or
   an error. A model/judge score is not a proof of factual correctness.
   Generated writes and external effects require a separately authorized,
   validated operation; generation itself is not a database commit.

Do not hold transaction locks across generation or human approval. Where
snapshot pinning is supported, bound its lifetime and report stale/expired
evidence; do not infer a global cross-shard snapshot from independent reads.
LLM planning supplies candidate requests, not a replacement for the trusted
query optimizer or authorization checks.

### Locality, lifecycle, and failure domains

- Local model switching is an existing architectural concern, not something
  categorically excluded from the database. Remote/node routing requires an
  explicit deployment policy; local failure must not silently trigger cloud
  egress or a larger external model.
- Embedding encoder, generation model/tokenizer, and LoRA version are distinct.
  A generation-only change requires re-budgeting and cache/quality review,
  not automatically re-embedding an unchanged index.
- Admission control must cover query work, prompt prefill/decode, retries,
  iterative tools, and background training/indexing across CPU, RAM and VRAM.
  Source-level queue/quota helpers do not prove mixed-workload isolation.
- Inference unavailability must not disable ordinary database operations.
  An embedded shared-process backend still shares a crash/address-space
  failure domain; process isolation would require an explicit worker design.

Detailed acceptance work is in [the LLM backlog](FUTURE_ENHANCEMENTS.md) and
[the context-efficiency design](../rag/ARCHITECTURE.md). These proposals change
no existing API, plugin ABI, transaction guarantee, or deployment default.

### AdaLoRA, Multi-LoRA and tensor layers in the hybrid

These are architectural pillars, not incidental model-selection options:

1. **Retrieval supplies current evidence.** Source-backed facts, permissions
   and revisions stay in the database; an adapter is not a replacement for
   fresh, authorized retrieval.
2. **AdaLoRA specializes a frozen local base.** The current
   [native pipeline](../../include/training/adalora_training_pipeline.h)
   consumes bounded normalized samples, trains a block-sharded `output.weight`
   update from frozen-model hidden states/logits, exports GGUF and can request
   loading through the wrapper. This does not establish full-transformer
   fine-tuning, general domain-quality gains or signed deployment.
3. **Multi-LoRA serves approved specializations.**
   [MultiLoRAManager](../../include/llm/multi_lora_manager.h) manages adapter
   lifecycle and has actual llama.cpp load/apply calls. Multiple resident
   adapters, selecting one per request, applying several weighted updates to
   one context, and creating a fused artifact are different capabilities.
   The existing request field names one adapter; broader composition must
   be qualified per backend rather than inferred from the manager name.
4. **Tensor abstraction organizes and reduces artifacts/candidates.**
   [TensorMidLayer](../../include/tensor/tensor_mid_layer.h) handles
   adapter/package/fingerprint/shard scopes;
   [AdaLoraTTBridge](../../include/training/adalora_tt_bridge.h) exposes
   adapter-to-TT conversion, and
   [TensorRAGPipeline](../../include/rag/tensor_rag_pipeline.h) exposes
   retrieval-gate and layer handoffs. None alone proves direct compressed
   tensor execution in llama.cpp or expands its usable semantic context.

The proposed lifecycle is authorized samples -> bounded training -> evaluated,
versioned artifact -> explicit promotion -> request-pinned adapter/model ->
retrieval/context preparation -> inference -> monitored rollback if required.
Bind the base-model digest, tokenizer, target tensors/shapes, adapter revision,
rank/scaling and any composition weights; tensor artifacts additionally bind
layout/dtype/decomposition profile and approximation error.

Freeze adapter identity for a request or explicitly define a safe transition.
Mid-generation switching cannot assume old KV/prefix states remain valid:
invalidate/re-prefill or demonstrate backend-specific equivalence. Pin loaded
artifacts until in-flight use ends, reject incompatible/revoked adapters, and
partition caches by the actual model/adapter composition. Required-adapter
application failure is not permission to silently use the base model.
**Observed path-specific exception:** `MultiLoRAManager::batchInference()`
currently logs a failed `applyLoRA()` and continues with base-model inference;
the response still records the requested adapter ID and can be marked
successful ([implementation](multi_lora_manager.cpp#L1157)). Do not treat this
batch path as enforcing the required-adapter rule until its failure semantics
are corrected and covered by a focused test.

Training runs separately from latency-sensitive request threads and competes
under shared admission budgets; the current pipeline requires a dedicated
wrapper, not concurrent inference on its training context. Promotion, signing
and authorization differ by training path: the snapshot-provenance flow below
must not be generalized to all AdaLoRA exports.

Use the [training plan](../training/FUTURE_ENHANCEMENTS.md) and
[tensor plan](../tensor/FUTURE_ENHANCEMENTS.md) for missing end-to-end gates.
RAG, adapters and tensor representations are complementary; compare them in
controlled ablations instead of assuming their gains add up.

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
