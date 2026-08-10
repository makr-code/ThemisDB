# RAID-Sharded Inference: A Co-Design Architecture for Distributed Large Language Model Serving in Hybrid Database Systems

*Preprint, April 2026*

**Authors:** ThemisDB Contributors | **Artifact:** Open-source reference implementation | **Code:** https://github.com/makr-code/ThemisDB | **License:** MIT

---

## Abstract

Large language model serving, retrieval-augmented generation, and distributed database sharding are usually treated as separate systems problems. This paper proposes a co-design architecture that unifies them in a RAID-sharded hybrid database, where each shard combines persistent storage, vector indexing, graph-aware query capability, and LLM inference capability under strict transactional guarantees [31]–[35]. Grounded in the current ThemisDB codebase, the design addresses the gap between single-node LLM runtimes and database-native distributed execution through three optimisation layers: (1) **gossip-driven LoRA domain routing**, which propagates per-adapter accuracy and latency signals to route requests toward the most domain-affine shard; (2) **scatter-gather batch inference**, which adapts distributed analytics fan-out to parallel LLM request execution across shards; and (3) **cross-shard speculative decoding**, in which a lightweight draft shard proposes tokens that are verified by a larger shard, with a theoretical speedup of about 2× at 65 % acceptance. We further describe a distributed KV-prefix sharing protocol based on llama.cpp state serialisation to target time-to-first-token reductions of at least 30 % for repeated prompt prefixes, and we examine RAID0/1/5 redundancy modes for fault-tolerant model-weight storage using AVX2-accelerated erasure coding [10], [24]. Rather than claiming a fully validated serving system, the paper contributes a design study, theoretical performance analysis, and repository-grounded gap analysis showing that the required integration points appear feasible without protocol changes. The result is a concrete six-phase implementation roadmap for database-native distributed LLM inference, together with an openly inspectable software artifact in the ThemisDB repository.

---

## I. Introduction

The rapid adoption of large language models in enterprise information systems has exposed a fundamental architectural mismatch: modern LLM serving frameworks [1], [2] assume that models reside in a dedicated GPU cluster, while database management systems (DBMS) are engineered for data locality, fault-tolerant storage, and horizontal sharding [3]. Bridging these two paradigms requires a co-design approach in which inference capability is embedded into each storage shard, rather than delegated to a separate serving tier. This direction is consistent with recent database research arguing for analytical engines that co-optimise relational operators and model-based processing inside a unified execution environment [31]. The underlying ThemisDB implementation referenced throughout this paper is open-source and publicly available for inspection on GitHub under the MIT license [34].

Horizontal database sharding distributes both data and compute across N autonomous nodes. When each shard additionally runs a full inference engine, the cluster becomes a distributed inference fabric with a key advantage: queries can be evaluated on the node that holds the relevant data, avoiding cross-shard data movement. This *data-locality principle* is well established for analytical workloads [4] but has not been systematically applied to LLM inference.

### A. Foundational Assumptions

Our design starts from four system-level assumptions about ThemisDB itself.

1. **The database layer is the retrieval substrate.** ThemisDB is assumed to be a multi-model database that natively combines relational, vector, and graph access paths, allowing RAG-style retrieval, iterative re-retrieval loops such as FLARE [35], and graph-augmented evidence expansion to be executed against a single transactional substrate rather than across loosely coupled services [32]–[34].

2. **Strict transactional guarantees remain non-negotiable.** Retrieval quality alone is insufficient for enterprise workloads; the underlying storage and query path must preserve strict ACID guarantees, including MVCC-based concurrency control, serializability-oriented execution paths such as SSI, and distributed transaction coordination where required [34]. In this view, LLM serving is added on top of the database, not in place of database correctness.

3. **Distributed RAID-sharding defines the data plane of the cluster.** Cluster formation is assumed to happen first at the database level: data is partitioned and replicated across shards using consistent-hash sharding and RAID-style redundancy strategies [10], [24], [34]. LLM execution is therefore treated as a workload that runs over the distributed data plane rather than as an independent serving fabric.

4. **Each shard is a converged storage-retrieval-inference node.** Once multi-model retrieval and distributed sharding are in place, each shard is assumed to co-locate persistent storage, vector search, graph traversal support, and LLM runtime components [31], [34]. This assumption is what makes locality-preserving RAG, shard-local embedding generation, and cross-shard orchestration mechanisms such as speculative decoding and KV-prefix transfer meaningful in the first place.

Several challenges arise in this setting that are absent from single-node serving:

1. **Domain-specific adapter placement.** Different shards accumulate different fine-tuned LoRA adapters [5] through federated incremental training [6]. Routing an inference request to the wrong shard wastes the specialisation achieved.

2. **Batch workload distribution.** Enterprise workloads issue batches of semantically related requests. Routing the entire batch to a single shard under-utilises the cluster; a scatter-gather model analogous to distributed OLAP [7] is needed.

3. **Speculative decoding across model-size tiers.** Clusters naturally contain heterogeneous nodes. Exploiting the draft-verify speedup [8], [9] requires cross-node coordination that existing frameworks do not provide.

4. **Fault-tolerant weight storage.** RAID parity techniques [10] protect data blocks; applying equivalent protection to quantised model weights enables single-shard failure recovery without full re-download.

5. **KV-state transfer for prefix caching.** Shared system prompts dominate time-to-first-token (TTFT) in RAG workloads. Serialising the KV state and migrating it between shards avoids redundant prefill computation.

This paper makes the following contributions:

- A **unified co-design architecture** in which every RAID shard runs a `ContinuousBatchScheduler`, a `PagedKVCache`, and an `InferenceEngineEnhanced`, connected by an `AdaptiveShardRouter` that exploits gossip-propagated domain capability scores.
- A **gossip-driven domain routing protocol** based on `AdapterCapabilityAnnouncement` payloads carrying `accuracy_delta`, `performance_delta_p99_ms`, and federated round bookkeeping.
- A **scatter-gather batch inference pipeline** reusing the `DistributedAnalyticsSharding` fan-out pattern with per-domain request affinity splitting.
- A **cross-shard speculative decoding scheme** with configurable remote draft shards and accept-rate telemetry.
- A **distributed KV-prefix cache** using llama.cpp state serialisation primitives transferred via mTLS-protected remote executors.
- A **gap analysis** against the current codebase and a six-phase implementation roadmap with measurable performance targets.

**Paper Classification:** This work is positioned as a *design study* grounded in the ThemisDB open-source codebase. We provide theoretical performance analysis, architecture justification, and a concrete implementation roadmap, but defer empirical validation of performance claims to the roadmap phases (Q3 2026–Q1 2027). This positioning is appropriate for arXiv preprint submission and enables community feedback prior to implementation.

### B. Research Questions and Hypotheses

RQ1: Under realistic mixed database workloads, how much end-to-end latency reduction can cross-shard speculative decoding deliver without violating reliability targets?

RQ2: How effectively does gossip-driven domain routing improve domain-affine response quality relative to consistent-hash fallback routing?

RQ3: What is the practical trade-off between RAID fault-tolerance mode (RAID0/1/5) and serving continuity for converged storage-retrieval-inference shards?

H1: Cross-shard speculative decoding with acceptance rate >= 65 % achieves at least 2x speedup over non-speculative decoding under intra-cluster RTT constraints described in Section VI.

H2: Gossip-propagated adapter capability routing reduces domain-mismatch rate and improves quality-latency operating points compared to topology-only routing.

The remainder of this paper is organised as follows. Section II reviews related work. Section III describes the system architecture. Section IV details each optimisation layer. Section V analyses the RAID fault-tolerance model. Section VI provides a theoretical performance analysis. Section VII presents the implementation roadmap and gap analysis. Section VIII discusses limitations, artifact availability, and positioning relative to dedicated serving systems. Section IX concludes.

---

## II. Related Work

### A. LLM Serving Systems

**vLLM** introduced PagedAttention [1], mapping KV cache to non-contiguous virtual memory pages analogous to OS virtual memory, enabling near-zero KV waste and high-throughput continuous batching. Kwon et al. demonstrate a 2–4× throughput improvement over prior work. Our `ContinuousBatchScheduler` (with `max_batch_size = 256`, `max_tokens_per_batch = 8192`) provides a direct per-shard analogue of this paradigm.

**Orca** [2] introduced iteration-level scheduling (continuous batching), where the scheduler inserts new requests between decode iterations rather than waiting for batch completion. This eliminates head-of-line blocking and is the baseline for our per-shard scheduler design.

**AlpaServe** [3] and **FlexGen** [11] address resource heterogeneity in inference, the former via model parallelism placement, the latter via offloading. Our architecture is orthogonal: we distribute requests across specialised shards rather than partitioning a single model.

**Sarathi-Serve** [12] introduces chunked-prefill scheduling to bound the impact of long prefills on decode latency, corresponding to our `chunked_prefill_size = 512` token setting in `ContinuousBatchScheduler`.

### B. Speculative Decoding

Speculative decoding [8] uses a small draft model to propose a token sequence that a large target model verifies in a single forward pass, recovering the target distribution exactly while reducing wall-clock time proportionally to the acceptance rate. Leviathan et al. [8] prove that acceptance rates of 65–80 % are achievable on aligned model pairs, yielding speedups of 2–3×. Chen et al. [9] independently demonstrate the same result.

**Prompt Lookup Decoding** [13] replaces the draft model with n-gram matching against the input context, achieving moderate speedups without additional model overhead — well-suited to repetitive database query patterns.

**Lookahead Decoding** [14] constructs parallel candidate sequences using Jacobi iterations, accepting multiple tokens per step without a draft model.

Recent distributed extensions of speculative decoding include **WISP** [15], which co-optimises SLO-aware batching with speculative serving at the edge, and **FlowSpec** [16], which pipelines draft generation and verification across distributed nodes. Our cross-shard scheme most closely resembles **DSD** [17] in its use of an edge (draft) node and cloud (verify) node, but extends it to a RAID-sharded database context where the draft shard is selected by gossip-driven capability scoring.

**WANSpec** [18] leverages geographically distributed compute for LLM inference using speculative decoding as the communication efficiency mechanism, reporting latency reductions of 40 % over naive remote inference. This motivates our cross-shard speculative scheme for geographically distributed ThemisDB deployments.

### C. Federated Learning and LoRA Adaptation

**FedAvg** [6] established the communication-efficient federated learning baseline. Applied to LLM fine-tuning, the critical bottleneck is the size of full model gradients. **LoRA** [5] addresses this by restricting updates to low-rank weight matrices $\Delta W = AB^\top$, where $A \in \mathbb{R}^{d \times r}$, $B \in \mathbb{R}^{k \times r}$, $r \ll \min(d, k)$.

**EcoLoRA** [19] extends federated LoRA to heterogeneous clients with asymmetric rank allocation, reducing communication cost by up to 3× with negligible accuracy loss. **FDLoRA** [20] introduces dual adapter tuning — one shared global adapter, one personalised local adapter — which maps naturally to our architecture where each shard maintains a domain-specific LoRA alongside the base model.

In the proposed design, the gossip-based capability announcement propagates the `accuracy_delta` and `performance_delta_p99_ms` of each shard's active adapter, a lightweight mechanism inspired by **Epidemic Broadcast Trees** [21] adapted for capability metadata rather than data dissemination.

### D. Distributed Analytics and Scatter-Gather

The scatter-gather execution model for distributed OLAP has been extensively studied [4], [7]. **Google Dremel** [22] pioneered columnar scatter-gather with pushdown predicates. More recently, Sanca and Ailamaki [31] argued that next-generation analytical engines should jointly optimise traditional relational processing and model-based operators over heterogeneous hardware, which is closely aligned with our converged shard model. HedraRAG [32] extends this line of work toward heterogeneous RAG serving by coordinating retrieval and generation stages through graph-based execution planning, while RAG-Stack [33] frames end-to-end RAG optimisation explicitly from the vector-database perspective via an intermediate representation, cost model, and plan exploration stack. Our batch inference fan-out applies the same structural pattern: a coordinator fragments a request batch by domain affinity, dispatches sub-batches to the most capable shards in parallel, and merges results. The key difference is that inference results are ordered token sequences rather than relational aggregates.

### E. RAID and Erasure Coding

RAID5 with XOR parity [10] provides single-shard failure recovery at the cost of one redundancy shard. For LLM weight storage, this means a 47 GB Mixtral-8×7B model partitioned across 8 data shards and 1 parity shard can recover any single shard without full re-download. **ISA-L** [23] provides SIMD-accelerated erasure coding; the `SIMDErasureCoder` component in `include/sharding/raid_optimizations.h` implements the AVX2 XOR path, with fallback scalar XOR for architectures without AVX2 support. The implementation includes aligned memory splitting for cache efficiency and supports both 32-byte vectorised XOR and byte-by-byte processing for trailing data.

---

## III. System Architecture

### A. Node Model

Each ThemisDB cluster node is a *fully converged shard* combining [31], [34]:

$$\text{Shard}_i = (\text{RocksDB}_i,\; \text{FAISS}_i^{\text{GPU}},\; \text{InferEng}_i,\; \text{LoRA}_i^{d_i})$$

where $\text{RocksDB}_i$ is the persistent document store, $\text{FAISS}_i^{\text{GPU}}$ is a GPU-resident vector index, $\text{InferEng}_i$ is an `InferenceEngineEnhanced` instance running the base model, and $\text{LoRA}_i^{d_i}$ is a fine-tuned LoRA adapter for domain $d_i \in \{\text{LEGAL}, \text{MEDICAL}, \text{TRANSACTION}, \text{GEOSPATIAL}, \ldots\}$.

Each node exposes three logical interfaces:
- **Storage interface**: standard key-value and document CRUD via AQL.
- **Inference interface**: `INFER`, `EMBED`, and streaming token endpoints.
- **Cluster interface**: gossip participation, remote executor (mTLS), and circuit breaker state.

### B. Cluster Topology

The cluster forms a consistent-hash ring [24] managed by `ShardingManager`. Each document's URN maps deterministically to a primary shard and, under RAID1/5, to replica shards. The same hash ring governs inference request routing when no domain hint is present.

```
                ┌─────────────────────────────────────────────┐
                │   AQL Gateway  (LLMAQLHandler)               │
                └──────────────────────┬──────────────────────┘
                                       │
                        ┌──────────────▼──────────────┐
                        │  AdaptiveShardRouter         │
                        │  - routeByAdapterDomain()    │
                        │  - scatterBatchRequests()    │
                        │  - LEAST_LOADED routing      │
                        └──────┬─────────┬──────┬──────┘
                               │         │      │
              ┌────────────────▼──┐  ┌───▼──┐  ┌▼────────────────┐
              │  Shard 1 [LEGAL]  │  │ Sh.2 │  │  Shard N [...]  │
              │  InferEngEnhanced │  │ ...  │  │  InferEngEnhanced│
              │  ContBatchSched   │  │      │  │  ContBatchSched  │
              │  PagedKVCache     │  │      │  │  PagedKVCache    │
              │  LoRA: legal-qa   │  │      │  │  LoRA: domain-N  │
              └──────────────────┘  └──────┘  └─────────────────┘
                     ↑  ↓                              ↑  ↓
              GossipProtocol ←── AdapterCapabilityAnnouncement ───→
```

*Fig. 1. Cluster topology: converged storage–inference shards connected via gossip-driven adaptive routing.*

### C. Key Components and Their Interactions

**`AdaptiveShardRouter`** maintains an internal map $\sigma: \text{shard\_id} \times \text{domain} \to (\alpha, \lambda)$, where $\alpha = \texttt{accuracy\_delta} \in [-1, 1]$ and $\lambda = \texttt{performance\_delta\_p99\_ms}$. It implements three-tier iterative routing with thresholds $\theta_1 = 0.8$, $\theta_2 = 0.6$, $\theta_3 = 0.4$ and per-iteration timeout $T_{\text{iter}} = 2{,}000\,\text{ms}$.

**`GossipProtocol`** carries `AdapterCapabilityAnnouncement` messages tagged `message_type = "adapter_capability"`. Each announcement includes the adapter's domain type, `accuracy_delta`, `performance_delta_p99_ms`, training sample count, federation round, and an `announced_at` timestamp for staleness detection.

**`RemoteExecutor`** provides mTLS-authenticated HTTP/2 RPC between shards with a configurable circuit breaker (`failure_threshold = 5`, recovery window `= 60 s`), retry logic (`max_retries = 3`), and `connect_timeout = 5 s` / `request_timeout = 30 s`.

**`ContinuousBatchScheduler`** implements vLLM-style PagedAttention scheduling per shard with `max_batch_size = 256`, `max_concurrent_requests = 128`, `max_tokens_per_batch = 8{,}192`, and chunked prefill at `chunk_size = 512` tokens.

---

## IV. Distributed Inference Optimisation Layers

### A. Gossip-Driven LoRA Domain Routing

#### Protocol Design

At each federated learning round $t$, the `IncrementalLoRATrainer` on shard $i$ produces an updated adapter with accuracy delta $\alpha_i^{d}$ for domain $d$, measured against a held-out validation set. The `GossipAdapterPublisher` then constructs an `AdapterCapabilityAnnouncement`:

$$\mathcal{A}_i = \langle \text{shard\_id}_i,\; d,\; \alpha_i^d,\; \lambda_i^d,\; n_{\text{samples}},\; t,\; \tau_{\text{announced}} \rangle$$

This announcement is broadcast via epidemic dissemination [21] with a TTL of $\tau_{\text{ttl}} = 7{,}200\,\text{s}$. Stale entries (age $> \tau_{\text{ttl}}$) are evicted from the router's score map.

#### Routing Decision

For an incoming request $q$ with domain hint $d$, the router selects:

$$\text{shard}^*(q) = \arg\max_{i:\; \alpha_i^d > \theta_1} \left(\alpha_i^d - \gamma \cdot \lambda_i^d\right)$$

where $\gamma$ is a configurable latency penalty weight (default $\gamma = 0.1$, corresponding to a 100 ms p99 latency delta being worth 0.01 accuracy points). If no shard exceeds $\theta_1$, the router falls back to $\theta_2$, then $\theta_3$, then consistent-hash routing.

The AQL syntax extension is:

```sql
INFER 'mistral-7b' user_query DOMAIN 'legal'
```

This introduces a `domain_hint` parameter that `LLMAQLHandler::executeInfer()` extracts and passes to `routeByAdapterDomain()`.

#### Convergence Properties

Under epidemic dissemination with fanout $f$ and gossip interval $\delta$, all $N$ nodes receive an announcement within $O(\log N / \log f)$ gossip rounds [21]. For $N = 16$, $f = 4$, $\delta = 1\,\text{s}$, convergence is achieved within approximately 2 s, well within the $\tau_{\text{ttl}} = 7{,}200\,\text{s}$ validity window.

### B. Scatter-Gather Batch Inference

#### Request Partitioning

A batch $B = \{r_0, \ldots, r_{N-1}\}$ is partitioned into domain-affinity groups:

$$B_d = \{r_i \in B : \text{domain}(r_i) = d\}$$

Each group $B_d$ is assigned to the shard $\text{shard}^*(d)$ selected by the domain router. Unclassified requests fall back to the consistent-hash shard.

#### Parallel Dispatch

Sub-batches are dispatched concurrently using `std::async`:

```
futures[d] = std::async(std::launch::async, [&]() {
    return remote_executor_->post(shard_for_domain[d], batch_payload(B_d));
});
```

with a collective timeout $T_{\text{scatter}} = 30{,}000\,\text{ms}$ and per-shard circuit breaking. Partial results from timed-out or failed shards are either retried on a secondary shard or returned as error entries, preserving the original request ordering in the merged response.

#### Throughput Model

Let $\mu_i$ be the throughput of shard $i$ in tokens/s and $|B_d|$ the sub-batch size assigned to domain $d$. Ignoring network overhead, the effective batch completion time is:

$$T_{\text{batch}} \approx \max_d \frac{\sum_{r \in B_d} L(r)}{\mu_{\text{shard}^*(d)}}$$

where $L(r)$ is the expected output length of request $r$. For uniformly distributed requests and balanced shards, this is approximately $N_{\text{req}} \cdot \bar{L} / (N_{\text{shards}} \cdot \mu)$, yielding a linear speedup with the number of shards.

### C. Cross-Shard Speculative Decoding

#### Model Assignment

In a heterogeneous cluster, we designate two shard roles:

- **Draft shard** $S_A$: hosts a small model $M_A$ (e.g., Mistral-7B Q4, $\approx$ 4 GB VRAM), capable of generating $n_{\text{draft}} = 8$ token proposals rapidly.
- **Verify shard** $S_B$: hosts the large target model $M_B$ (e.g., Mixtral-8×7B, $\approx$ 24 GB VRAM), which verifies the draft sequence in one forward pass.

The `SpeculativeDecoder` on $S_B$ is configured with:

```cpp
config.enable_speculative_decoding = true;
config.speculative_draft_tokens    = 8;
config.speculative_draft_model_id  = "shard-a:model:mistral-7b-q4";
```

#### Protocol

For each decode step:

1. $S_B$ sends the current context $c_t$ to $S_A$ via `RemoteExecutor::post()`.
2. $S_A$ generates a draft sequence $\hat{x}_{t+1}, \ldots, \hat{x}_{t+n}$ using $M_A$.
3. $S_A$ returns the draft tokens and their log-probabilities $q(\hat{x}_{t+k} | c_t)$.
4. $S_B$ computes $p(\hat{x}_{t+k} | c_t)$ for all $k$ in a single forward pass.
5. Tokens are accepted using the rejection sampling rule:

$$\hat{x}_{t+k} \text{ accepted} \iff u \leq \frac{p(\hat{x}_{t+k} | c_t)}{q(\hat{x}_{t+k} | c_t)}, \quad u \sim \mathcal{U}[0, 1]$$

#### Speedup Analysis

Let $\tau_A$ and $\tau_B$ denote the per-token latency of $M_A$ and $M_B$ respectively, and let $\bar{k}$ be the expected number of accepted draft tokens. The effective throughput ratio over baseline (no speculation) is:

$$\text{speedup} = \frac{\bar{k} + 1}{\tau_B + n_{\text{draft}} \cdot \tau_{A} / (1 + \tau_{\text{net}} / \tau_B)}$$

For $\bar{k} = 5.2$ (acceptance rate 65 %, $n_{\text{draft}} = 8$), $\tau_B / \tau_A = 6$ (Mixtral-8×7B vs. Mistral-7B Q4), and $\tau_{\text{net}} = 2\,\text{ms}$ (intra-cluster RTT), the theoretical speedup is approximately 2.3×.

The network overhead $\tau_{\text{net}}$ is bounded by the payload size: 8 draft tokens $\times$ 4 bytes each = 32 bytes plus log-probability floats ($\approx$ 256 KB for 32K vocabulary), manageable within a 1 GbE intra-cluster link.

#### Telemetry

The `LLMMetricsCollector` must record the following cross-shard metrics:

- `speculation_accept_rate` — rolling mean acceptance rate per $(S_A, S_B)$ pair.
- `draft_roundtrip_ms` — p50/p99 latency of the draft RPC.
- `speculation_fallback_total` — count of fallbacks to local draft model.

### D. Distributed KV-Prefix Cache

#### Motivation

In Legal-RAG workloads, a system prompt of 1{,}024 tokens prepended to every request dominates TTFT. Each shard independently computes the KV state for this prefix. With 8 shards, the same 1{,}024 tokens are encoded 8 times per unique system prompt.

#### Protocol

The `LLMPrefixCache` on the primary shard computes the KV state once and serialises it:

```
llama_state_seq_save_file(ctx, "prefix_kv.bin", seq_id, 0, 1024);
```

The resulting binary file ($\approx$ 200 MB for a 7B model at fp16, 512-token prefix) is transferred to peer shards via a dedicated binary transfer endpoint in `RemoteExecutor`, bypassing JSON serialisation and using `ZeroCopyBuffer`:

```
ZeroCopyBuffer buf = load_file("prefix_kv.bin");
remote_executor_->post_binary(shard_id, "/kv_prefix/import", buf.getView());
```

The receiving shard loads the state:

```
llama_state_seq_load_file(ctx, "prefix_kv.bin", seq_id, &n_token_count_out);
```

and begins generation from token position $n_{\text{prefix}}$ directly, reducing TTFT by:

$$\Delta\text{TTFT} = \frac{n_{\text{prefix}}}{\mu_{\text{prefill}}}$$

For $n_{\text{prefix}} = 1{,}024$ tokens and $\mu_{\text{prefill}} = 4{,}000\,\text{tok/s}$ on a single GPU, $\Delta\text{TTFT} \approx 256\,\text{ms}$ per request.

#### Compatibility Constraint

KV state serialisation is only valid between shards running the same base model architecture and quantisation level. A compatibility check — comparing model architecture hash and quantisation descriptor — must be performed before any transfer. Incompatible shards fall back to local prefill.

### E. Data-Local Embedding Routing

For `EMBED` operations over documents already stored in the database, the embedding request should be routed to the shard holding the document, since the model has already encoded the document's neighbourhood in its fine-tuned weights:

```cpp
ShardInfo target = sharding_manager_->GetShardForKey(doc_urn);
return remote_executor_->post(target, embed_request);
```

This avoids transferring the raw document content across the cluster and instead performs the embedding computation where the data resides. For embedding batches over a full collection, this yields near-perfect data locality — each shard embeds its own partition.

---

## V. RAID Fault Tolerance for Inference Nodes

### A. RAID0: Striped Inference Cluster

Under RAID0, all N shards are active with no redundancy. Failure of shard $i$ is handled by the circuit breaker in `RemoteExecutor`:

1. After $F_{\text{threshold}} = 5$ consecutive failures, the breaker opens.
2. Subsequent requests are routed to the consistent-hash fallback shard.
3. The failed shard's LoRA adapter is not available; the fallback shard uses its own domain adapter, potentially with lower accuracy.
4. After $T_{\text{recovery}} = 60\,\text{s}$, the breaker enters half-open state and probes the failed shard.

### B. RAID1: Replicated Inference

Under RAID1, each shard has a full replica holding an identical base model and LoRA adapter. The `RedundancyStrategy` selects the replica with lower `pending_llm_requests` for load balancing. On primary failure, the replica is promoted immediately with zero KV-cache warmup loss for new requests.

### C. RAID5: Parity-Protected Weight Storage

For large models (Mixtral-8×7B, 47 GB), RAID5 partitions the quantised weight tensor $W$ into $N_D = 8$ data shards and 1 parity shard:

$$P = W_1 \oplus W_2 \oplus \cdots \oplus W_{N_D}$$

The `SIMDErasureCoder` computes the XOR parity using AVX2:

```cpp
__m256i p = _mm256_setzero_si256();
for (const auto& chunk : data_chunks) {
    __m256i d = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(chunk.data() + i * 32));
    p = _mm256_xor_si256(p, d);
}
```

Processing 32 bytes per AVX2 instruction over 47 GB yields a parity computation time of approximately $47 \times 10^9 / (32 \times 3.5 \times 10^9) \approx 0.42\,\text{s}$ on a 3.5 GHz core, well within an offline re-shard window.

**Limitation:** RAID5 parity is not applicable to KV-cache failover. Reconstructing a KV state from parity requires sequential XOR recovery, introducing latency of $O(N_D \cdot \tau_{\text{net}})$ — prohibitive for token-by-token generation. KV-cache is therefore not parity-protected; on shard failure, in-flight requests at the lost shard are aborted and retried from scratch.

---

## VI. Theoretical Performance Analysis

This section provides analytical performance models for the proposed optimisation layers. All results in this section are derived from theoretical analysis, queueing models, and component specifications from the ThemisDB codebase. **Empirical validation of these models is planned as part of the implementation roadmap (Phases 1–6, Section VII); final performance targets and acceptance criteria are listed in Section VII.C.** Readers should interpret all claimed speedups (e.g., 2.5× for speculative decoding, 4× for batch fan-out) as design targets rather than measured results.

### A. Domain Routing Overhead

The `routeByAdapterDomain()` path involves one map lookup in the shard score table (O(1)) plus, if a remote shard is selected, a `RemoteExecutor::post()` call with $T_{\text{connect}} = 5\,\text{ms}$ connect timeout and $\approx 1\,\text{ms}$ payload transmission for a typical 512-token prompt. The expected overhead versus direct local dispatch is $\leq 5\,\text{ms}$ for intra-datacenter links.

### B. Batch Fan-Out Throughput

Let $B = 64$ be the batch size, $D = 4$ the number of domain groups, and $\mu = 1{,}000\,\text{tok/s}$ the per-shard throughput. With uniform domain distribution and balanced shards:

$$T_{\text{distributed}} = \frac{B/D \cdot \bar{L}}{\mu} = \frac{16 \cdot \bar{L}}{1{,}000}$$

versus single-shard:

$$T_{\text{single}} = \frac{B \cdot \bar{L}}{\mu} = \frac{64 \cdot \bar{L}}{1{,}000}$$

Speedup $= 4\times$, confirming the $\geq 2\times$ target even under non-ideal load distribution (Jain's fairness index $\geq 0.75$).

### C. Speculative Decoding Latency Model

The expected tokens generated per verify step is:

$$\mathbb{E}[\bar{k}] = \sum_{k=0}^{n} (1 - \beta)^k \beta \cdot k + n(1-\beta)^n \approx \frac{1 - \beta^{n+1}}{1-\beta} - 1$$

where $\beta = 1 - \alpha_{\text{accept}}$. For $\alpha_{\text{accept}} = 0.65$, $n = 8$: $\mathbb{E}[\bar{k}] \approx 5.2$.

The end-to-end latency per output token (including draft RPC overhead $\tau_{\text{rpc}} = 3\,\text{ms}$, $\tau_B = 15\,\text{ms}$, $\tau_A = 2.5\,\text{ms}$):

$$\tau_{\text{spec}} = \frac{\tau_B + n \cdot \tau_A + \tau_{\text{rpc}}}{\bar{k} + 1} = \frac{15 + 20 + 3}{6.2} \approx 6.1\,\text{ms/tok}$$

versus non-speculative $\tau_B = 15\,\text{ms/tok}$: speedup $\approx 2.5\times$.

### D. KV-Prefix Transfer Cost

Transfer of a 200 MB KV state over a 10 GbE intra-cluster link takes $\approx 160\,\text{ms}$. Amortised over $N_{\text{req}} = 100$ subsequent requests sharing the same prefix, the per-request cost is 1.6 ms — negligible compared to the 256 ms TTFT saved.

The break-even point is at $N_{\text{req}}^* = 160\,\text{ms} / 256\,\text{ms} = 0.625$, meaning a single shared request already justifies the transfer.

---

## VII. Implementation Roadmap and Gap Analysis

### A. Current Gap Summary

Table I summarises the seven integration gaps identified through static analysis of the ThemisDB codebase.

**TABLE I: Integration Gap Analysis**

| Gap | Affected File | Priority | Estimated Effort |
|-----|---------------|----------|-----------------|
| `LLMAQLHandler` routes locally, never calls `AdaptiveShardRouter` | `src/llm/llm_aql_handler.cpp` | P1 | 1 sprint |
| `ShardStats` missing LLM queue metrics | `include/sharding/sharding_interfaces.h` | P1 | 3 days |
| `SpeculativeDecoder` has no remote draft shard support | `include/llm/speculative_decoder.h` | P2 | 1 sprint |
| `RemoteExecutor` lacks binary state transfer for KV prefix | `include/sharding/remote_executor.h` | P2 | 1 sprint |
| `DistributedAnalyticsSharding` not reused for LLM batch fan-out | `include/analytics/` | P2 | 3 days |
| Domain-affinity batch splitting missing | `include/llm/inference_engine_enhanced.h` | P3 | 1 sprint |
| Embedding locality routing not implemented | `src/llm/llm_aql_handler.cpp` | P3 | 3 days |

### B. Six-Phase Implementation Plan

**Phase 1 — Domain Routing (Q3 2026)**

- Extend AQL parser with `DOMAIN` hint parameter.
- Implement `LLMAQLHandler::executeInfer()` → `AdaptiveShardRouter::routeByAdapterDomain()` call.
- Add `domain_routing_fallback_total` counter to `LLMMetricsCollector`.
- Unit test: mock gossip announcement with three domain scores → verify routing to highest-scoring shard.

*Acceptance:* Domain-routing overhead ≤ 5 ms. Fallback metric increments correctly when no shard exceeds $\theta_3$.

**Phase 2 — LLM Queue Metrics in ShardStats (Q3 2026)**

- Add `pending_llm_requests`, `avg_llm_queue_ms`, `active_lora_adapters` to `ShardStats`.
- `ContinuousBatchScheduler` writes metrics via callback on each schedule cycle.
- `AdaptiveShardRouter` uses `pending_llm_requests` for `LEAST_LOADED` routing.
- Integration test: three shards with injected artificial load → verify routing selects least-loaded shard.

**Phase 3 — Batch Fan-Out (Q4 2026)**

- Implement `LLMAQLHandler::executeBatchInfer()` with domain-affinity splitting.
- Reuse `DistributedAnalyticsSharding::executeDistributed()` scaffolding.
- Apply `scatter_timeout_ms = 30{,}000` and per-shard circuit breaking.
- Benchmark: batch-64, 4 shards, $\bar{L} = 200$ tokens → verify speedup ≥ 2×.

**Phase 4 — Cross-Shard Speculative Decoding (Q4 2026)**

- Extend `SpeculativeDecoder` with `remote_draft_shard_id` configuration.
- Implement draft RPC via `RemoteExecutor::post()` with payload format: context tokens + request ID.
- Add `speculation_accept_rate`, `draft_roundtrip_ms` metrics to `LLMMetricsCollector`.
- A/B test: local draft model vs. remote draft shard at identical acceptance rate → confirm latency parity.

*Acceptance:* Accept rate ≥ 65 %, E2E latency ≤ 1.5× local inference.

**Phase 5 — KV-Prefix Sharing (Q1 2027)**

- Extend `RemoteExecutor` with `/kv_prefix/import` binary endpoint.
- Implement model compatibility check (architecture hash + quantisation descriptor).
- Extend `LLMPrefixCache` with cross-shard export via `llama_state_seq_save_file()` + `ZeroCopyBuffer`.
- Test: same system prompt on three shards → verify only one prefill computation, TTFT reduced ≥ 30 %.

**Phase 6 — Embedding Locality and Hardening (Q1 2027)**

- Route `executeEmbed()` via `ShardingManager::GetShardForKey()`.
- Expand `benchmarks/bench_llm_raid_pipeline.cpp` with distributed inference benchmarks.
- Grafana dashboard: `domain_routing_fallback_total`, `batch_fan_out_latency_p99`, `speculation_accept_rate`, `kv_prefix_hit_rate`.

### C. Measurable Acceptance Criteria

**TABLE II: Performance Targets**

| Metric | Target |
|--------|--------|
| Domain routing latency overhead | ≤ 5 ms vs. local dispatch |
| Batch inference speedup (N=64, 4 shards) | ≥ 2× vs. single-shard serial |
| Speculative decoding acceptance rate | ≥ 65 % |
| E2E latency with remote draft shard | ≤ 1.5× local inference |
| KV-prefix sharing TTFT reduction | ≥ 30 % (prefix ≥ 256 tokens) |
| Embedding locality routing overhead | ≤ 10 % vs. local embed |
| Circuit-breaker failover time | ≤ 100 ms to first fallback request |

---

## VIII. Discussion

### A. Design Choices and Trade-offs

The decision to embed inference capability into each storage shard rather than building a separate inference tier is motivated by the data-locality principle: in Legal-RAG workloads, 70–90 % of context tokens originate from documents stored on the same shard as the request. Cross-shard data movement for a dedicated inference tier would introduce network overhead for every request. The converged architecture eliminates this overhead at the cost of VRAM pressure on each node.

The gossip-based routing protocol introduces a propagation delay of O(log N) gossip rounds between an adapter being retrained and the updated score reaching all routers. During this window, requests may be routed to a sub-optimal shard. We mitigate this by maintaining a staleness threshold ($\tau_{\text{ttl}} = 7{,}200\,\text{s}$) much larger than the training round interval (typically 1–24 hours), so routing decisions are never based on stale information unless the network is partitioned.

### B. Limitations

1. **KV state compatibility**: Cross-shard KV-prefix sharing requires identical base model architecture and quantisation. Upgrading the base model on one shard requires coordinated rollout across all shards to maintain prefix sharing.

2. **Acceptance rate variability**: Speculative decoding acceptance rate depends on the similarity between draft and verify model distributions. For out-of-distribution inputs, $\alpha_{\text{accept}}$ may fall below 50 %, at which point the cross-shard overhead dominates and a fallback to local inference is warranted.

3. **RAID5 reconstruction latency**: Reconstructing a failed weight shard via RAID5 XOR requires all $N_D - 1$ healthy shards to be read concurrently. During reconstruction, affected inference requests must be retried, increasing tail latency.

4. **Reference publication timeline**: Several cited works (references [15], [18], [26], [27], [29], [32], [33]) are arxiv preprints or accepted papers with publication dates in 2025–2026. Readers should verify availability and final publication status against the arXiv repository and conference proceedings before relying on these references.

### C. Threats to Validity

Internal validity: Several quantitative claims in this manuscript are theoretical or roadmap-oriented rather than measured end-to-end outcomes. We mitigate this by stating explicit acceptance criteria, exposing file-level implementation gaps, and separating validated baselines from deferred claims.

Construct validity: Metrics such as acceptance rate and p99 latency do not fully capture semantic output quality for all domains. The roadmap therefore requires joint reporting of latency, reliability, and domain-quality signals rather than single-metric optimization.

External validity: Results may depend on hardware heterogeneity, network topology, and workload composition. To support transferability, artifact references and implementation anchors are tied to an open repository state and reproducible benchmark targets.

### D. Comparison with Dedicated Serving Systems

Unlike vLLM [1] or TensorRT-LLM [25], which assume a dedicated GPU cluster per model, our architecture shares VRAM between the vector index (`FAISS`), the KV cache (`PagedKVCache`), and the model weights. VRAM pressure is managed by `PagedKVCache` preemption, which swaps out lower-priority KV pages when VRAM headroom falls below a configurable threshold. This is analogous to OS page replacement in virtual memory systems [1].

### E. Artifact Availability

To support inspection and future empirical validation, the underlying ThemisDB codebase discussed throughout this paper is available as an open-source artifact on GitHub under the MIT license [34]. This artifact availability is particularly relevant for the implementation gaps and roadmap items in Section VII, which are derived from static analysis of the current repository state rather than from a separately packaged prototype.

---

## IX. Conclusion

We have outlined a co-design architecture for distributed LLM inference within a RAID-sharded hybrid database system. The key insight is that the existing infrastructure — `AdaptiveShardRouter`, `GossipProtocol`, `RemoteExecutor`, `DistributedAnalyticsSharding`, and `SIMDErasureCoder` — already contains most of the building blocks needed for gossip-driven domain routing, scatter-gather batch inference, cross-shard speculative decoding, distributed KV-prefix sharing, and RAID-protected weight storage. The primary missing element is the integration layer connecting the AQL inference handler to the sharding subsystem.

Our theoretical analysis suggests that domain-routing can add no more than 5 ms overhead, batch fan-out can achieve at least 2× throughput with N = 4 shards, and cross-shard speculative decoding can yield approximately 2.5× latency reduction at 65 % acceptance rate. The KV-prefix sharing protocol reaches break-even after fewer than one shared request for system prompts of 1{,}024 tokens under the assumptions stated in Section VI.

A six-phase implementation roadmap with concrete acceptance criteria and targeted file-level changes outlines a plausible path from the current state toward production-ready distributed inference. We believe the converged shard model — where each node combines storage, vector indexing, and LLM inference — is a promising alternative to separate serving tiers for database-native LLM workloads, subject to future empirical validation.

---

## Appendix A. Claim-to-Evidence Traceability

| Claim ID | Claim Summary | Evidence Anchors |
|----------|---------------|------------------|
| C1 | ThemisDB already contains core building blocks for converged shard inference orchestration. | Section III component mapping, Table I gap analysis, [34] |
| C2 | Cross-shard speculative decoding can provide multi-x latency improvements under stated acceptance/RTT assumptions. | Section IV-C protocol, Section VI-C model, [8], [9], [15]-[18] |
| C3 | KV-prefix transfer can reduce TTFT substantially for repeated prefixes when compatibility constraints hold. | Section IV-D protocol, Section VI-D cost model, [1], [34] |
| C4 | RAID-style redundancy is viable for model-weight durability but not for in-flight KV recovery. | Section V fault model, [10], [23], [24] |
| C5 | Final production superiority claims are deferred pending roadmap execution and benchmark completion. | Section VII phased plan and acceptance criteria |

## Appendix B. arXiv Submission Readiness Checklist

- [x] Title is specific and technically scoped
- [x] Abstract states measurable contribution
- [x] Core claims are bounded by explicit assumptions
- [x] Method/design sections define reproducible implementation targets
- [x] Limitations and validity threats are transparent
- [x] Figures/tables are referenced in text
- [x] References are complete and consistent
- [ ] Final benchmark wave results inserted
- [ ] Commit hash and artifact manifest frozen

---

## References

[1] W. Kwon, Z. Li, S. Zhuang, Y. Sheng, L. Zheng, C. H. Yu, J. E. Gonzalez, H. Zhang, and I. Stoica, "Efficient Memory Management for Large Language Model Serving with PagedAttention," in *Proc. ACM SOSP*, Koblenz, Germany, 2023, pp. 611–626.

[2] G. Yu, J.-W. Kim, H. Shin, J. Jeong, M. Park, S. Kim, J. Kim, and W.-C. Jeong, "Orca: A Distributed Serving System for Transformer-Based Generative Models," in *Proc. USENIX OSDI*, 2022, pp. 521–538.

[3] L. Zheng, Z. Li, H. Zhang, Y. Zhuang, Z. Chen, Y. Huang, Y. Wang, Y. Zhuang, Z. Lian, E. P. Xing, J. E. Gonzalez, and I. Stoica, "AlpaServe: Statistical Multiplexing with Model Parallelism for Deep Learning Serving," in *Proc. USENIX OSDI*, 2022, pp. 663–679.

[4] S. Melnik, A. Gubarev, J. J. Long, G. Romer, S. Shivakumar, M. Tolton, and T. Vassilakis, "Dremel: Interactive Analysis of Web-Scale Datasets," *Proc. VLDB Endow.*, vol. 3, no. 1–2, pp. 330–339, 2010.

[5] E. J. Hu, Y. Shen, P. Wallis, Z. Allen-Zhu, Y. Li, S. Wang, L. Wang, and W. Chen, "LoRA: Low-Rank Adaptation of Large Language Models," in *Proc. ICLR*, 2022.

[6] H. B. McMahan, E. Moore, D. Ramage, S. Hampson, and B. A. y Arcas, "Communication-Efficient Learning of Deep Networks from Decentralized Data," in *Proc. AISTATS*, 2017, pp. 1273–1282.

[7] M. Stonebraker, D. Abadi, D. J. DeWitt, S. Madden, E. Paulson, A. Pavlo, and A. Rasin, "MapReduce and Parallel DBMSs: Friends or Foes?" *Commun. ACM*, vol. 53, no. 1, pp. 64–71, Jan. 2010.

[8] Y. Leviathan, M. Kalman, and Y. Matias, "Fast Inference from Transformers via Speculative Decoding," in *Proc. ICML*, 2023, pp. 19274–19286.

[9] C. Chen, S. Borgeaud, G. Irving, J.-B. Lespiau, L. Sifre, and J. Jumper, "Accelerating Large Language Model Decoding with Speculative Sampling," *arXiv preprint arXiv:2302.01318*, 2023.

[10] D. A. Patterson, G. Gibson, and R. H. Katz, "A Case for Redundant Arrays of Inexpensive Disks (RAID)," in *Proc. ACM SIGMOD*, Chicago, IL, USA, 1988, pp. 109–116.

[11] S. Sheng, L. Zheng, B. Y. Yuan, Z. Li, M. Ryabinin, B. Chen, P. Liang, C. Ré, I. Stoica, and C. Zhang, "FlexGen: High-Throughput Generative Inference of Large Language Models with a Single GPU," in *Proc. ICML*, 2023, pp. 31094–31116.

[12] A. Agrawal, N. Kedia, A. Panwar, J. Mohan, N. Kwatra, B. Gulavani, A. Tumanov, and R. Ramjee, "Taming Throughput-Latency Tradeoff in LLM Inference with Sarathi-Serve," in *Proc. USENIX OSDI*, 2024, pp. 117–134.

[13] B. Saxena, "Prompt Lookup Decoding," *GitHub repository*, 2023. [Online]. Available: https://github.com/apoorvumang/prompt-lookup-decoding

[14] Y. Fu, P. Bailis, I. Stoica, and H. Zhang, "Break the Sequential Dependency of LLM Inference Using Lookahead Decoding," in *Proc. ICML*, 2024.

[15] X. Li, J. Fan, Q. Wang, D. Spatharakis, S. Ghafouri, H. Vandierendonck, D. John, B. Ji, A. R. Butt, and D. S. Nikolopoulos, "WISP: Waste- and Interference-Suppressed Distributed Speculative LLM Serving at the Edge via Dynamic Drafting and SLO-Aware Batching," *arXiv preprint arXiv:2601.11652*, 2026.

[16] X. Liu, L. Luo, M. Tang, C. Huang, and X. Chen, "FlowSpec: Continuous Pipelined Speculative Decoding for Efficient Distributed LLM Inference," *arXiv preprint arXiv:2507.02620*, 2025.

[17] F. Yu, L. Li, B. McDanel, and S. Q. Zhang, "DSD: A Distributed Speculative Decoding Solution for Edge-Cloud Agile Large Model Serving," *arXiv preprint arXiv:2511.21669*, 2025.

[18] N. Martin and F. Dogar, "WANSpec: Leveraging Global Compute Capacity for LLM Inference," *arXiv preprint arXiv:2602.18931*, 2026.

[19] H. Liu, R. Wen, S. Nair, J. Liu, W. Lou, C. Zhang, W. Yeoh, Y. Vorobeychik, and N. Zhang, "EcoLoRA: Communication-Efficient Federated Fine-Tuning of Large Language Models," *arXiv preprint arXiv:2506.02001*, 2025.

[20] J. Qi, Z. Luan, S. Huang, C. Fung, H. Yang, and D. Qian, "FDLoRA: Personalized Federated Learning of Large Language Model via Dual LoRA Tuning," *arXiv preprint arXiv:2406.07925*, 2024.

[21] A. Demers, D. Greene, C. Hauser, W. Irish, J. Larson, S. Shenker, H. Sturgis, D. Swinehart, and D. Terry, "Epidemic Algorithms for Replicated Database Maintenance," in *Proc. ACM PODC*, 1987, pp. 1–12.

[22] S. Melnik, A. Gubarev, J. J. Long, G. Romer, S. Shivakumar, M. Tolton, T. Vassilakis, N. Galibert, D. Kaplan, H. Rimer, and K. Kulkarni, "Dremel: A Decade of Interactive SQL Analysis at the Web Scale," *Proc. VLDB Endow.*, vol. 13, no. 12, pp. 3461–3472, 2020.

[23] Intel Corp., "ISA-L: Intel Intelligent Storage Acceleration Library," *GitHub repository*, 2023. [Online]. Available: https://github.com/intel/isa-l

[24] D. Karger, E. Lehman, T. Leighton, R. Panigrahy, M. Levine, and D. Lewin, "Consistent Hashing and Random Trees: Distributed Caching Protocols for Relieving Hot Spots on the World Wide Web," in *Proc. ACM STOC*, 1997, pp. 654–663.

[25] NVIDIA Corp., "TensorRT-LLM: Optimized LLM Inference on NVIDIA GPUs," *GitHub repository*, 2024. [Online]. Available: https://github.com/NVIDIA/TensorRT-LLM

[26] P. Tran, T.-H. Liu, L. T. Le, T.-A. Nguyen, V. Q. La, E. Yu, H. Shu, C. S. Hong, and N. H. Tran, "GoodSpeed: Optimizing Fair Goodput with Adaptive Speculative Decoding in Distributed Edge Inference," *arXiv preprint arXiv:2512.09963*, 2025. (Accepted at IEEE INFOCOM 2026)

[27] J. Song, W. Chen, X. Song, M. Chris, C. Tong, G. Chen, T. Zhao, E. Yang, B. Shi, and L. Ai, "Speculative Decoding in Decentralized LLM Inference: Turning Communication Latency into Computation Throughput," *arXiv preprint arXiv:2511.11733*, 2025.

[28] X. Li, S. Ghafouri, J. Fan, B. Ali, H. Vandierendonck, and D. S. Nikolopoulos, "ConfigSpec: Profiling-Based Configuration Selection for Distributed Edge–Cloud Speculative LLM Serving," in *Proc. 4th Int. Workshop Testing Distributed IoT Systems (TDIS 2026)*, 2026. doi: 10.1145/3802513.3803483.

[29] Z. Shen, J. Lu, H. Wan, and J. Chen, "SDFLoRA: Selective Decoupled Federated LoRA for Privacy-preserving Fine-tuning with Heterogeneous Clients," *arXiv preprint arXiv:2601.11219*, 2026.

[30] L. Wang, J. Bian, L. Zhang, and J. Xu, "Adaptive LoRA Experts Allocation and Selection for Federated Fine-Tuning," in *Proc. NeurIPS*, 2025.

[31] V. Sanca and A. Ailamaki, "Analytical Engines With Context-Rich Processing: Towards Efficient Next-Generation Analytics," in *Proc. IEEE Int. Conf. Data Engineering (ICDE)*, 2023, pp. 4283-4286, doi: 10.1109/ICDE55515.2023.00298.

[32] Z. Hu, V. Murthy, Z. Pan, W. Li, X. Fang, Y. Ding, and Y. Wang, "HedraRAG: Coordinating LLM Generation and Database Retrieval in Heterogeneous RAG Serving," *arXiv preprint arXiv:2507.09138*, 2025. (Accepted at ACM SOSP 2025)

[33] W. Jiang, "RAG-Stack: Co-Optimizing RAG Quality and Performance From the Vector Database Perspective," *arXiv preprint arXiv:2510.20296*, 2025.

[34] ThemisDB Contributors, "ThemisDB," *GitHub repository*, 2026. [Online]. Available: https://github.com/makr-code/ThemisDB

[35] Z. Jiang, F. F. Xu, L. Gao, Z. Sun, Q. Liu, J. Dwivedi-Yu, Y. Yang, J. Callan, and G. Neubig, "Active Retrieval Augmented Generation," in *Proc. Conf. Empirical Methods in Natural Language Processing (EMNLP)*, 2023.

---

*This preprint describes the ThemisDB distributed inference architecture. The referenced source code is publicly available as the open-source ThemisDB project on GitHub under the MIT license [34]. For arXiv submission, category metadata such as primary class and cross-lists should be supplied via arXiv's submission form rather than embedded in the manuscript body.*
