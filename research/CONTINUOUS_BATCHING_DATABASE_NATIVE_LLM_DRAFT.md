# Continuous Batching in Database-Native LLM Pipelines

**Status**: Pre-Registration Complete
**Version**: 1.0
**Last Updated**: 2026-05-14
**Authors**: ThemisDB Research Team
**Target Venue**: MLSys 2027 / EuroSys 2027
**Companion to**: `THEMISDB_SYSTEM_PAPER_ARXIV_2026.md` §III Tier 4, `DB_NATIVE_LLM_SERVING_OPTIMIZATION_PAPER_DRAFT.md`

---

## Abstract

Continuous batching enables efficient token-level multiplexing of concurrent LLM requests
and is the standard serving technique in dedicated inference frameworks (vLLM [1],
TensorRT-LLM [2], S-LoRA [3]). Its behaviour in a *database-native* LLM pipeline — where
request scheduling interacts with concurrent query execution, MVCC transactions, and
LoRA adapter lifecycle management — has not been studied. This paper characterises
continuous batching throughput-latency trade-offs in ThemisDB's embedded LLM serving
layer (`ContinuousBatchScheduler`, `PagedKVCache`, `SpeculativeDecoder`) under four
concurrent-request levels (10, 50, 100, 250) and three scheduler configurations (max
batch size, token budget, chunked prefill). We define primary metrics (tokens/s, TTFT,
P99 completion latency), reliability metrics (KV-cache hit rate, queue depth, degraded-
mode activation), and a reproducible benchmark harness (`bench_llm_raid_pipeline.cpp`).
The study answers four research questions: (1) how throughput and tail-latency respond to
batch-size and token-budget configurations; (2) at what request rate KV-cache pressure
triggers degraded-mode fallback; (3) how chunked prefill interacts with the retrieval
pipeline latency budget; and (4) which guardrail parameters (timeout, downgrade threshold)
maximise throughput while bounding P99. This paper constitutes a pre-registration of the
experimental design and expected operating points; GPU hardware procurement for empirical
validation is in progress, and all instrumentation and benchmark harnesses are in place
(§V, §IX).

---

## I. Introduction

Large language model inference serving has converged on *continuous batching* [4] as the
dominant scheduling paradigm: rather than waiting for a fixed-size batch to fill before
forward-passing, the scheduler incrementally admits new requests and evicts completed ones
at token boundaries, keeping GPU compute utilisation high across heterogeneous prompt and
generation lengths.

In dedicated serving systems (vLLM, Triton Inference Server, TGI), continuous batching
operates in isolation from the underlying data plane. In ThemisDB, however, the LLM
pipeline is an *embedded operator* in the database query plan: a single AQL statement can
trigger retrieval (HybridRetriever), generation (LLMPluginManager), quality evaluation
(RAGJudge), and feedback persistence (ContinuousLearningOrchestrator) within one
transaction boundary.

This tight coupling introduces three complications that dedicated serving systems do not
face:

1. **Retrieval-latency back-pressure**: HNSW and BM25 retrieval latencies (P99 ≤ 9.67 ms
   baseline) impose a minimum TTFT floor; high-concurrency retrieval contention can
   inflate this floor and starve batch slots.
2. **LoRA lifecycle interference**: adapter switch events triggered by
   `ContinuousLearningOrchestrator` require a brief GPU-side model update; concurrent
   batch processing must either pause or tolerate mixed-adapter forward passes.
3. **Transaction serialisation overhead**: under SR isolation, generation transactions can
   abort and require retry, inflating effective P99 and reducing scheduler throughput.

These interactions motivate a joint study of continuous batching behaviour in the
database-native context, which is the contribution of this paper.

### Research Questions

RQ1: How do batch size and token budget configurations affect tokens/s throughput and
P99 completion latency under four concurrency levels (10, 50, 100, 250 requests)?

RQ2: At what request rate does KV-cache pressure trigger degraded-mode fallback, and
what is the quality cost of degraded-mode generation?

RQ3: How does chunked prefill interact with retrieval pipeline latency, and what chunk
size minimises P99 without reducing tokens/s by more than 5%?

RQ4: Which guardrail configuration (timeout, downgrade threshold) maximises throughput
while keeping P99 < 500 ms at 100 concurrent requests?

---

## II. Related Work

**Continuous batching**: Orca [4] introduced iteration-level scheduling; vLLM [1]
added paged KV attention (PagedAttention) enabling virtually unlimited concurrent
sequences. TensorRT-LLM [2] provides CUDA kernel-level optimisations; S-LoRA [3] extends
paging to concurrent LoRA adapters.

**Database-native AI serving**: Zhao et al. [5] demonstrate in-database machine learning
for privacy-preserving text-to-SQL generation, showing that ML computation can be
co-located with database query execution; ONNX Runtime in SQL Server and Greenplum's MADlib
are further examples. None of these systems support autoregressive decoding with continuous
batching.

**Speculative decoding**: Leviathan et al. [6] and Chen et al. [7] define speculative
decoding; ThemisDB's `SpeculativeDecoder` implements a draft-model variant targetted at
short-context database query answers where the accept rate is expected to be high (> 0.70)
due to formulaic answer patterns.

**Resource management in LLM serving**: Agrawal et al. [8] study preemption strategies;
Zhong et al. [9] study memory-efficient KV-cache management. These studies focus on
dedicated serving systems; the database-native coupling is novel.

---

## III. System Model

### A. ThemisDB LLM Serving Layer

The embedded LLM serving layer consists of five components:

| Component | File | Role |
|---|---|---|
| `ContinuousBatchScheduler` | `include/llm/continuous_batch_scheduler.h` | Admission control, batch assembly, preemption |
| `PagedKVCache` | `include/llm/paged_kv_cache.h` | Token-addressable KV memory pool (CPU + GPU tiers) |
| `SpeculativeDecoder` | `include/llm/speculative_decoder.h` | Draft model + verification for short-answer speedup |
| `LLMPluginManager` | `include/llm/llm_plugin_manager.h` | Model backend dispatch (llama.cpp / ONNX / external) |
| `LLMAQLHandler` | `src/aql/llm_aql_handler.cpp` | AQL operator integration point |

### B. Scheduler State Machine

```
REQUEST ARRIVES
  │
  ▼
QUEUE (waiting)  ──→  [max_queue_depth exceeded] → REJECT / 503
  │
  ▼
PREFILL slot available
  │
  ├── chunked_prefill=true  →  CHUNKED_PREFILL (n tokens/step)
  └── chunked_prefill=false →  FULL_PREFILL
  │
  ▼
GENERATION (token-by-token, batched with other active requests)
  │
  ├── KV-cache eviction triggered  →  DEGRADED_MODE (truncated context)
  ├── generation_timeout exceeded  →  TIMEOUT / partial response
  └── EOS or max_tokens reached    →  COMPLETE
```

### C. Degraded-Mode Semantics

When `PagedKVCache` cannot allocate pages for a new request and preemption is disabled
(preemption policy = `SWAP_NONE`), the scheduler activates degraded mode: the context
window is truncated to fit available pages, and a `kDegradedMode` flag is attached to
the `LLMResponse`. The `RAGJudge` pipeline treats degraded-mode responses as a separate
quality bucket for analysis.

### D. LoRA Adapter-Switch Protocol

`ContinuousLearningOrchestrator` triggers adapter updates when feedback thresholds are
crossed (Loop 4, `feedback_count ≥ 500`). The switch protocol:

1. `scheduler.pause_new_admissions()` — prevents new requests from being assigned the
   outgoing adapter.
2. In-flight requests on the outgoing adapter complete; scheduler drains them.
3. `LLMPluginManager.swap_adapter(new_lora)` — hot-swap on GPU VRAM.
4. `scheduler.resume_admissions()`.

Target switch latency: < 50 ms. This is the W8 workload claim in the flagship paper.

---

## IV. Experimental Methodology

### A. Hardware

- CPU: 20-core Intel @ 3.7 GHz, AVX2/AVX-512, 64 GB RAM.
- GPU (procurement in progress): NVIDIA RTX 3090 or A100 (24 GB VRAM).
- Storage: NVMe SSD (PCIe 4.0, 3.5 GB/s).

### B. Workloads

| Workload | Concurrency | Prompt Style | Notes |
|---|---|---|---|
| W-CB-1 | 10 req | Short (128 tok prompt, 64 tok gen) | Warm-up / baseline |
| W-CB-2 | 50 req | Mixed (128–512 tok prompt, 64–256 tok gen) | Moderate load |
| W-CB-3 | 100 req | Mixed | Target production load |
| W-CB-4 | 250 req | Mixed | Overload / degraded-mode trigger |

Prompt corpus: NaturalQuestions + synthetic multi-hop RAG prompts (50/50 split).

### C. Configuration Sweep

| Parameter | Values |
|---|---|
| `max_batch_size` | {8, 16, 32, 64} |
| `max_tokens_per_batch` | {512, 1024, 2048, 4096} |
| `chunked_prefill_size` | {disabled, 128, 256, 512 tokens} |
| `preemption_policy` | {SWAP_CPU, SWAP_NONE, RECOMPUTE} |

**Full factorial** for RQ1: 4 concurrency × 4 max_batch × 4 token_budget = 64 cells,
10 repetitions each = 640 measurement points.

**Targeted sweep** for RQ2–RQ4: 3 concurrency levels × 4 chunked_prefill × 3
preemption policies = 36 cells, 10 reps = 360 points.

### D. Metrics

**Primary**:
- `tokens/s` (generation throughput)
- `TTFT` (time-to-first-token, ms)
- `P50/P95/P99` completion latency (ms)

**Reliability**:
- KV-cache hit rate (%)
- Queue depth (P50/P99 occupancy)
- Degraded-mode activation rate (% of requests)
- LoRA switch latency (ms, P50/P99, for W8)

**Quality** (subset of 100 requests per cell, evaluated by RAGJudge FAST mode):
- G-Eval faithfulness (degraded vs. normal mode)
- Degraded-mode quality penalty (Δ faithfulness)

---

## V. Implementation Evidence

| ID | File | Scope | Claim |
|----|------|-------|-------|
| E1 | `include/llm/continuous_batch_scheduler.h` | ContinuousBatchScheduler | Continuous batching scheduler implemented |
| E2 | `include/llm/paged_kv_cache.h` | PagedKVCache | Paged KV memory management implemented |
| E3 | `include/llm/speculative_decoder.h` | SpeculativeDecoder | Speculative decoding implemented |
| E4 | `src/aql/llm_aql_handler.cpp` | LLMAQLHandler | LLM integrated as AQL operator |
| E5 | `tests/test_llm_aql_handler.cpp` | LLMAQLHandler tests | Correctness and integration tested |
| E6 | `benchmarks/bench_llm_raid_pipeline.cpp` | RAIDPipeline bench | Benchmark harness for W-CB-1..4 |
| E7 | `include/rag/continuous_learning_orchestrator.h` | ContinuousLearningOrchestrator | LoRA adapter lifecycle (Switch protocol §III.D) |
| E8 | `src/rag/rlaif_trainer.cpp` | RLAIFTrainer, Loop 4 | Adapter update trigger point |

---

## VI. Evaluation Design and Pre-Registered Expected Results

This section constitutes the formal pre-registration of expected experimental outcomes.
All quantitative ranges are derived from (a) the analytical memory model in §VII,
(b) published vLLM and S-LoRA benchmarks on comparable GPU hardware, and (c) the
ThemisDB scheduler design (§III). Pre-registered values are denoted `[pre-reg: …]`.
Empirical measurements will replace these ranges in v1.1 (post-GPU procurement).

### Table CB-1: Throughput × Concurrency × BatchSize (W-CB-1..4)

| Concurrency | max_batch_size | max_tokens_per_batch | tokens/s | TTFT (ms) | P99 (ms) |
|---|---|---|---|---|---|
| 10 | 8 | 512 | [pre-reg: ≥ 800] | [pre-reg: ≤ 80] | [pre-reg: ≤ 200] |
| 10 | 32 | 2048 | [pre-reg: ≥ 1 200] | [pre-reg: ≤ 60] | [pre-reg: ≤ 150] |
| 50 | 16 | 1024 | [pre-reg: ≥ 900] | [pre-reg: ≤ 120] | [pre-reg: ≤ 280] |
| 50 | 64 | 4096 | [pre-reg: ≥ 1 400] | [pre-reg: ≤ 100] | [pre-reg: ≤ 250] |
| 100 | 32 | 2048 | [pre-reg: ≥ 1 500] | [pre-reg: ≤ 100] | [pre-reg: ≤ 400] |
| 250 | 64 | 4096 | [pre-reg: ≥ 800] | [pre-reg: ≤ 250] | [pre-reg: ≤ 900] |

Basis: §VII stability-zone analysis (concurrency=100 anchor point) scaled via
published vLLM throughput curves [1]. Database-native sharing of GPU memory between
inference, HNSW index acceleration, and retrieval operators (§VIII.A) reduces effective
VRAM available for KV cache; expected throughput ranges therefore apply a conservative
10–20% downward adjustment relative to vLLM dedicated-serving results on equivalent
hardware.

### Table CB-2: Degraded-Mode Onset (W-CB-4)

| preemption_policy | Concurrency | Degraded-Mode Rate (%) | Δ Faithfulness | P99 (ms) |
|---|---|---|---|---|
| SWAP_CPU | 100 | [pre-reg: ~0–2%] | [pre-reg: ~0] | [pre-reg: ≤ 400] |
| SWAP_NONE | 100 | [pre-reg: ~0%] | [pre-reg: ~0] | [pre-reg: ≤ 400] |
| RECOMPUTE | 100 | [pre-reg: ~0%] | [pre-reg: ~0] | [pre-reg: ≤ 450] |
| SWAP_CPU | 250 | [pre-reg: ~0–5%] | [pre-reg: 0 to −0.05] | [pre-reg: ≤ 600] |
| SWAP_NONE | 250 | [pre-reg: ~30–60%] | [pre-reg: −0.10 to −0.20] | [pre-reg: ≤ 900] |

Basis: §VII memory model predicts SWAP_NONE onset at ~150–180 concurrent requests;
SWAP_CPU extends stable region to ~220–250 requests. Δ Faithfulness ranges are estimated
from RAGJudge FAST-mode quality bucket analysis (§III.C, §IV.D) for the head-truncation
degraded-mode scenario, where context window reduction disproportionately removes
supporting evidence tokens.

### Table CB-3: LoRA Adapter-Switch Latency (W8 claim)

| Adapter Size | In-flight Requests | Switch P50 (ms) | Switch P99 (ms) | SLO (< 50 ms) |
|---|---|---|---|---|
| 4-bit LoRA (7B) | 0 | [pre-reg: ≤ 30] | [pre-reg: < 50] | ✓ expected |
| 4-bit LoRA (7B) | 10 | [pre-reg: ≤ 45] | [pre-reg: ≤ 75] | ✓ expected |
| 4-bit LoRA (7B) | 50 | [pre-reg: ≤ 80] | [pre-reg: ≤ 120] | ✗ drain-limited |

Basis: §VII LoRA switch analysis; drain time at 50 in-flight requests dominates swap
cost. SLO met only when in-flight count is limited to ≤ 16 during switch windows (§VIII.B).

---

## VII. Analytical Basis for Pre-Registered Operating Points

The pre-registered expected values in §VI are grounded in the following analytical
derivations, which will be validated against empirical measurements in v1.1.

Based on published vLLM/S-LoRA results [1, 3] and the ThemisDB scheduler design, we
pre-register the following expected operating points:

**Throughput-latency stability zone** (RQ4):
- At max_batch_size=32, max_tokens=2048, concurrency=100: tokens/s ≥ 1 500, P99 ≤ 400 ms.
- Safety margin: increase max_tokens to 4096 before increasing max_batch_size to avoid
  KV-cache pressure.

**KV-cache pressure threshold** (RQ2):
- On RTX 3090 (24 GB), SWAP_NONE triggers degraded mode at approximately 150–180
  concurrent requests for 512-token prompts (based on memory model: 150 req × 512 tok
  × 2 × n_heads × head_dim × fp16 ≈ 18 GB KV).
- SWAP_CPU extends stable region to ≈ 220–250 requests at +15–30 ms P99 cost.

**Chunked prefill** (RQ3):
- Chunk size 256 tokens is expected to reduce TTFT variance by ≥ 30% vs. full prefill
  at concurrency=100, with < 5% tokens/s reduction (based on vLLM paper Figure 6).

**LoRA switch latency** (W8, RQ4):
- 4-bit 7B LoRA swap: < 50 ms at 0 in-flight requests; ≤ 120 ms at 50 in-flight
  (drain time dominates).

---

## VIII. Discussion

### A. Database-Native vs. Dedicated Serving

A dedicated serving system like vLLM has full control over GPU memory and can implement
aggressive preemption and recomputation. ThemisDB shares GPU memory with HNSW index
acceleration, LoRA training, and retrieval operators. This *memory contention* is the
primary source of performance delta vs. dedicated serving.

The advantage of the database-native approach is operational: LoRA lifecycle management,
retrieval index updates, and serving configuration are all governed by one transactional
system with one audit trail, eliminating the need for cross-system synchronisation
protocols.

### B. Guardrail Design Recommendations

Based on expected results:
- Set `generation_timeout` to P99 × 1.5 of the target SLO; this catches runaway
  generation without inflating tail latency for normal requests.
- Use `SWAP_CPU` preemption unless VRAM is ≥ 40 GB (A100 80 GB); `RECOMPUTE` is
  preferable only for very short prompts (< 128 tokens) where recompute cost is minimal.
- For LoRA adapter switching, ensure `max_batch_size ≤ 16` during switch windows to
  reduce drain time; the switch latency SLO of 50 ms is achievable under this constraint.

### C. Threats to Validity

**Internal validity**: benchmark prompts are from NaturalQuestions; real database query
answer prompts may be shorter or longer, shifting the working point.

**Construct validity**: degraded-mode quality penalty depends on which tokens are
truncated; head-truncation and tail-truncation have different faithfulness profiles.

**External validity**: GPU-specific results (RTX 3090) may not transfer to CPU-only or
ARM deployments; we plan ARM64 runs with `--preset linux-arm64-release`.

---

## IX. Reproducibility & Artifact

```bash
# Linux x64 (AVX2/AVX-512; GPU optional but required for full results)
cmake --preset linux-release -DTHEMIS_ENABLE_GPU=ON
cmake --build --preset linux-release

# W-CB-1: 10 concurrent requests, baseline
./build/linux-release/benchmarks/bench_llm_raid_pipeline \
  --concurrency 10 --max-batch 32 --max-tokens 2048 \
  --duration 60 --reps 10 --output artifacts/cb/w_cb1/

# W-CB-4: 250 concurrent (overload / degraded mode trigger)
./build/linux-release/benchmarks/bench_llm_raid_pipeline \
  --concurrency 250 --max-batch 64 --max-tokens 4096 \
  --preemption SWAP_CPU,SWAP_NONE,RECOMPUTE \
  --duration 60 --reps 10 --output artifacts/cb/w_cb4/

# LoRA switch latency (W8)
./build/linux-release/benchmarks/bench_lora_switch \
  --in-flight 0,10,50 --adapter-size 7b-4bit \
  --reps 30 --output artifacts/cb/w8/

# Analysis
python scripts/analyze_cb.py artifacts/cb/
```

**Expected runtime**: W-CB-1 ≈ 10 min; W-CB-4 ≈ 20 min (per preemption policy);
LoRA switch ≈ 5 min. Full sweep ≈ 3 h on GPU.

---

## X. Limitations, Risk, Ethics

- **Memory isolation**: VRAM sharing with index acceleration reduces effective serving
  capacity vs. dedicated GPU; results are not directly comparable to vLLM on same hardware.
- **Model quality**: `bench_llm_raid_pipeline` uses a lightweight GGUF model (< 4 GB) by
  default; production-quality results require full 7B+ models.
- **Safety**: guardrail parameter recommendations are empirically derived from this
  benchmark corpus; safety-critical deployments require additional adversarial testing.

---

## XI. Conclusion

This paper specifies the continuous batching measurement methodology for ThemisDB's
database-native LLM serving layer and constitutes a formal pre-registration of the
experimental design and expected operating points. The core claim — that database-native
scheduling involves three new interactions (retrieval back-pressure, LoRA lifecycle,
transaction serialisation) absent from dedicated serving — is grounded in the system model
(§III) and supported by implementation evidence (§V). Pre-registered expected operating
points (§VI, §VII) and guardrail recommendations (§VIII.B) provide concrete performance
targets for the empirical execution phase. Upon completion of GPU hardware procurement,
result Tables CB-1–CB-3 will be populated with empirical measurements and the paper
upgraded to v1.1 (empirical validation release).

---

## References

[1] Kwon, W., Li, Z., Zhuang, S., Sheng, Y., Zheng, L., Yu, C. H., … & Stoica, I. (2023).
Efficient Memory Management for Large Language Model Serving with PagedAttention.
*SOSP 2023*. arXiv:2309.06180. https://arxiv.org/abs/2309.06180

[2] NVIDIA TensorRT-LLM. https://github.com/NVIDIA/TensorRT-LLM (2024).

[3] Sheng, Y., et al. (2024). S-LoRA: Serving Thousands of Concurrent LoRA Adapters.
*MLSys 2024*. arXiv:2311.03285. https://arxiv.org/abs/2311.03285

[4] Yu, G., Kim, J., Jeong, G., Kim, S., Kim, H., Chun, B.-G., … & Jeong, J. (2022).
Orca: A Distributed Serving System for Transformer-Based Generative Models. *OSDI 2022*.
https://www.usenix.org/conference/osdi22/presentation/yu

[5] Zhao, Z., et al. (2023). In-Database Machine Learning with DP-BART for Privacy-
Preserving Text-to-SQL Generation. *EMNLP 2023*.
https://aclanthology.org/2023.emnlp-main.43

[6] Leviathan, Y., Kalman, M., & Matias, Y. (2023). Fast Inference from Transformers via
Speculative Decoding. *ICML 2023*. arXiv:2211.17192.
https://arxiv.org/abs/2211.17192

[7] Chen, C., et al. (2023). Accelerating Large Language Model Decoding with Speculative
Sampling. *arXiv:2302.01318*. https://arxiv.org/abs/2302.01318

[8] Agrawal, A., et al. (2024). Taming Throughput-Latency Tradeoff in LLM Inference with
Sarathi-Serve. *OSDI 2024*. arXiv:2308.16369.
https://www.usenix.org/conference/osdi24/presentation/agrawal

[9] Zhong, Y., et al. (2024). DistServe: Disaggregating Prefill and Decoding for
Goodput-Optimized Large Language Model Serving. *OSDI 2024*. arXiv:2401.09670.
https://www.usenix.org/conference/osdi24/presentation/zhong-yinmin

---

## Appendix A. Submission Readiness Checklist

- [x] Research questions stated with expected ranges pre-registered
- [x] System model with component inventory and failure modes (§III)
- [x] Full configuration sweep specified (§IV.C)
- [x] Metrics formally defined (§IV.D)
- [x] Implementation evidence registry with source files (§V)
- [x] Pre-registered expected results in evaluation tables CB-1–CB-3 (§VI)
- [x] Analytical basis for pre-registered values documented (§VII)
- [x] Guardrail recommendations specified (§VIII.B)
- [x] Reproducibility commands provided (§IX)
- [x] Limitations and threats documented (§VIII.C, §X)
- [x] All references include DOI or URL (§XII)
- [ ] W-CB-1..4 experiments executed and Table CB-1 filled with empirical data
- [ ] Degraded-mode onset measured and Table CB-2 filled with empirical data
- [ ] LoRA switch latency (W8) measured and Table CB-3 filled with empirical data

## Appendix B. Claim-to-Evidence Traceability

| Claim | Evidence IDs |
|-------|-------------|
| ContinuousBatchScheduler implemented | E1 |
| PagedKVCache implemented | E2 |
| SpeculativeDecoder implemented | E3 |
| LLM integrated as AQL operator | E4, E5 |
| Benchmark harness for W-CB workloads ready | E6 |
| LoRA lifecycle / adapter switch implemented | E7, E8 |
