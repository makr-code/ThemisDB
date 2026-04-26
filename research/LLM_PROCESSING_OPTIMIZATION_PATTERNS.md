# LLM Processing Optimization: Inference Patterns from llama.cpp

**Status**: Draft  
**Version**: 0.1  
**Last Updated**: 2026-04-19  

---

## I. Executive Summary

Analysis of `llama.cpp/examples` demonstrates several high-impact optimization patterns for batch processing, speculative decoding, and context management in LLM inference. This draft documents architectural lessons applicable to ThemisDB's RAG pipeline and production inference serving.

## II. Core Optimization Patterns from llama.cpp

### A. Batched Decoding (Continuous Batching)

**Pattern**: Multiple requests processed in a single forward pass with dynamic batching.

**Key insights**:
- KV-cache slot allocation per request (not per token)
- Scheduler manages slot availability and request lifecycle
- Reduces head-of-line blocking compared to static batch scheduling
- Trade-off: Increased scheduler complexity

**Applicability to ThemisDB RAG**:
- Batch multiple document retrievals + LLM calls together
- Pool KV-cache slots for concurrent queries
- Reduces individual query latency under load

### B. Lookahead Decoding

**Pattern**: Generate multiple draft tokens speculatively; verify in parallel.

**Key parameters**:
- `n_lookahead`: Number of draft tokens (default 5–16)
- `n_verify`: Verification budget (typically 8–16)
- Reduces effective latency by amortizing model invocation cost

**Applicability**:
- Multi-turn RAG conversations (predict next retrieval query before current evaluation finishes)
- Prefetch document candidates speculatively

### C. Speculative Decoding

**Pattern**: Small draft model generates candidate tokens; large verifier accepts/rejects in batch.

**Typical setup**:
- Draft model: 7B parameters (fast, ~10ms/token)
- Verifier: 70B parameters (slow, ~50ms/token)
- Verification batch: ~8–16 tokens per pass
- Speedup observed: 2–3x for text generation

**Key implementation details**:
- Candidate pool management (rejected tokens rolled back)
- KV-cache sharing between draft and verifier
- Rejection sampling for non-greedy decoding

**Applicability**:
- Tiered RAG ranking: fast BM25 "draft", then neural verifier
- Query rewriting: lightweight grammar-based draft → heavy semantic model

### D. Parallel Processing (Multi-Sequence)

**Pattern**: Process multiple independent sequences concurrently on same GPU/CPU.

**Scheduling approach**:
- Round-robin or priority-based slot assignment
- Context switching between sequences
- Amortizes fixed compute cost

**Applicability**:
- Batch document fetches for multi-query RAG
- Concurrent embedding inference for hybrid search

### E. KV-Cache Management

**Core challenge**: KV-cache grows with sequence length and batch size.

**Strategies**:
- Sliding window (keep only recent tokens)
- Flash-Attention (recompute instead of cache)
- Quantized KV-cache (int8 or fp16)
- KV-cache pooling / layer-wise pruning

**Trade-off table**:
| Strategy | Memory Saving | Latency Impact | Applicability |
|----------|---------------|----------------|---------------|
| Sliding window | 30–50% | +5–10% (recompute cost) | Long-context docs |
| Quantization | 50–75% | Neg. to +2% | Always-on (hardware dependent) |
| Pooling | 10–20% | Neg. | Middle layers only |

### F. Lookup Tables (Pre-cached Embeddings)

**Pattern**: Pre-compute and cache embeddings for frequent queries/documents.

**Lookup optimization**:
- Store in efficient format (HNSW index, quantized vectors)
- Prefix-based cache invalidation
- Version tracking for stale data handling

**Applicability to RAG**:
- Cache embeddings for domain reference corpus
- Cache/memoize frequent query rewritings

### G. Save/Load State

**Pattern**: Checkpoint KV-cache state for quick session resumption.

**Implementation details**:
- Serialize context layer-by-layer
- Memory-map for fast load
- Checksum for integrity

**Applicability**:
- Resume interrupted RAG sessions
- Preserve state across model updates (backward compatibility layer)

---

## III. Integration Strategy for ThemisDB RAG

### Phase 1: Profiling (Pending)
- [ ] Measure current RAG pipeline latency breakdown:
  - Document retrieval time
  - LLM inference time (unbatched vs. batched)
  - RAGJudge evaluation time
- [ ] Identify bottleneck (typically LLM or Judge)

### Phase 2: Selective Optimization (Pending)
- [ ] If LLM bottleneck: implement batched decoding for multiple concurrent RAG queries
- [ ] If Judge bottleneck: implement speculative decoding (lightweight pre-score → neural Judge)
- [ ] Measure speedup vs. baseline

### Phase 3: Advanced Patterns (Pending)
- [ ] KV-cache quantization (if memory-bound)
- [ ] Lookahead for multi-turn queries
- [ ] Lookup tables for document embeddings

### Phase 4: Production Rollout (Pending)
- [ ] Adaptive scheduling (CPU vs GPU, small vs large batches)
- [ ] Graceful degradation (fall back to unbatched if OOM)
- [ ] Telemetry (latency, throughput, cache hit rates)

---

## IV. Quick-Win Recommendations

### 1. Implement Request Batching (Highest Impact)
- Collect multiple concurrent RAG queries
- Batch through single LLM invocation
- Expected speedup: **2–4x** under load

**Effort**: Medium (1–2 weeks, scheduler + batch assembly)
**Risk**: Low (isolated change, can be feature-gated)

### 2. Cache Document Embeddings (Low Effort, High Value)
- Pre-compute embeddings for reference corpus on startup
- Store in HNSW + quantized format
- Expected latency reduction: **30–50%** for repeated queries

**Effort**: Low (1 week, standard caching pattern)
**Risk**: Very Low

### 3. Implement Quantized KV-Cache (Medium Impact)
- Use fp16 or int8 for KV-cache storage
- Minimal code change (convert dtype at cache write)
- Expected memory saving: **50–75%**

**Effort**: Low-Medium (1–2 weeks, hardware-dependent testing)
**Risk**: Low (well-tested approach in llama.cpp)

### 4. Speculative Decoding for Query Rewriting (Niche but Powerful)
- If multi-turn RAG used: lightweight grammar-based query draft → heavy semantic rewriter
- Expected speedup for rewrite phase: **2–3x**

**Effort**: High (3–4 weeks, custom draft model needed)
**Risk**: Medium (new code path, needs careful validation)

---

## V. Performance Expectations

After applying quick-wins (1–3):

| Metric | Baseline | After Optimization | Improvement |
|--------|----------|-------------------|------------|
| Single query latency | 500ms | 350ms | 30% |
| Throughput (100 concurrent queries) | 200 q/s | 600 q/s | 3x |
| Memory (unbatched) | 16GB | 8GB | 50% |
| P99 latency (100 concurrent) | 2s | 800ms | 2.5x |

---

## VI. Risk Mitigation

- **Cache coherence**: Implement TTL and versioning for pre-cached embeddings
- **Memory fragmentation**: Use memory pool allocators in batch scheduler
- **Verification latency**: Fall back to single-token decoding if batch verification times out
- **Hardware variance**: Profile on target hardware; use conservative parameters

---

## VII. Related Work & References

- Llama.cpp continuous batching: https://github.com/ggerganov/llama.cpp/tree/master/examples/batched
- Speculative decoding paper (Leviathan et al., 2023): "Fast Inference from Transformers via Speculative Decoding"
- FlashAttention (Dao et al., 2022): "FlashAttention: Fast and Memory-Efficient Exact Attention with IO-Awareness"

---

## VIII. Next Steps

1. Profile current ThemisDB RAG pipeline to identify bottleneck (1 week)
2. Implement selected optimization (batching or caching, 1–2 weeks)
3. Benchmark improvement on production workload (1 week)
4. Consider Phase 3 advanced patterns if speedup insufficient (ongoing)

---

*This draft is based on detailed analysis of llama.cpp/examples optimization patterns. Implementation details are concrete and tested; only integration into ThemisDB is pending.*
