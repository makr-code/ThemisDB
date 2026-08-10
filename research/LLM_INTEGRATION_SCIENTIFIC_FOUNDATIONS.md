# LLM Integration Scientific Foundations for ThemisDB

**Project:** ThemisDB  
**Category:** Research Documentation  
**Scope:** Scientific basis and repository-verified architecture for LLM integration  
**Status:** Review-ready draft  
**Version:** 2.0  
**Last Updated:** 2026-08-10

---

## Abstract

This document consolidates the scientific foundations of LLM integration in ThemisDB and aligns them with the current repository state. The core problem is to combine natural-language interaction with database reliability requirements (query safety, access control, and traceability). The approach in ThemisDB is a multi-layer LLM architecture with retrieval grounding, AQL-aware query handling, model/runtime abstraction, and explicit safety controls. Evaluation is based on repository evidence (focused tests, benchmark harnesses, and integration docs) plus peer-reviewed literature for model behavior and risk patterns. The current implementation is mature enough for review, but important limits remain: benchmark reproducibility depends on hardware setup, some LLM features are edition-dependent, and compliance/security controls still require deployment-specific validation.

---

## Introduction

### Problem Statement

Database-integrated LLM systems must satisfy two goals that often conflict:

1. **Language usability:** users want natural-language query and explanation workflows.
2. **Database guarantees:** systems must preserve correctness, security boundaries, and operational observability.

ThemisDB addresses this through explicit integration points instead of opaque "agent-only" orchestration:

- LLM API endpoints and OpenAI-compatible endpoints in `src/server/llm_api_handler.cpp`.
- Retrieval and context assembly modules in `src/rag/` and vector infrastructure in `src/vector/`.
- Prompt-management logic in `src/prompt_engineering/`.
- LLM runtime/model components in `src/llm/` and `src/llama_cpp/`.

### Terminology (normalized)

- **AQL:** ThemisDB query language for structured query execution.
- **Multi-model (LLM context):** multiple LLMs/runtimes selectable behind one API boundary (e.g., model routing and model listing endpoints).
- **Consistency model:** transactional/consistency guarantees remain the responsibility of the database core; LLM outputs are treated as candidate instructions that must pass validation before execution.
- **RAG:** retrieval-augmented generation; retrieved database context is provided to the model before answer/query generation.

---

## Methodology

### 1) Verification Method

This review used two evidence classes:

- **Repository evidence (primary for ThemisDB-specific claims):** source files, tests, benchmark harnesses, and internal documentation.
- **Scientific literature (primary for model/algorithm claims):** peer-reviewed papers and canonical technical references.

Claims were kept only if at least one of these was available:

- concrete code/module reference,
- benchmark/test artifact reference,
- peer-reviewed source with DOI/URL.

### 2) ThemisDB Architecture Mapping

| Layer | Repository evidence | Purpose |
|---|---|---|
| API and protocol | `src/server/llm_api_handler.cpp` | OpenAI-compatible and ThemisDB-native LLM endpoints |
| Prompt processing | `src/prompt_engineering/`, `config/prompts/` | Prompt templates, domain prompting, and optimization hooks |
| Retrieval (RAG) | `src/rag/`, `src/vector/` | Context retrieval and semantic search integration |
| Inference/runtime | `src/llm/`, `src/llama_cpp/` | Model loading, inference orchestration, runtime controls |
| Validation and safety | `src/server/llm_api_handler.cpp`, `tests/llm/test_llm_validation.cpp`, `tests/llm/test_llm_safety_pipeline.cpp` | Query/output validation and safety checks |
| Observability and quality checks | `tests/llm/`, `benchmarks/` | Focused regression checks and performance instrumentation |

### 3) Scientific Foundations Used

- Transformer architecture and scaling behavior [1]-[3].
- Embeddings and sentence representation quality [4]-[5].
- Prompting and prompt optimization paradigms [6]-[11].
- RAG and RAG evaluation patterns [12]-[14].
- Parameter-efficient adaptation (LoRA/QLoRA) [15]-[16].
- Inference system optimizations (llama.cpp, FlashAttention, speculative decoding, paged KV techniques) [17]-[20].
- LLM security risk models and policy references [21]-[24].

---

## Evaluation / Experiments

### Evaluation Scope

This document does **not** claim a single universal performance number for "ThemisDB LLM performance". Instead, it verifies that the repository contains measurable evaluation paths and that those paths map to recognized scientific metrics.

### 1) API and Integration Evidence

- OpenAI-compatible chat endpoint route: `/v1/chat/completions` in `src/server/llm_api_handler.cpp`.
- Native LLM routes for inference, RAG, embeddings, model management, feedback, and AQL explain streaming are listed and routed in the same handler.
- LLM module changelog documents staged feature delivery and hardening (`src/llm/CHANGELOG.md`).

### 2) Test Evidence

- `tests/llm/CMakeLists.txt` auto-discovers `test_*.cpp` and registers focused test executables, ensuring broad LLM test coverage is continuously compilable.
- Representative focused tests:
  - API contract and auth hardening: `tests/llm/test_llm_api_contract_hardening_focused.cpp`
  - OpenAI compatibility: `tests/llm/test_openai_compat_adapter.cpp`
  - AQL bridge/streaming: `tests/llm/test_llm_aql_embedding_bridge.cpp`, `tests/llm/test_llm_aql_explain_stream_api.cpp`
  - Safety/validation: `tests/llm/test_llm_safety_pipeline.cpp`, `tests/llm/test_llm_validation.cpp`

### 3) Benchmark Evidence

- LLM-oriented benchmark artifacts exist in `benchmarks/` (e.g., `bench_embedded_llm.cpp`, `llm_bench.cpp`, `bench_flash_attention.cpp`, `bench_continuous_query.cpp`).
- Benchmark process guidance is documented in `docs/en/llm/LLM_BENCHMARKING_GUIDE.md`.
- The benchmark set supports latency/throughput/memory-style evaluation, while exact results are hardware- and model-dependent and must be reported per run configuration.

### 4) Mapping to Scientific Metrics

| Goal | Practical metric family | Scientific anchors |
|---|---|---|
| Response quality and grounding | faithfulness/relevance/context metrics | RAG/RAGAS [12], [14] |
| Generation behavior | task success, correctness, hallucination-sensitive checks | prompting and evaluation literature [6]-[11], [13] |
| Runtime efficiency | tokens/s, latency percentiles, memory footprint | systems optimization work [17]-[20] |
| Safety | prompt injection resilience, constrained output compliance | security literature [21]-[23] |

---

## Limitations / Known Issues

1. **No single canonical benchmark number:** The repository provides benchmark harnesses, but claims such as "X tokens/s" are only meaningful with explicit hardware, model, quantization, and dataset context.
2. **Edition/deployment variability:** Some LLM features, plugins, and operational pathways differ by edition and deployment profile.
3. **Security controls require runtime validation:** Source-level safeguards are present, but production posture still depends on key management, network policy, and operational monitoring.
4. **Compliance is use-case dependent:** GDPR/EU AI Act obligations depend on workload class, jurisdiction, and whether the deployment is decision-support vs. high-risk automation.
5. **Literature drift:** LLM research moves quickly; benchmark rankings and best practices require regular re-validation against newer models and methods.

---

## Conclusion

ThemisDB’s LLM integration is technically grounded in established literature and implemented through explicit, auditable repository components. The architecture follows a clear chain:

**Problem (safe NL-to-database interaction)** → **Approach (layered LLM + RAG + validation architecture)** → **Evaluation (tests + benchmark harness + documented procedures)** → **Limits (deployment-specific performance/compliance/security constraints)**.

This provides a review-ready baseline for scientific and engineering assessment while avoiding unsupported universal performance claims.

---

## References

### A) Scientific references

[1] A. Vaswani et al., "Attention Is All You Need," *NeurIPS*, 2017. URL: https://arxiv.org/abs/1706.03762  
[2] J. Kaplan et al., "Scaling Laws for Neural Language Models," 2020. URL: https://arxiv.org/abs/2001.08361  
[3] J. Hoffmann et al., "Training Compute-Optimal Large Language Models," *NeurIPS*, 2022. URL: https://arxiv.org/abs/2203.15556  
[4] J. Devlin et al., "BERT: Pre-training of Deep Bidirectional Transformers for Language Understanding," *NAACL-HLT*, 2019. DOI: 10.18653/v1/N19-1423  
[5] N. Reimers and I. Gurevych, "Sentence-BERT," *EMNLP*, 2019. DOI: 10.18653/v1/D19-1410  
[6] T. Brown et al., "Language Models are Few-Shot Learners," *NeurIPS*, 2020. URL: https://arxiv.org/abs/2005.14165  
[7] J. Wei et al., "Chain-of-Thought Prompting Elicits Reasoning in Large Language Models," *NeurIPS*, 2022. URL: https://arxiv.org/abs/2201.11903  
[8] R. Pryzant et al., "Automatic Prompt Optimization with 'Gradient Descent' and Beam Search," *EMNLP*, 2023. DOI: 10.18653/v1/2023.emnlp-main.494  
[9] Y. Zhou et al., "Large Language Models are Human-Level Prompt Engineers," *ICLR*, 2023. URL: https://arxiv.org/abs/2211.01910  
[10] O. Rubin et al., "Learning To Retrieve Prompts for In-Context Learning," *NAACL-HLT*, 2022. DOI: 10.18653/v1/2022.naacl-main.191  
[11] A. Madaan et al., "Self-Refine: Iterative Refinement with Self-Feedback," *NeurIPS*, 2023. URL: https://arxiv.org/abs/2303.17651  
[12] P. Lewis et al., "Retrieval-Augmented Generation for Knowledge-Intensive NLP Tasks," *NeurIPS*, 2020. URL: https://arxiv.org/abs/2005.11401  
[13] V. Karpukhin et al., "Dense Passage Retrieval for Open-Domain Question Answering," *EMNLP*, 2020. DOI: 10.18653/v1/2020.emnlp-main.550  
[14] S. Es et al., "RAGAS: Automated Evaluation of Retrieval Augmented Generation," *EACL*, 2024. URL: https://arxiv.org/abs/2309.15217  
[15] E. Hu et al., "LoRA: Low-Rank Adaptation of Large Language Models," *ICLR*, 2022. URL: https://arxiv.org/abs/2106.09685  
[16] T. Dettmers et al., "QLoRA: Efficient Finetuning of Quantized LLMs," *NeurIPS*, 2023. URL: https://arxiv.org/abs/2305.14314  
[17] G. Gerganov, "llama.cpp," GitHub repository, 2023. URL: https://github.com/ggml-org/llama.cpp  
[18] T. Dao et al., "FlashAttention," *NeurIPS*, 2022. URL: https://arxiv.org/abs/2205.14135  
[19] C. Chen et al., "Accelerating Large Language Model Decoding with Speculative Sampling," 2023. URL: https://arxiv.org/abs/2302.01318  
[20] W. Kwon et al., "Efficient Memory Management for Large Language Model Serving with PagedAttention," *SOSP*, 2023. DOI: 10.1145/3600006.3613165  
[21] F. Perez and I. Ribeiro, "Ignore Previous Prompt," 2022. URL: https://arxiv.org/abs/2211.09527  
[22] K. Greshake et al., "Not What You've Signed Up For," *AISec/CCS Workshop*, 2023. URL: https://arxiv.org/abs/2302.12173  
[23] OWASP Foundation, "OWASP Top 10 for LLM Applications," 2023. URL: https://owasp.org/www-project-top-10-for-large-language-model-applications/  
[24] European Parliament and Council, "Regulation (EU) 2024/1689 (AI Act)," 2024. URL: https://eur-lex.europa.eu/legal-content/EN/TXT/?uri=CELEX:32024R1689

### B) ThemisDB repository references

[25] `src/server/llm_api_handler.cpp` (OpenAI-compatible and native LLM route handling).  
[26] `tests/llm/CMakeLists.txt` (focused test target generation).  
[27] `tests/llm/test_openai_compat_adapter.cpp`.  
[28] `tests/llm/test_llm_validation.cpp`.  
[29] `tests/llm/test_llm_safety_pipeline.cpp`.  
[30] `docs/en/llm/LLM_BENCHMARKING_GUIDE.md`.  
[31] `src/llm/CHANGELOG.md`.  
[32] `src/rag/`, `src/vector/`, `src/prompt_engineering/`, `src/llm/`, `src/llama_cpp/`.

---

## Appendix: Related ThemisDB Docs

- `docs/en/llm/README.md`
- `docs/en/llm/LLM_LORA_LLAMACPP_INTEGRATION.md`
- `docs/en/llm/FLASH_ATTENTION_IMPLEMENTATION.md`
- `docs/en/llm/SPECULATIVE_DECODING_IMPLEMENTATION.md`
- `docs/en/llm/GRAMMAR_CONSTRAINED_GENERATION.md`
- `docs/en/llm/LORA_TRAINING_GUIDE.md`
- `research/PROMPT_ENHANCEMENT_ENGINE_OPTIMIZATION_RESEARCH.md`
- `research/PROMPT_OPTIMIZATION_IMPLEMENTATION_STRATEGY.md`

