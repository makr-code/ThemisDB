# Layered Retrieval Architecture — 4-Layer Chain Design

<!-- Status: current | validated: 2026-08-18 -->
<!-- Links: README.md · ROADMAP.md · ARCHITECTURE.md · LAYERED_RETRIEVAL_SLA.md -->

## Overview

The **LayeredRetrievalOrchestrator** implements a four-stage retrieval pipeline designed to combine different retrieval modalities into a unified result:

```
Query Input
    ↓
[ANN Layer] — Vector similarity via HNSW/distributed clustering
    ↓ (optional fallthrough)
[Tensor Layer] — Embedding fingerprint matching
    ↓ (optional fallthrough)
[Graph Layer] — Provenance inference & knowledge graph traversal
    ↓ (optional fallthrough)
[LLM Layer] — Answer generation from aggregated candidates
    ↓
Final Answer + Diagnostics
```

**Key Design Principles:**
- Each layer is independently optional, configurable, and fail-safe
- Hard per-layer time budgets prevent cascading latency
- Per-query guardrails bound memory and fan-out at every stage
- Full tracing and diagnostics propagate through the entire chain
- Graceful degradation when any layer is unavailable or fails

---

## Layer Specifications

### Layer 1: ANN (Approximate Nearest Neighbor) Retrieval

**Purpose:** Dense vector-based similarity search using HNSW or GPU-accelerated indices.

**Inputs:**
- `query_vector`: Dense embedding (float32 array)
- `config.ann_top_k`: Number of candidates to retrieve (default: 10)

**Outputs:**
- `AnnLayerCandidate[]` — List of {id, distance, score}
- Each distance score is converted to 1.0 / (1.0 + distance)

**Execution Model:**
1. Validate `query_vector` is non-empty; skip layer if not provided
2. Clamp `top_k` to guardrail `max_ann_candidates` if enabled
3. Execute `AdvancedVectorIndex::search(query_vector, top_k)` with hard timeout
4. Convert distances to normalized scores
5. Emit tracing span with latency and output count

**Fail-Safe Behavior:**
- Timeout → TIMEOUT_SKIP with diagnostics
- Backend missing → FALLBACK (empty candidates)
- Empty results → FALLBACK (candidates pruned by guardrail)

**Guardrails:**
- `max_ann_candidates`: Maximum output fan-out (prevents downstream explosion)
- `max_layers`: Can be skipped by layer count limit

**Distributed Clustering Integration:**
- For distributed HNSW: clustering is performed by `AdvancedVectorIndex` itself
- Orchestrator treats index as opaque and does not manage partitioning
- Index must handle cross-shard candidate merging internally
- Orchestrator only requires a conforming `search(vector, k)` interface

---

### Layer 2: Tensor Fingerprint Graph

**Purpose:** Match embeddings against pre-computed tensor fingerprint signatures to detect similar models/adapters.

**Inputs:**
- `tensor_query_key`: Fingerprint identifier or key (string)
- `top_k`: Derived from max(1, ann_candidates.size()) or `config.ann_top_k`

**Outputs:**
- `TensorLayerCandidate[]` — List of {adapter_key, domain, base_model_id, score}
- Each result represents a matched adapter fingerprint

**Execution Model:**
1. Validate `tensor_query_key` is non-empty; skip layer if not provided
2. Clamp `top_k` to guardrail `max_tensor_candidates` if enabled
3. Execute `TensorFingerprintGraph::findSimilar(query_key, top_k)` with hard timeout
4. Return adapted candidates with similarity scores
5. Emit tracing span with latency and output count

**Fail-Safe Behavior:**
- Timeout → TIMEOUT_SKIP with diagnostics
- Backend missing → FALLBACK (empty candidates)
- No query key → FALLBACK (layer disabled)

**Guardrails:**
- `max_tensor_candidates`: Cap output from tensor layer

---

### Layer 3: Knowledge Graph Provenance

**Purpose:** Infer logical provenance chains and validate candidate relationships using knowledge graph rules.

**Inputs:**
- `graph_subject_id`: Entity whose provenance should be inferred (string)
- `config.graph_max_hops`: Maximum traversal depth (default: 2)
- `lora_adapter_id`: Optional adapter ID for model-specific scoring

**Outputs:**
- `ProvenanceEntry[]` — List of {subject, predicate, object, rule_id, lora_score}
- Each entry represents a validated fact or inference step

**Execution Model:**
1. Validate `graph_subject_id` is non-empty; skip layer if not provided
2. Execute `KnowledgeGraphReasoner::infer(subject, max_hops)` with hard timeout
3. If `lora_adapter_id` is provided, apply adapter scoring via `applyLoRAScore()`
4. Clamp results to guardrail `max_graph_edges` if enabled
5. Emit tracing span with latency and output count

**Fail-Safe Behavior:**
- Timeout → TIMEOUT_SKIP with diagnostics
- Backend missing → FALLBACK (empty provenance)
- No subject → FALLBACK (layer disabled)

**Guardrails:**
- `max_graph_edges`: Limit inference depth in downstream LLM prompt
- Prevents runaway knowledge graph traversal

**LoRA Scoring:**
- When `lora_adapter_id` is provided, each provenance edge receives a model-specific plausibility score
- Used for answer confidence estimation in LLM layer

---

### Layer 4: LLM Answer Generation

**Purpose:** Generate a natural-language answer by synthesizing all upstream retrieval results.

**Inputs:**
- `query`: Natural language question (string)
- Aggregated results from ANN, Tensor, and Graph layers
- `llm_prompt_prefix`: Optional system preamble (string)

**Outputs:**
- `final_answer`: Generated text response
- `success`: Boolean indicating generation succeeded

**Prompt Synthesis:**
1. Start with optional system prefix
2. Append natural query text
3. Append summarized ANN candidates (id, score pairs)
4. Append summarized Tensor candidates (adapter key, score pairs)
5. Append summarized Graph provenance (S-P-O triplets)
6. Clamp total prompt to guardrail `max_prompt_chars` if enabled
7. Invoke LLM with timeout and generation options

**Execution Model:**
1. Validate `query` is non-empty; skip layer if not provided
2. Build composite prompt from upstream results
3. Execute `LLMClient::generate(prompt, options)` with hard timeout
4. If generation fails or returns empty, use fallback answer (first ANN/Tensor/Graph result)
5. Emit tracing span with latency and output size

**Fail-Safe Behavior:**
- Timeout → TIMEOUT_SKIP with fallback answer
- Backend missing → FALLBACK (use buildFallbackAnswer)
- Generation failed/empty → FALLBACK (structured fallback)

**Guardrails:**
- `max_prompt_chars`: Truncate prompt if it exceeds limit
- `max_layers`: Can skip LLM layer if upstream layers exceeded limit

**Fallback Answer Construction (buildFallbackAnswer):**
1. If provenance available: return first S-P-O triplet
2. Else if tensor candidates available: return first adapter key
3. Else if ANN candidates available: return first candidate ID
4. Otherwise: return "no layered retrieval answer available"

---

## Timeout Model & Concurrency

### Per-Layer Deadline Enforcement

Each layer is wrapped in a **hard timeout** (configured via `LayeredRetrievalConfig::layer_timeout_ms`):

```cpp
runWithDeadline(
    [backend, inputs]() { /* layer execution */ },
    timeout_duration,
    error_string
);
```

**Implementation:**
- Each layer spawns its execution in a detached thread
- Main thread waits for completion or deadline
- On timeout, thread continues in background (fire-and-forget)
- Layer marked as TIMEOUT_SKIP; chain continues

**Default Timeout:** 50ms per layer (configurable)

**Total Latency Budget:** Up to 200ms for full 4-layer chain at default 50ms per layer

### Thread Safety Model

**Constructor Setup (NOT thread-safe):**
- `setAnnIndex()`, `setTensorGraph()`, `setGraphReasoner()`, `setLlmClient()`, `setTracer()`, `setConfig()`
- Must complete before any concurrent execute() calls
- Synchronized by caller

**execute() (Read-only, thread-safe if backends are safe):**
- No mutable state modified during execute()
- All state reads are from config_ and injected backends
- Safe for concurrent calls if backend implementations are thread-safe

**Recommendation:** Pre-wire all backends during application startup; treat orchestrator as read-only during request serving.

---

## Guardrail Model

**PerQueryRetrievalGuardrails** enforce bounded behavior across the entire chain:

```cpp
struct PerQueryRetrievalGuardrails {
    bool enabled = false;
    std::size_t max_ann_candidates = 10;
    std::size_t max_tensor_candidates = 10;
    std::size_t max_graph_edges = 8;
    std::size_t max_prompt_chars = 4096;
    std::size_t max_layers = 4;  // Prevent running all 4 layers
};
```

**Enforcement Order:**
1. **max_layers check** happens first: if `layers_started >= max_layers`, subsequent layers are skipped with GUARDRAIL_SKIP
2. **Per-layer clamping** happens within each layer before execution
3. **Prompt truncation** happens in LLM layer if prompt exceeds max_prompt_chars

**Use Cases:**
- **Low-latency queries:** Set `max_layers=2` to run only ANN + Tensor
- **Bounded memory:** Set `max_ann_candidates=5`, `max_graph_edges=4` to limit fan-out
- **Cost-aware:** Set `max_prompt_chars=1024` for smaller LLM models

---

## Tracing & Observability

### OpenTelemetry Integration

Each execute() call emits a root span and per-layer child spans:

```
Span: search.layered_retrieval.execute (root)
  ├─ Attributes:
  │  ├─ correlation.id (from context)
  │  ├─ guardrails.enabled
  │  ├─ layers.executed (count)
  │  └─ result.timed_out
  │
  ├─ Span: search.layer.ann (child)
  │  ├─ layer.name, layer.status, layer.latency_ms
  │  ├─ layer.output_count, correlation.id
  │  └─ [Error recorded if state != EXECUTED]
  │
  ├─ Span: search.layer.tensor (child)
  │  └─ ...similar attributes...
  │
  ├─ Span: search.layer.graph (child)
  │  └─ ...similar attributes...
  │
  └─ Span: search.layer.llm (child)
     └─ ...similar attributes...
```

### Layer Routing Decisions

Every layer emits a `LayerDecisionRecord`:

```cpp
struct LayerDecisionRecord {
    std::string layer_name;
    LayerRoutingDecision decision;  // EXECUTED, FALLBACK, TIMEOUT_SKIP, GUARDRAIL_SKIP, DISABLED
    std::string detail;             // Error message or status detail
    std::uint64_t latency_ms;       // Wall-clock time
    std::size_t input_count;        // Inputs to this layer
    std::size_t output_count;       // Outputs from this layer
};
```

**Decisions:**
- `EXECUTED`: Layer completed successfully
- `FALLBACK`: Layer unavailable or empty results
- `TIMEOUT_SKIP`: Deadline exceeded
- `GUARDRAIL_SKIP`: Guardrail blocked execution
- `DISABLED`: Layer disabled in config

### Diagnostics Vector

All issues are logged to `LayeredRetrievalResult::diagnostics`:

Example entries:
- `"ann layer timed out"`
- `"tensor layer requested without TensorFingerprintGraph backend"`
- `"graph provenance pruned by guardrail"`
- `"llm prompt truncated by guardrail"`

---

## Configuration & Deployment

### Configuration Structure

```cpp
struct LayeredRetrievalConfig {
    bool ann_enabled = true;
    bool tensor_enabled = true;
    bool graph_enabled = true;
    bool llm_enabled = true;

    std::uint32_t layer_timeout_ms = 50;
    std::size_t ann_top_k = 10;
    int graph_max_hops = 2;

    PerQueryRetrievalGuardrails guardrails;  // Default: disabled
};
```

### Deployment Topologies

**Option A: Full 4-Layer Pipeline (Default)**
- All layers enabled, 50ms timeout per layer
- Suitable for high-quality retrieval, e.g. high-value customer queries
- Expected latency: 150–250ms (accounting for variance)

**Option B: Fast ANN + LLM (Low-latency)**
```cpp
config.tensor_enabled = false;
config.graph_enabled = false;
config.layer_timeout_ms = 30;
```
- Expected latency: 60–100ms
- Suitable for real-time search UIs

**Option C: Staged Execution with Guardrails (Cost-aware)**
```cpp
config.guardrails.enabled = true;
config.guardrails.max_layers = 2;
config.guardrails.max_ann_candidates = 5;
```
- Only top 2 layers run, fewer candidates propagate downstream
- Ideal for budget-constrained environments or high query volume

---

## Scaling & Performance Considerations

### ANN Layer Scaling (Distributed Clustering)

The ANN layer delegates clustering to `AdvancedVectorIndex`:

- **Single-shard:** Index holds full vector space; search() returns top-k directly
- **Multi-shard (distributed HNSW):** Index manages k-way clustering internally; search() merges results from all shards
- **Orchestrator responsibility:** None (clustering is opaque to orchestrator)
- **Guarantees:** Any implementation meeting `AdvancedVectorIndex` interface works seamlessly

### Memory Profile

Per-query memory consumption:

| Component | Memory | Notes |
|---|---|---|
| ANN candidates | O(k) | Typically ~2–10 results |
| Tensor candidates | O(k) | Typically ~2–10 results |
| Graph provenance | O(edges) | Guardrail: max_graph_edges ≤ 8 |
| LLM prompt | O(chars) | Guardrail: max_prompt_chars ≤ 4096 |
| Tracing spans | O(1) | Fixed-size metadata per layer |
| **Total** | **~10–20 KB** | Per-query peak |

### CPU & Latency Scaling

| Layer | Typical Latency | CPU Profile | Scaling Notes |
|---|---|---|---|
| ANN | 10–30ms | Parallel search + merge | Scales with k and shard count |
| Tensor | 5–15ms | Fingerprint matching | Scales with k |
| Graph | 20–50ms | BFS/DFS traversal | Scales with hops and graph density |
| LLM | 50–200ms | Inference | Scales with prompt length and model |
| **Total (p99)** | **200–300ms** | Sequential chain | See LAYERED_RETRIEVAL_SLA.md |

---

## Error Handling & Resilience

### Failure Modes

| Failure | Handling | Result |
|---|---|---|
| Backend unavailable (null pointer) | FALLBACK | Layer skipped, chain continues |
| Missing query input | FALLBACK | Layer skipped, chain continues |
| Timeout exceeded | TIMEOUT_SKIP | Layer skipped, chain continues |
| Guardrail exceeded | GUARDRAIL_SKIP | Layer skipped, diagnostics recorded |
| Layer exception | Caught, FALLBACK | Error logged, chain continues |
| Final answer missing | Fallback construction | Synthesized from upstream results |

### Guarantee: Never Throws

`execute()` is `const` and never throws. All errors are captured in `LayeredRetrievalResult`.

---

## Testing & Validation

### Unit Tests

- `test_layered_retrieval_ann_layer.cpp` — ANN execution, fallback, timeout
- `test_layered_retrieval_tensor_layer.cpp` — Tensor matching, pruning
- `test_layered_retrieval_graph_layer.cpp` — Graph inference, LoRA scoring
- `test_layered_retrieval_llm_layer.cpp` — Answer generation, fallback
- `test_layered_retrieval_guardrails.cpp` — Per-query limit enforcement
- `test_layered_retrieval_timeouts.cpp` — Deadline enforcement, deadline race conditions

### Integration Tests

- `test_layered_retrieval_full_chain.cpp` — All 4 layers in sequence
- `test_layered_retrieval_partial_failures.cpp` — Shard failure, timeout, guardrail interaction
- `test_layered_retrieval_concurrency.cpp` — Multiple concurrent execute() calls
- `test_layered_retrieval_tracing.cpp` — OpenTelemetry span emission

### Chaos/Fault-Injection Tests

- Backend timeouts (injected via layer-level deadline violations)
- Partial results (backend returns fewer candidates than requested)
- Guardrail overflow (query exceeds max_prompt_chars)

---

## Related Documentation

- **LAYERED_RETRIEVAL_SLA.md** — Performance targets, latency/memory/throughput baselines
- **ARCHITECTURE.md** — Full search module architecture including v3.0.0 contract
- **ROADMAP.md** — Wave B completion status and evidence
- **bench_search_release_gates.cpp** — Performance gate benchmarks (SRCP-1..6)

---

**Status:** ✅ Architecture v3.0.0 (Wave B Complete)  
**Last Updated:** 2026-08-18  
**Maintainer:** Search Module Team
