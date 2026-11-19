---
Author: "ThemisDB Team"
Created: "2026-10-29"
Last Updated: "2026-11-17"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# LLM Module Contract

Datum: 2026-10-29
**Status:** Active  
**Module:** llm (LLM inference, model switching, context mgmt)  
**Primary:** include/llm/inference_engine.h, src/llm/ROADMAP.md

## Public API Surface

### RAG knowledge and generation boundary

Knowledge retrieval is owned by `rag::RAGProvider`, not by an LLM backend.
See [the common RAG contract](../../docs/rag/API.md). AQL and YAML consumers can
retain an existing QueryEngine with `setQueryEngine()`. Both llama.cpp wrappers
use `llm::prepareRAGRequest()` for shared sanitization, templates and estimated
context budgeting; GPU/attention configuration stays in the inference provider.
The existing `ILLMPlugin` vtable and `RAGContext` layout are unchanged.

| API | Namespace | Input Contract | Output Contract | Errors | Thread-Safety | Ownership/Lifetime | Notes |
|---|---|---|---|---|---|---|---|
| `createInferenceEngine()` | `themis::llm` | Backend type (ONNX/Llama/HF), model spec | std::unique_ptr<IInferenceEngine> | ModelNotFoundError, InitError if GPU unavailable | 🔒 Single-threaded factory call | Caller owns returned pointer; valid for lifetime of use | P0; Phase 2 complete |
| `embed()` | `themis::llm::IInferenceEngine` | Text (1-4K UTF-8), options (timeout, batch-size) | std::vector<float> (embedding, dim 384/768/1024 per model) | InputValidationError, TimeoutError, OOMError | ✅ Multi-call-thread-safe (one engine per thread recommended) | Embedding owned by return; caller may modify | P0; GATE-LLM-01 ≤100µs |
| `embedBatch()` | `themis::llm::IInferenceEngine` | Vec<string> (1-1K items), batch options | Vec<Embedding> (same order as input), may be cached | TimeoutError (batch-level timeout), InputError | ✅ Thread-safe (serialized via internal queue) | Embeddings borrowed; valid for ~1min or next call | P0; GATE-LLM-02 ≤50µs/item |
| `generateText()` | `themis::llm::IInferenceEngine` | Prompt (UTF-8, ≤8K tokens), generation config | std::string (generated text, at most max_tokens) | GenerationTimeout, ContextLimitExceeded, HallucinationFilter triggered | ✅ Thread-safe (async queue internal) | Text owned by return; immutable copy safe | P1 (slower path); defaults 30s timeout |
| `switchModel()` | `themis::llm::IModelRouter` | Target model name (registered via registry) | void (engine reloads in background, serves old model during reload) | ModelNotRegisteredError | 🔒 Single-writer per router instance | N/A (mutation); no references affected | Internal; P2 (rare admin op) |
| `getModelInfo()` | `themis::llm::IModelRouter` | Model name (or null for current) | ModelMetadata (dim, context_size, supported_tasks) | NotFoundError | ✅ Lock-free (immutable metadata) | Metadata borrowed; valid for app lifetime | P1; utility |
| `modelRegistry()` | `themis::llm` | N/A (accessor) | IModelRegistry* (borrowed, global) | None | ✅ Lock-free accessor | Global registry; append-only after init | Internal API |
| `contextManager()` | `themis::llm::IInferenceEngine` | N/A (accessor) | IContextManager* (borrowed) | None | ✅ Provides ref to context controller | Borrowed ref; valid for engine lifetime | Internal; context is stateful |
| `cancel()` | `themis::llm::IInferenceEngine` | CancellationToken (or call without param) | void (inference aborted, partial result may be available) | None | ✅ Thread-safe (signal-based) | N/A (cancellation only) | P0 (interrupt path) |

### Optional Agent Skill package API

`themis::llm::SkillRegistry` is an opt-in, read-only filesystem catalog. Call
`refresh()` to scan configured roots, then `list()` to obtain name/description
metadata. `load(name)` reads one `SKILL.md` instruction body; `readResource()`
reads a package-relative UTF-8 text file. Metadata, instructions and resources
are bounded by explicit `SkillRegistryOptions` limits. Refresh returns metadata
only, but buffered discovery may read ahead into the instruction body; it does
not guarantee zero body-byte I/O. Refresh publishes a
complete snapshot atomically; failed refresh preserves the last successful
snapshot. Path validation and opening are not atomic against concurrent
filesystem replacement: roots, package contents, and their parent directories
must not be writable by untrusted users while the registry is in use.
Initial opt-in guidance packages and host integration steps are documented in
[`config/agent_skills/README.md`](../../config/agent_skills/README.md).

`registerSkillTools()` optionally registers `list_skills`, `load_skill`, and
`read_skill_resource` in the existing `ToolRegistry`. Calls continue to pass
through its active `ModeSpec` tool allow/deny checks. Skills cannot grant
permissions, execute scripts, access database state, or commit effects. Errors
use these distinct codes: `ERR_LLM_SKILL_NOT_FOUND`,
`ERR_LLM_SKILL_INVALID_METADATA`, `ERR_LLM_SKILL_DUPLICATE_NAME`,
`ERR_LLM_SKILL_SIZE_LIMIT`, `ERR_LLM_SKILL_PATH_ESCAPE`,
`ERR_LLM_SKILL_IO_FAILED`, `ERR_LLM_SKILL_REGISTRY_NOT_INITIALIZED`, and
`ERR_LLM_SKILL_INVALID_CONTENT`.

### Optional native time series comparison

With the built-in TSStore available, call `registerTimeseriesAnalysisTool()`
from [`timeseries_analysis_tool.h`](../../include/llm/timeseries_analysis_tool.h)
and allow `compare_timeseries_windows` in the active mode. This is separate
from loading the skill package. The host fixes the collection and metric;
the entity document's primary key must match the TSStore entity ID.
This direct storage path does not run Query Engine or HTTP authorization:
authorize the entire bound collection and the corresponding series in the
bound store for the caller before registration, and keep the registry scoped
to that caller. No existing host or endpoint enables the tool by default.

The tool accepts non-negative integer epoch milliseconds for
`baseline_start_ms` and `current_start_ms`, and `duration_ms` (1 ms through
31 days). The baseline must end no later than the current window starts.
Both windows are half-open. Optional `geo: {lon, lat, radius_m}` selects
stored JSON documents with a two-coordinate GeoJSON `location` Point inside
the radius (at most 1,000 km). Missing locations are counted and skipped;
invalid locations fail the request. Without `geo`, location is not read.

The response contains per-entity sample counts, unweighted sample means,
`delta` (current minus baseline), and optional `distance_m`. Empty windows
have count zero and a null mean; a missing mean also makes the delta null.
The tool fails rather than returning truncated results beyond 64 documents
or 4,096 samples per entity and window. Geo reads also cap each document at
64 KiB. These limits are not execution deadlines or storage I/O budgets.
Reads are synchronous and not atomic across the two windows; results reflect
the samples returned by TSStore, which can skip unreadable records.

## Model Registration & Switching

| Operation | Contract | Notes |
|---|---|---|
| Register model | Done at startup via registry; immutable after | registry.register("gpt-4-turbo", spec) |
| List models | Returns Vec<ModelMetadata>, read-only | registry.listModels() |
| Switch model | Async; old model continues serving until switch ready | router.switchModel("qwen2.5") |
| Query current | Returns name of model currently serving | router.currentModel() |

## Embedding Invariants

| Invariant | Enforcement |
|---|---|
| Same text → same embedding (deterministic) | Verified by test_embedded_llm_batch_contract.cpp |
| Dimension matches model spec | Runtime check; returns ErrorDimMismatch if cached embedding has wrong dim |
| Batch order preserved | Unit test: test_llm_embedding_order_preservation.cpp |
| Batch fallback: embedBatch() → N×embed() | Verified for EmbeddedLLM stub (Phase 2 complete) |

## Concurrency & Threading Model

| Scenario | Behavior | Test |
|---|---|---|
| 8 threads calling embed() concurrently | All serve from same CUDA context via queuing; ≤100µs latency P95 | test_llm_concurrent_inference.cpp |
| generateText() + embed() simultaneous | Serialized (context not multi-callable); embed prioritized | test_llm_mixed_workload.cpp |
| Model switch during inference | In-flight requests finish with old model; new requests use new | integration test: test_llm_model_switch_during_load.cpp |

## Performance Commitments (Release Gates)

| Gate | Latency | Model | Test |
|---|---|---|---|
| GATE-LLM-01 | embed() ≤100 µs | Sentence-BERT 384D | bench_llm_inference_gates.cpp |
| GATE-LLM-02 | embedBatch() ≤50 µs/item | Via GPU batching | bench_llm_batch_inference.cpp |
| GATE-LLM-03 | generateText() ≤30s P95 | GPT-2 small (100 tokens) | bench_llm_generation.cpp |
| GATE-LLM-04 | modelSwitch prep ≤5s | Model load time + CUDA warmup | bench_llm_model_switch.cpp |

## Error Categories

| Error | When | Recovery |
|---|---|---|
| ModelNotFoundError | Model name not in registry | Check registry; register model first |
| TimeoutError | embed() exceeds timeout_ms (default 30s) | Increase timeout or use smaller batch |
| HallucinationFilterTriggered | Generated text violates safety policy | Retry with different seed/temp or different model |
| ContextLimitExceeded | Prompt + generation would exceed max_tokens | Truncate prompt or reduce max_tokens |
| OOMError | GPU/CPU memory exhausted | Reduce batch size or model dim; fallback to CPU if available |

## API Stability

| Item | Status | Stability |
|---|---|---|
| IInferenceEngine interface | Public v1.x | Frozen (breaking change = major version) |
| embed() signature | Public v1.x | Stable; may add optional params with defaults |
| Model registry format | Internal | Fluid; may change between patch versions |
| generateText() | Beta (internal) | Will finalize in Q4 2026 |

---

**Zuletzt geprueft (LLM contracts):** 2026-08-03
