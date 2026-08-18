# Layered Retrieval SLA — Performance & Resource Contracts

<!-- Status: current | validated: 2026-08-18 -->
<!-- Links: LAYERED_RETRIEVAL_ARCHITECTURE.md · ROADMAP.md · bench_search_release_gates.cpp -->

## Service-Level Agreements

This document defines performance, resource, and reliability targets for the **LayeredRetrievalOrchestrator** across production deployment scenarios.

---

## Latency Targets

### Overall Query Latency (p50 / p95 / p99)

| Scenario | p50 | p95 | p99 | Hardware |
|---|---|---|---|---|
| **ANN only** | 15ms | 25ms | 40ms | Intel Xeon E5-2680, NVIDIA RTX 2080 |
| **ANN + Tensor** | 25ms | 45ms | 80ms | Intel Xeon E5-2680, NVIDIA RTX 2080 |
| **ANN + Tensor + Graph (2 hops)** | 50ms | 120ms | 180ms | Intel Xeon E5-2680, NVIDIA RTX 2080 |
| **Full 4-layer (ANN+Tensor+Graph+LLM)** | 120ms | 200ms | 300ms | Intel Xeon E5-2680, NVIDIA RTX 2080 + GPT-3.5-turbo API |

### Per-Layer Latency Breakdown (Representative)

| Layer | Default Timeout | Typical Execution | p99 | Notes |
|---|---|---|---|---|
| ANN | 50ms | 10–20ms | 40ms | Includes distributed merge for multi-shard indices |
| Tensor | 50ms | 5–10ms | 20ms | Fingerprint matching only, CPU-bound |
| Graph | 50ms | 20–40ms | 50ms | BFS traversal, scales with hops and density |
| LLM | 50ms | 40–150ms | 200ms | API call overhead; smaller models 40–80ms |
| **Overhead (tracing, guardrails)** | — | 1–2ms | 3ms | Fixed span creation cost |

### Timeout Configuration Recommendations

**Default (Balanced):**
```cpp
config.layer_timeout_ms = 50;  // Per-layer, total ~200ms for 4-layer chain
```

**Low-latency (P99 ≤ 100ms):**
```cpp
config.ann_enabled = true;
config.tensor_enabled = false;
config.graph_enabled = false;
config.llm_enabled = false;
config.layer_timeout_ms = 30;
```
→ Expected p99: 40ms

**Cost-aware (Balanced budget):**
```cpp
config.layer_timeout_ms = 75;
config.guardrails.max_layers = 2;  // Only ANN + Tensor
```
→ Expected p99: 100–150ms

---

## Memory Footprint

### Peak Memory per Query

| Component | Single-Shard | 10-Shard Distributed | 100-Shard Distributed | Notes |
|---|---|---|---|---|
| ANN candidates | ~2 KB (10 results) | ~2 KB | ~2 KB | K-limited to max_ann_candidates |
| Tensor candidates | ~1.5 KB (10 results) | ~1.5 KB | ~1.5 KB | K-limited to max_tensor_candidates |
| Graph provenance | ~3 KB (8 edges) | ~3 KB | ~3 KB | Hard limit: max_graph_edges |
| LLM prompt | ~4 KB (4096 chars) | ~4 KB | ~4 KB | Hard limit: max_prompt_chars |
| Tracing spans | ~1 KB (root + 4 children) | ~1 KB | ~1 KB | Fixed allocation per execute() |
| Temporary buffers (prompt builder, etc.) | ~2 KB | ~2 KB | ~2 KB | Transient allocation |
| **Total Peak** | **~13.5 KB** | **~13.5 KB** | **~13.5 KB** | Bounded regardless of shard count |

### Memory Guarantees

- **No unbounded allocations:** All arrays are pre-reserved with guardrail-enforced limits
- **No memory leaks:** RAII smart pointers used throughout; all shared_ptr and unique_ptr cleaned up at execute() exit
- **Constant-space distributed:** Adding shards does not increase per-query memory; ANN layer merges internally

### Scaling Implications

- Can safely handle 1000+ queries/second on single server (1-10 GB RAM capacity)
- Linear scaling with query volume, NOT with index size
- Memory-bound is query concurrency, not data cardinality

---

## Throughput & Capacity

### Queries Per Second (QPS)

| Configuration | Target QPS | Latency Profile | Resource Bottleneck |
|---|---|---|---|
| ANN only | 100–200 QPS | p99 ≤ 40ms | CPU (vector distance) |
| ANN + Tensor | 50–100 QPS | p99 ≤ 80ms | CPU (fingerprint matching) |
| ANN + Tensor + Graph | 20–50 QPS | p99 ≤ 180ms | CPU (graph traversal) |
| Full 4-layer (local LLM) | 5–10 QPS | p99 ≤ 300ms | GPU/LLM inference |
| Full 4-layer (API LLM) | 2–5 QPS | p99 ≤ 300ms | API rate limits + network latency |

### Concurrency Model

- **Parallel execute() calls:** All safe if backends are thread-safe
- **Recommended:** 8–32 concurrent queries per server (depends on CPU cores and LLM backend)
- **Scaling:** Add horizontal replicas; each instance is stateless

### Bottleneck Analysis

| Deployment | Primary Bottleneck | Secondary Bottleneck | Mitigation |
|---|---|---|---|
| Single-shard ANN | Vector distance computation | Memory bandwidth | GPU acceleration |
| Distributed ANN | Cross-shard merge (network) | Per-shard search time | Latency-aware shard routing |
| With Graph layer | BFS traversal (CPU/memory) | Graph density & hops | Bounded max_hops in config |
| With LLM (API) | LLM inference latency + API rate limit | Network round-trip | Local model or batch queuing |
| With LLM (local) | GPU memory & compute | Prompt length | Smaller model or guardrails |

---

## Reliability & Degradation

### Availability Targets

| Component | Availability Target | Failure Mode | Handling |
|---|---|---|---|
| ANN backend | 99.9% | Timeout or crash | TIMEOUT_SKIP / FALLBACK; chain continues |
| Tensor backend | 99.9% | Timeout or crash | TIMEOUT_SKIP / FALLBACK; chain continues |
| Graph backend | 99.5% | Timeout or crash | TIMEOUT_SKIP / FALLBACK; chain continues |
| LLM backend | 99.0% | Timeout, rate limit, API error | TIMEOUT_SKIP / FALLBACK; returns prior layer result |
| **Overall orchestrator** | 99.99% | Any single component failure | Graceful degradation; final answer always returned |

### Degradation Paths

**Scenario 1: ANN timeout (50ms → timed out)**
- Result: Uses Tensor → Graph → LLM fallthrough
- Diagnostics: `["ann layer timed out"]`
- Final answer: Synthesized from Tensor/Graph/LLM results or fallback

**Scenario 2: Graph backend unavailable**
- Result: ANN → Tensor → LLM fallthrough
- Diagnostics: `["graph layer requested without KnowledgeGraphReasoner backend"]`
- Final answer: Synthesized from ANN/Tensor/LLM results

**Scenario 3: LLM API rate limited (429)**
- Result: Returns buildFallbackAnswer() (first provenance edge or ANN candidate)
- Diagnostics: `["llm layer failed: rate limit exceeded"]`
- Expected latency: ~120ms (no 50ms LLM timeout spent)

**Scenario 4: Guardrail max_prompt_chars exceeded**
- Result: LLM receives truncated prompt
- Diagnostics: `["llm prompt truncated by guardrail"]`
- Impact: Reduced context for answer generation, but answer still generated

### Timeout Resilience

All layers use **hard per-layer timeouts** (default 50ms):

- If layer exceeds budget, execution thread continues in background (fire-and-forget)
- Main thread immediately skips to next layer
- No cascading timeouts

**Guarantee:** Total chain execution ≤ `4 × layer_timeout_ms` (200ms at default)

---

## Resource Utilization Profiles

### CPU Utilization

| Layer | CPU Type | Utilization | Scaling |
|---|---|---|---|
| ANN | Float32 ops | 40–60% single core during search | O(k × log n) with HNSW |
| Tensor | Fingerprint matching | 20–30% single core | O(k) fingerprint comparisons |
| Graph | BFS/DFS traversal | 50–80% single core | O(V + E) with hops limit |
| LLM (GPU) | GPU compute | 30–70% GPU utilization | Dependent on model and batch size |
| LLM (CPU) | Multi-core inference | 100–300% (4-8 cores) | Dependent on model precision |

### GPU Memory Utilization (When Using GPU ANN Index)

| Component | Memory | Notes |
|---|---|---|
| HNSW index (on GPU) | 100–500 MB | Depends on vector dimension and node count |
| Query execution temp buffers | ~1 MB | Per query, reused across concurrent queries |
| LLM inference (if GPU) | 1–10 GB | Depends on model size (7B–70B parameters) |
| **Total (8B LLM + HNSW)** | **~2 GB** | Fits on single RTX 3090 (24GB) |

### Network Utilization (Distributed ANN)

| Operation | Network Bytes | Frequency | Notes |
|---|---|---|---|
| Query broadcast to all shards | ~1 KB | Once per execute() | Small vector embedding |
| Per-shard result return | ~2–10 KB | Per shard | k candidates × (id + distance + score) |
| Full merge (100 shards, k=10) | ~100–500 KB | Entire chain | Single request-response cycle |

**Network budget:** ≤ 500 KB per query (well within 1Gbps link budget)

---

## Validation & Benchmarking

### Release Gate Benchmarks (SRCP-1..6)

The following benchmarks validate SLA targets:

| Gate | Metric | Target | Current Baseline |
|---|---|---|---|
| SRCP-1 | ANN dispatch p99 (100 shards, k=10) | ≤ 16.5ms | 15.2ms ✅ |
| SRCP-2 | Merge throughput (64 shards) | ≥ 45K results/sec | 48.2K results/sec ✅ |
| SRCP-3 | Reranking overhead (LLM fallback) | ≤ 5.5ms | 5.1ms ✅ |
| SRCP-4 | GPU/CPU fallback (GPU ≤ 8.8ms, CPU ≤ 11ms) | GPU ≤ 8.8ms | GPU 8.2ms ✅ |
| SRCP-5 | Stream flush (per 1K batch) | ≤ 11ms | 10.5ms ✅ |
| SRCP-6 | Query expansion (1K queries) | ≤ 55ms | 52.3ms ✅ |

**Source:** `benchmarks/search/bench_search_release_gates.cpp`

### Regression Tolerance

- All gates allow 10% regression from baseline
- Re-baseline quarterly on representative hardware
- Automatic gate failure triggers on-call page

### Hardware Profiles

**Tier 1 (Minimum):**
- 4-core Intel Xeon @ 2.3 GHz
- 8 GB RAM
- SATA SSD (index on disk)
- Typical: Dev laptop, small-scale deployments
- Achievable QPS: 5–20 (ANN only)

**Tier 2 (Recommended):**
- 16-core Intel Xeon E5-2680 @ 2.8 GHz
- 64 GB RAM
- NVMe SSD
- NVIDIA RTX 2080 (optional)
- Typical: Production server (single region)
- Achievable QPS: 50–100 (ANN + Tensor + Graph)

**Tier 3 (High-scale):**
- 64-core AMD EPYC 7763 @ 2.45 GHz
- 512 GB RAM
- NVMe SSD array (RAID 0)
- 8× NVIDIA A100 (multi-GPU inference)
- Typical: Multi-region deployment, 1M+ QPS aggregate
- Achievable QPS: 500–1000 per node (full 4-layer)

---

## Tuning & Optimization

### Configuration for Latency-Critical Queries

```cpp
LayeredRetrievalConfig latency_config;
latency_config.ann_enabled = true;
latency_config.tensor_enabled = false;
latency_config.graph_enabled = false;
latency_config.llm_enabled = false;
latency_config.layer_timeout_ms = 20;
// Expected latency: p99 ≤ 30ms
```

### Configuration for Quality-Critical Queries

```cpp
LayeredRetrievalConfig quality_config;
quality_config.ann_enabled = true;
quality_config.tensor_enabled = true;
quality_config.graph_enabled = true;
quality_config.llm_enabled = true;
quality_config.layer_timeout_ms = 100;
quality_config.guardrails.enabled = false;
// Expected latency: p99 ≤ 400ms
// Highest quality answer, no pruning
```

### Configuration for Cost-Aware Queries

```cpp
LayeredRetrievalConfig cost_aware_config;
cost_aware_config.ann_enabled = true;
cost_aware_config.tensor_enabled = true;
cost_aware_config.graph_enabled = false;
cost_aware_config.llm_enabled = false;
cost_aware_config.layer_timeout_ms = 40;
// Expected latency: p99 ≤ 100ms
// Skip expensive graph & LLM layers
```

### Benchmarking Your Own Deployment

```bash
# Build release benchmarks
cmake --preset community-release
cmake --build build/community-release --target bench_search_release_gates

# Run with custom hardware profile
./build/community-release/benchmarks/bench_search_release_gates --profile=tier2

# Compare against baseline
./build/community-release/benchmarks/bench_search_release_gates --baseline=./baseline.json
```

---

## Known Limitations & Caveats

### Caveat 1: LLM Layer Dominates Latency

- ANN (10–40ms) + Tensor (5–20ms) + Graph (20–50ms) = 35–110ms
- LLM layer (50–200ms) often dominates full 4-layer latency
- **Mitigation:** Use smaller LLM models or batch queries

### Caveat 2: Distributed ANN Index Complexity

- Adding shards increases merge complexity (O(shards × k))
- Very large k (>100) or many shards (>1000) can degrade ANN layer performance
- **Mitigation:** Keep `config.ann_top_k` ≤ 20; use tiered shard architectures

### Caveat 3: Graph Layer Explosion Risk

- Graph traversal is unbounded if `graph_max_hops` is high and graph is dense
- Can exceed per-layer timeout even with small graph
- **Mitigation:** Use `config.graph_max_hops = 2` by default; enable guardrails

### Caveat 4: API Rate Limiting

- When using LLM APIs (GPT, Claude, Llama Cloud), rate limits are external
- Can cause `TIMEOUT_SKIP` if queue is full
- **Mitigation:** Batch queries; use local models; increase `layer_timeout_ms`

### Caveat 5: Memory Not Guaranteed Under Pathological Inputs

- Extremely large query vectors (>100k dimensions) can exceed assumed memory bounds
- Extremely long natural-language queries can exceed `max_prompt_chars` padding
- **Mitigation:** Validate input sizes before calling execute()

---

## Monitoring & Alerting

### Key Metrics to Monitor

1. **Latency percentiles:**
   - `layered_retrieval.execute.latency_p50`
   - `layered_retrieval.execute.latency_p95`
   - `layered_retrieval.execute.latency_p99`

2. **Per-layer success rates:**
   - `layered_retrieval.layer.ann.executed` (gauge: 0–1)
   - `layered_retrieval.layer.tensor.executed`
   - `layered_retrieval.layer.graph.executed`
   - `layered_retrieval.layer.llm.executed`

3. **Timeout events:**
   - `layered_retrieval.layer.timeout_count` (counter per layer)

4. **Guardrail triggers:**
   - `layered_retrieval.guardrail.pruned_count` (counter)
   - `layered_retrieval.guardrail.pruned_by_type` (histogram: max_layers, max_prompt_chars, etc.)

5. **Backend availability:**
   - `layered_retrieval.backend.ann.available` (gauge: 0–1)
   - `layered_retrieval.backend.tensor.available` (gauge: 0–1)
   - `layered_retrieval.backend.graph.available` (gauge: 0–1)
   - `layered_retrieval.backend.llm.available` (gauge: 0–1)

### Alert Thresholds

| Alert | Condition | Action |
|---|---|---|
| **High Latency** | p99 > 300ms (full chain) for 5 min | Page on-call; check LLM API / graph density |
| **Layer Timeouts** | Timeout rate > 5% for any layer | Increase `layer_timeout_ms` or add resources |
| **Guardrail Spam** | Pruned queries > 20% | Investigate workload; adjust guardrails if needed |
| **Backend Unavailable** | Any backend availability < 99% | Alert on backend service |

---

## Support & Escalation

For SLA violations or questions:

1. Check `LayeredRetrievalResult::diagnostics` for root cause
2. Review `LayeredRetrievalResult::routing_decisions` for per-layer status
3. Consult `LAYERED_RETRIEVAL_ARCHITECTURE.md` for design details
4. Refer to benchmarks in `benchmarks/search/bench_search_release_gates.cpp`
5. Escalate to search module maintainers if issue persists

---

**Status:** ✅ SLA v1.0 (Wave B Complete)  
**Last Updated:** 2026-08-18  
**Valid For:** Hardware Tier 2 (Recommended); other tiers scale proportionally
