# llama.cpp Examples — Inference Insights for ThemisDB

<!-- Roadmap reference: src/llm/ROADMAP.md, src/llm/FUTURE_ENHANCEMENTS.md -->
<!-- Issue: LLM Inference Optimierungen (Batching, KV-Sharing, Lookup/Speculation) -->

This document captures the key findings from analysing the
`llama.cpp/examples/` directory and maps each insight to the ThemisDB
inference pipeline. It is the **canonical technical reference** for the
issues related to continuous batching, prefix sharing, and speculation
(lookup / draft-model) optimisations.

---

## 1 · `batched` / `parallel` — Continuous Batching

### What llama.cpp does

`examples/batched` and `examples/parallel` illustrate two styles of multi-request
inference:

| Example | Technique |
|---|---|
| `batched` | Single `llama_batch` filled with tokens from N sequences simultaneously; one forward pass evaluates all of them. |
| `parallel` | Sequences are decoded concurrently; the example tracks which sequence is complete and removes it from the active set. |

### Key design decisions

- `llama_batch_add()` assigns each token to a specific *sequence ID* and *position*.
- Logits are only required for the *last token* of each sequence (the next-token
  prediction target). Set `logits[i] = false` for non-last tokens to save memory.
- Once a sequence finishes it must be freed with `llama_kv_cache_seq_rm()` to
  release KV blocks immediately. **Not freeing causes progressive KV exhaustion.**
- `n_parallel` controls the maximum number of simultaneously decoded sequences;
  `n_batch` controls how many tokens are dispatched per forward pass.

### ThemisDB mapping

| llama.cpp concept | ThemisDB component |
|---|---|
| `llama_batch` | `ContinuousBatchScheduler` → `scheduleNextBatch()` |
| `n_batch` (token quota) | `SchedulerConfig::max_tokens_per_batch` |
| `n_parallel` | `SchedulerConfig::max_batch_size` |
| `llama_kv_cache_seq_rm()` | `PagedKVCache::removeSequence()` / `ContinuousBatchScheduler::freeKVCacheBlocks()` |
| adaptive half-batch on OOM | `enable_adaptive_batch_retry` (halves `prefill_chunk_size` on decode error) |

**Implementation guidance**:

- Before each decode, guard against KV budget exhaustion:
  check `PagedBlockManager::freeBlocks() > 0` for the required tokens.
  Refuse/retry if insufficient (→ Phase 3 n_ctx guard).
- Use `enable_adaptive_batch_retry` to halve the chunk size automatically on
  decode errors; allow gradual recovery (implemented in `v1.18.0`).

---

## 2 · `save-load-state` — KV Prefix Sharing

### What llama.cpp does

`examples/save-load-state` demonstrates that a processed prompt state can be
serialised (`llama_state_save_file`) and reloaded (`llama_state_load_file`),
allowing multiple requests to resume from a shared prefix without re-running the
prefill phase.

`examples/parallel` uses `llama_kv_cache_seq_cp()` to share tokens:

```c
// Copy the prompt's KV slot into sequence `s`
llama_kv_cache_seq_cp(ctx, 0, s, -1, -1);
// Now seq 0's KV is visible to seq s — no re-prefill needed
```

### Key design decisions

- `llama_kv_cache_seq_cp(ctx, src, dst, p0, p1)` — copy-on-write KV blocks
  for a token range.  Callers must call `seq_keep` / `seq_rm` to manage
  lifecycle.
- Sharing is only valid while the source sequence is alive. If the source is
  evicted, dependents must re-prefill.
- The system prompt (often identical across requests) is the primary candidate
  for prefix sharing: prefill once, share across all sessions.

### ThemisDB mapping

| llama.cpp concept | ThemisDB component |
|---|---|
| `llama_kv_cache_seq_cp(src, dst)` | `PagedKVCache::sharePrefix()` (CoW block sharing) |
| System-prompt prefix entry | `LLMPrefixCache` — stores embedding + precomputed KV |
| `seq_rm` / `seq_keep` | `PagedKVCache::removeSequence()` / `BlockTable` refcount |

**Implementation guidance**:

- A request whose prompt *starts with a cached system prompt* can call
  `PagedKVCache::sharePrefix(new_seq, system_seq, prefix_token_count)` to
  obtain a CoW view and skip prefill for those tokens.
- The `LLMPrefixCache` provides similarity-based lookup; the nearest hit with
  similarity ≥ `config.similarity_threshold` should be tested for prefix
  sharing eligibility.

---

## 3 · `lookup` — Prompt Lookup Decoding (n-gram Speculation)

### What llama.cpp does

`examples/lookup` implements **Prompt Lookup Decoding** (Fu et al., 2023):
instead of a draft model, it builds an n-gram index from the input prompt and
already-generated tokens. When the current context ends in a known n-gram, the
following tokens from the index are proposed as *draft* candidates. The target
model verifies them in a single forward pass, accepting or rejecting each one.

Acceptance rate is typically 40–80 % for repetitive / structured text (code,
documents, RAG contexts). For short chat turns the benefit is lower.

```c
// Build context n-gram index
struct ngram_data {
    std::vector<llama_token> tokens;           // n-gram key
    std::vector<llama_token> continuation;     // draft candidates
};
// On each step: find longest matching suffix in ctx_tokens → propose continuation
```

Three cache variants from the example:

| Variant | Description |
|---|---|
| `context` | Built from the current prompt at runtime; cleared each request |
| `static` | Pre-built from a file (e.g., a reference corpus) |
| `dynamic` | Updated from *generated* tokens; persists across steps |

### ThemisDB mapping

| llama.cpp concept | ThemisDB component |
|---|---|
| n-gram context index | `LookupDecoder` — built from `context_tokens` |
| `ngram_min` / `ngram_max` | `Config::ngram_min` / `Config::ngram_max` |
| dynamic cache persistence | `LookupDecoder::updateFromTokens()` |
| draft verification loop | `SpeculativeDecoder::verify()` (shared with draft-model path) |

**Implementation guidance**:

- `LookupDecoder::proposeDraftTokens(context, max_draft)` implements the
  suffix-match logic: iterate over `[ngram_max..ngram_min]` looking for the
  longest suffix match in the internal map.
- Enable via `InferenceEngineEnhanced::Config::enable_lookup_decoding = true`.
- Can be used *without* a draft model; mutually exclusive with full
  draft-model speculative decoding (use `enable_speculative_decoding` for that).

---

## 4 · `speculative` / `speculative-simple` — Draft-Model Speculation

### What llama.cpp does

`examples/speculative` demonstrates the full draft-model speculative decoding
loop:

1. Draft model generates K candidate tokens.
2. Target model evaluates all K+1 positions in one pass.
3. Acceptance/rejection follows the adjusted distribution
   `p'(t) = normalize(max(0, p(t) – q(t)))`.

`examples/speculative-simple` is a stripped-down version without tree
lookahead, suitable for integration.

### ThemisDB mapping

| llama.cpp concept | ThemisDB component |
|---|---|
| Draft model forward pass | `ILLMPlugin::generate()` on draft plugin |
| Target verification | `SpeculativeDecoder::verify()` |
| Per-step acceptance rate | `Statistics::speculative_avg_acceptance_rate` |
| Draft token count K | `Config::speculative_draft_tokens` |

---

## 5 · `embedding` — Prefix Cache Embedding

### What llama.cpp does

`examples/embedding` shows how to extract a text embedding from a loaded model
via `llama_get_embeddings()`. These can be used to build fast approximate
nearest-neighbour indices (e.g., HNSW) for semantic caching and prefix matching.

### ThemisDB mapping

| llama.cpp concept | ThemisDB component |
|---|---|
| `llama_get_embeddings()` | `ILLMPlugin::embed()` |
| HNSW similarity index | `LLMPrefixCache` (internal HNSW via `EmbeddingCache`) |
| Threshold tuning | `LLMPrefixCache::Config::similarity_threshold` |

---

## 6 · Metrics / Telemetry Schema

Based on the examples and the implementation plan (Issue: LLM inference
optimisations), the following Prometheus metrics are defined:

| Metric | Source | Description |
|---|---|---|
| `llm_batch_retry_count_total` | `ContinuousBatchScheduler::Stats::batch_retry_count` | Total adaptive retry downshifts |
| `llm_adaptive_prefill_chunk_size_tokens` | `Stats::adaptive_prefill_chunk_size_tokens` | Current effective chunk size |
| `llm_speculative_accept_rate` | `Statistics::speculative_avg_acceptance_rate` | Running average |
| `llm_speculative_drafted_tokens_total` | `Statistics::speculative_draft_tokens_total` | Total drafted |
| `llm_cache_miss_total` | `Statistics::cache_misses` | KV / prefix cache misses |
| `llm_lookup_accept_rate` | `LookupDecoder::Stats::acceptance_rate` | n-gram lookup hit/accept rate |

---

## 7 · Proposed Initial Parameters (from Issue)

| Parameter | Recommended start | Rationale |
|---|---|---|
| `n_parallel` | 4 | Conservative; scale to 8 after profiling |
| `n_batch` | 512 | Start below GPU occupancy limit; auto-scale via adaptive retry |
| `n_draft` (speculative) | 8–16 | Higher K → more parallelism but lower acceptance |
| `ngram_min` | 2 | Minimum for meaningful matches |
| `ngram_max` | 4 | Draft budget for lookup (matches `examples/lookup` default) |
| `similarity_threshold` | 0.95 | For prefix cache hit (tunable per workload) |

---

## 8 · Acceptance Criteria (from Issue)

- Throughput: ≥ 20 % token/s increase at 8 parallel requests vs. baseline.
- Latency: ≥ 15 % P95 improvement for mixed request profiles.
- Stability: no KV corruption, no memory leak in 24 h load test.
- Quality: no regression in deterministic replay tests (fixed seeds).
- Observability: all new metrics visible in Prometheus + Grafana.

---

## 9 · References

1. Y. Leviathan et al., "Fast Inference from Transformers via Speculative Decoding," ICML 2023. https://arxiv.org/abs/2211.17192
2. Z. Fu et al., "Break the Sequential Dependency of LLM Inference Using Lookahead Decoding," arXiv 2023. https://arxiv.org/abs/2312.11462
3. ggerganov/llama.cpp — `examples/batched`, `examples/parallel`, `examples/lookup`, `examples/speculative`. https://github.com/ggerganov/llama.cpp
4. W. Kwon et al., "Efficient Memory Management for Large Language Model Serving with PagedAttention," SOSP 2023.
