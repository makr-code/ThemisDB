# Core Scientific Foundations for LLM Integration in ThemisDB

**Project:** ThemisDB  
**Category:** Research Documentation  
**Research Topic:** Scientific Foundations, Peer-Reviewed References, and Implementation Best Practices for Large Language Model Integration  
**Status:** ✅ Research Complete  
**Date:** March 2026  
**Version:** 1.0

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Transformer Architecture & Foundation Models](#2-transformer-architecture--foundation-models)
3. [Embeddings & Semantic Representations](#3-embeddings--semantic-representations)
4. [Prompt Engineering](#4-prompt-engineering)
5. [Autonomous Prompt Optimization](#5-autonomous-prompt-optimization)
6. [Retrieval-Augmented Generation (RAG)](#6-retrieval-augmented-generation-rag)
7. [Parameter-Efficient Fine-Tuning & LoRA Pipelines](#7-parameter-efficient-fine-tuning--lora-pipelines)
8. [Edge AI & Embedded Inference (llama.cpp)](#8-edge-ai--embedded-inference-llamacpp)
9. [OpenAI & HuggingFace Ecosystem Integration](#9-openai--huggingface-ecosystem-integration)
10. [Model Evaluation & Benchmarking](#10-model-evaluation--benchmarking)
11. [Security Considerations for LLM-Powered Systems](#11-security-considerations-for-llm-powered-systems)
12. [Regulatory & Compliance Aspects](#12-regulatory--compliance-aspects)
13. [ThemisDB Integration Architecture](#13-themisdb-integration-architecture)
14. [References (IEEE Format)](#14-references-ieee-format)

---

## 1. Executive Summary

This document establishes the peer-reviewed scientific foundations underpinning the LLM integration strategy in ThemisDB. It covers the core theoretical and engineering pillars required to build a production-grade, embedded LLM subsystem. Topics include: transformer-based foundation models, embedding spaces, prompt engineering strategies, autonomous optimization loops, retrieval-augmented generation, and parameter-efficient fine-tuning (LoRA/QLoRA). Additional topics include edge inference via llama.cpp, model evaluation methodologies, and the security and regulatory frameworks applicable to LLM-powered database systems.

All peer-reviewed references are formatted in **IEEE citation style**. Implementation guidance draws directly from these foundations and maps each concept to existing ThemisDB modules.

### Key Findings

- ✅ **Transformer attention** (Vaswani et al., 2017 [1]) is the universal foundation; all major LLMs extend this architecture.
- ✅ **Dense embeddings** (Devlin et al., 2019 [4]; Reimers & Gurevych, 2019 [5]) enable semantic vector search, a first-class ThemisDB primitive.
- ✅ **Chain-of-Thought prompting** (Wei et al., 2022 [7]) and **few-shot prompting** (Brown et al., 2020 [6]) set the baseline for ThemisDB's prompt templates.
- ✅ **APE / automatic prompt optimization** (Zhou et al., 2022 [9]) provides the research foundation for ThemisDB's self-improving PromptEnhancementEngine.
- ✅ **LoRA** (Hu et al., 2022 [16]) is the recommended fine-tuning approach for domain adaptation without full model re-training.
- ✅ **llama.cpp** (Gerganov, 2023 [21]) enables GGUF-quantized on-device inference, eliminating external API dependencies.
- ✅ **Jailbreak / prompt injection** vulnerabilities (Perez & Ribeiro, 2022 [28]; Greshake et al., 2023 [29]) require explicit mitigations at the ThemisDB API boundary.
- ✅ **EU AI Act** (European Parliament, 2024 [33]) classifies certain database-integrated AI uses as high-risk, requiring conformity assessments.

---

## 2. Transformer Architecture & Foundation Models

### 2.1 Attention Is All You Need

The seminal work by Vaswani et al. [1] introduced the **Transformer**, replacing recurrent architectures with multi-head self-attention. Every modern LLM — GPT, LLaMA, Mistral, Phi — is a Transformer variant. Key contributions:

- **Scaled dot-product attention:** `Attention(Q, K, V) = softmax(QKᵀ / √dₖ) V`
- **Multi-head attention:** Parallel attention projections enrich representational capacity.
- **Positional encoding:** Injects sequential order into position-agnostic attention.

**ThemisDB relevance:** The llama.cpp backend (`src/llm/`) operates on Transformer-based models; understanding attention complexity (O(n²) in sequence length) informs context-window and batching design decisions.

### 2.2 Scaling Laws

Kaplan et al. [2] established empirical **scaling laws** for language models: loss scales as a power law with compute, data, and parameter count. Hoffmann et al. [3] refined these laws (Chinchilla), showing that optimally trained models use ~20 tokens per parameter. These laws:

- Justify choosing 7B–13B parameter models for embedded deployment (favorable quality-vs-compute trade-off).
- Inform quantization budgets: reducing bits-per-weight degrades perplexity predictably.

### 2.3 Open Foundation Models Relevant to ThemisDB

| Model Family | Organization | Parameters | License | GGUF Support |
|---|---|---|---|---|
| LLaMA 3 | Meta AI | 8B, 70B | Meta Custom | ✅ |
| Mistral / Mixtral | Mistral AI | 7B, 8×7B | Apache 2.0 | ✅ |
| Phi-3 | Microsoft | 3.8B, 7B | MIT | ✅ |
| Gemma 2 | Google DeepMind | 2B, 9B | Gemma ToS | ✅ |
| Qwen 2.5 | Alibaba | 0.5B–72B | Apache 2.0 | ✅ |

---

## 3. Embeddings & Semantic Representations

### 3.1 Contextual Word Representations (BERT)

Devlin et al. [4] introduced **BERT**, demonstrating that bidirectional pre-training on masked language modeling produces rich contextual embeddings. BERT embeddings became the standard input to downstream NLP tasks and formed the basis for sentence-level embedding models.

### 3.2 Sentence Embeddings (SBERT)

Reimers & Gurevych [5] adapted BERT with **Siamese network fine-tuning** on NLI and STS tasks, producing **Sentence-BERT (SBERT)** — fixed-size (384–1024 dimensional) sentence-level embeddings suitable for semantic search. SBERT models (`all-MiniLM-L6-v2`, `all-mpnet-base-v2`) are the primary embedding models integrated with ThemisDB's vector index (`src/vector/`).

### 3.3 Embedding Dimensionality & Vector Index Compatibility

| Model | Dimensions | Use Case | ThemisDB Index |
|---|---|---|---|
| all-MiniLM-L6-v2 | 384 | Fast semantic search | HNSW, IVF |
| all-mpnet-base-v2 | 768 | High-quality semantic search | HNSW, IVF |
| text-embedding-3-small (OpenAI) | 1536 | API-based general purpose | HNSW |
| text-embedding-3-large (OpenAI) | 3072 | API-based high accuracy | IVF |
| llama.cpp (custom) | 4096 | On-device model embeddings | HNSW |

### 3.4 Matryoshka Representation Learning

Kusupati et al. [18] introduced **Matryoshka Representation Learning (MRL)**, embedding vectors where lower-dimensional prefixes are meaningful. This allows a single embedding model to serve multiple index granularities (e.g., 64D for ANN pre-filtering, 768D for re-ranking), which is directly applicable to ThemisDB's multi-stage retrieval pipeline.

---

## 4. Prompt Engineering

### 4.1 Few-Shot In-Context Learning

Brown et al. [6] demonstrated that GPT-3 achieves competitive task performance with only a few input–output **examples in the context window** (few-shot prompting) — no gradient updates required. This establishes the basis for ThemisDB's prompt template system (`config/prompts/`):

- **Zero-shot:** Instruction only.
- **One-shot:** One example.
- **Few-shot:** 2–8 examples; diminishing returns beyond that.

### 4.2 Chain-of-Thought (CoT) Prompting

Wei et al. [7] showed that appending intermediate reasoning steps ("Let's think step by step") to prompts significantly improves performance on multi-step reasoning tasks. CoT prompting is implemented in ThemisDB's scientific and legal prompt templates and is foundational to the LLM-based query explanation feature.

### 4.3 Prompt Pattern Catalog

White et al. [8] systematically catalogued **reusable prompt patterns** (Persona, Cognitive Verifier, Flipped Interaction, Template, etc.) analogous to software design patterns. ThemisDB's prompt template YAML files (`config/prompts/*.yaml`) apply several of these patterns across domains (legal, scientific, geographic, mathematical).

### 4.4 Retrieval-Augmented Prompting

Rubin et al. [11] showed that **selecting in-context examples by semantic similarity** (rather than randomly) significantly improves few-shot performance. ThemisDB's RAG pipeline (`src/rag/`) implements this principle: retrieved context chunks are prepended to prompts, grounding LLM responses in the database's authoritative content.

---

## 5. Autonomous Prompt Optimization

### 5.1 Automatic Prompt Engineer (APE)

Zhou et al. [9] introduced **Automatic Prompt Engineer (APE)**, framing prompt generation as a program synthesis problem solved by the LLM itself. A meta-prompt is used to generate candidate instruction variants, which are then scored by execution accuracy. APE demonstrated human-level or better prompt quality on many benchmarks.

**ThemisDB integration:** APE-style meta-optimization is the theoretical basis for `src/prompt_engineering/PromptEnhancementEngine`, which generates prompt candidates and evaluates them against quality metrics.

### 5.2 Gradient-Based Prompt Optimization (ProTeGi)

Pryzant et al. [10] proposed treating **natural language gradient descent** as iterative critique-and-revision: an LLM critiques the current prompt based on errors, then suggests an improved version. This mirrors gradient descent without differentiating through the model. ThemisDB's A/B testing and rollback infrastructure is designed to support this optimization loop safely.

### 5.3 Evolutionary Prompt Optimization (EvoPrompt)

Guo et al. [12] applied **evolutionary algorithms** (genetic algorithms and differential evolution) to prompt optimization, producing EvoPrompt. Populations of prompt candidates undergo crossover and mutation (performed by the LLM), with fitness evaluated on a development set. This approach is particularly effective when a labeled dataset is available.

### 5.4 Self-Refine and Reflexion

Madaan et al. [13] showed that LLMs can iteratively improve their own outputs through **self-critique and refinement** loops without external labels. Shinn et al. [14] extended this with **Reflexion**, using verbal reinforcement learning where the LLM stores experience as natural language in its context. Both mechanisms inform ThemisDB's feedback-driven prompt improvement pipeline.

### 5.5 Promptbreeder (Self-Referential Optimization)

Fernando et al. [15] introduced **Promptbreeder**, a self-referential system where both the task-prompt and the mutation-prompt co-evolve. This is the most autonomous approach and represents the long-term research direction for ThemisDB's self-improving prompt subsystem.

---

## 6. Retrieval-Augmented Generation (RAG)

### 6.1 Original RAG Framework

Lewis et al. [24] introduced **Retrieval-Augmented Generation (RAG)**, combining dense passage retrieval (DPR) with a generative model (BART). Retrieved documents are prepended to the context, enabling the model to answer factual questions grounded in a dynamic knowledge corpus. RAG architecture is central to ThemisDB because:

- The database itself is the knowledge corpus.
- Vector search replaces traditional DPR.
- LLM inference (llama.cpp or API) serves as the generator.

### 6.2 Dense Passage Retrieval (DPR)

Karpukhin et al. [25] established the dense retrieval paradigm: a bi-encoder (BERT-based) produces embeddings for questions and passages independently; retrieval is maximum inner product search (MIPS) over the passage index. DPR outperforms BM25 on open-domain QA, validating the shift from keyword to semantic retrieval.

### 6.3 RAG Evaluation (RAGAS)

Es et al. [26] proposed **RAGAS**, an automated reference-free framework for evaluating RAG pipelines along four dimensions:

| Metric | Measures |
|---|---|
| Faithfulness | Are answers supported by retrieved context? |
| Answer Relevancy | Is the answer relevant to the question? |
| Context Precision | Is the retrieved context precise? |
| Context Recall | Does retrieved context contain the answer? |

RAGAS metrics are applicable to ThemisDB's LLM-assisted query responses and are referenced in the RAG evaluation module (`docs/en/llm/RAG_INDEX.md`).

---

## 7. Parameter-Efficient Fine-Tuning & LoRA Pipelines

### 7.1 Low-Rank Adaptation (LoRA)

Hu et al. [16] introduced **LoRA**, freezing pre-trained weights and injecting trainable low-rank decomposition matrices into each Transformer layer. For a weight matrix W ∈ ℝ^(d×k), LoRA adds:

```
W' = W + BA,  where B ∈ ℝ^(d×r), A ∈ ℝ^(r×k), r ≪ min(d, k)
```

**Benefits for ThemisDB:**
- Fine-tune 7B models with <1 GB of trainable parameters (r=8).
- Domain adapters for legal, scientific, or geographic query rewriting.
- Multiple LoRA adapters can be hot-swapped at inference time (Multi-LoRA, implemented in `src/llm/lora/`).

### 7.2 QLoRA

Dettmers et al. [17] combined LoRA with **4-bit NF4 quantization** and double quantization, enabling fine-tuning of 65B models on a single 48 GB GPU. The key insight is that quantization errors in frozen weights can be compensated by the LoRA adapter. This enables:

- Fine-tuning large models on consumer hardware.
- GGUF export of QLoRA fine-tuned models for llama.cpp deployment.

### 7.3 Prompt Tuning & Prefix Tuning

Lester et al. [19] demonstrated that prepending a small number of **trainable soft prompt tokens** to the input achieves near-full fine-tuning quality at scale. Li & Liang [20] extended this with **Prefix Tuning**, inserting trainable prefix tokens into all Transformer layers. These methods are lighter than LoRA but less transferable across inference runtimes.

### 7.4 LoRA Training Pipeline (ThemisDB)

```
Dataset Collection (ThemisDB queries + labels)
        ↓
Base Model (GGUF → FP16 conversion for training)
        ↓
QLoRA Fine-Tuning (HuggingFace PEFT + bitsandbytes)
        ↓
Merge & Export (merge LoRA → base → convert to GGUF)
        ↓
Evaluation (RAGAS / benchmark suite)
        ↓
Deploy to ThemisDB Multi-LoRA Manager
```

See: `docs/en/llm/LORA_TRAINING_GUIDE.md` for the complete pipeline implementation.

---

## 8. Edge AI & Embedded Inference (llama.cpp)

### 8.1 llama.cpp Architecture

Gerganov [21] created **llama.cpp**, a pure C/C++ implementation of LLaMA-family inference with no Python runtime dependency. Key design principles:

- **GGUF format:** A container format for quantized model weights, supporting Q4_0, Q4_K_M, Q5_K_M, Q8_0, and FP16 precisions.
- **CPU-first with optional acceleration:** AVX2/AVX-512 intrinsics on x86; Metal on Apple Silicon; CUDA / Vulkan / OpenCL for GPU offload.
- **Zero-copy memory mapping:** `mmap()` for multi-process model sharing.
- **KV cache:** Per-session key-value cache enabling multi-turn conversations.

**ThemisDB uses llama.cpp as the exclusive on-device inference backend** (build flag: `-DTHEMIS_ENABLE_LLM=ON`). The integration is documented in `docs/en/llm/LLAMA_CPP_MIGRATION.md`.

### 8.2 GGUF Quantization Trade-offs

| Quantization | Bits/Weight | 7B Memory | Perplexity δ | Recommended Use |
|---|---|---|---|---|
| Q4_K_M | ~4.5 | ~4.1 GB | +0.15 | General embedded use |
| Q5_K_M | ~5.5 | ~5.0 GB | +0.05 | Quality-sensitive tasks |
| Q8_0 | 8 | ~7.2 GB | ~0.00 | Near-lossless, high-RAM |
| FP16 | 16 | ~14 GB | 0.00 (baseline) | GPU-only, full precision |

### 8.3 Flash Attention

Dao et al. [22] introduced **FlashAttention**, an IO-aware exact attention algorithm that tiles computation to minimize HBM reads/writes. FlashAttention achieves 2–4× speedup and 5–20× memory reduction over standard attention for long sequences. ThemisDB's CUDA LLM build includes FlashAttention kernels (`docs/en/llm/FLASH_ATTENTION_IMPLEMENTATION.md`).

### 8.4 Speculative Decoding

Chen et al. [23] introduced **speculative decoding**: a small draft model generates k tokens; the larger target model verifies all k tokens in a single forward pass, accepting correct tokens and rejecting the first mismatch. This achieves 2–3× inference speedup at identical output quality. Implemented in ThemisDB: `docs/en/llm/SPECULATIVE_DECODING_IMPLEMENTATION.md`.

### 8.5 PagedAttention & Continuous Batching

Kwon et al. [27] introduced **PagedAttention** (vLLM), treating the KV cache like virtual memory pages to eliminate fragmentation. Combined with **continuous batching** (Orca scheduling), this improves GPU utilization by 23× compared to static batching. ThemisDB implements continuous batching in `docs/en/llm/CONTINUOUS_BATCHING_IMPLEMENTATION.md`.

---

## 9. OpenAI & HuggingFace Ecosystem Integration

### 9.1 OpenAI API

The OpenAI platform provides the reference API design for modern LLM integration:

- **Chat Completions API** (`/v1/chat/completions`): Structured message format (system/user/assistant) used as the canonical interface in ThemisDB's LLM abstraction layer.
- **Embeddings API** (`/v1/embeddings`): Source for `text-embedding-3-*` vectors when cloud inference is acceptable.
- **Function Calling / Tools:** Structured output via JSON schema — the theoretical basis for ThemisDB's grammar-constrained generation (`docs/en/llm/GRAMMAR_CONSTRAINED_GENERATION.md`).
- **Responses API** (2025): Stateful multi-turn interface with server-side conversation management.

ThemisDB's HTTP server (`docs/en/llm/HTTP_SERVER_INTEGRATION.md`) exposes an OpenAI-compatible endpoint, enabling drop-in replacement with any OpenAI-compatible client.

### 9.2 HuggingFace Transformers & PEFT

HuggingFace Transformers [30] provides the standard Python API for model loading, fine-tuning, and inference. For ThemisDB's LoRA training pipeline:

- **`transformers` library:** Base model loading, tokenizer, `AutoModelForCausalLM`.
- **`peft` library:** LoRA/QLoRA implementation via `LoraConfig` and `get_peft_model`.
- **`bitsandbytes`:** 4-bit and 8-bit quantization for QLoRA fine-tuning.
- **`trl`:** Supervised fine-tuning with `SFTTrainer`; RLHF with `PPOTrainer`.
- **`datasets`:** Standard dataset loading and preprocessing.

Model Hub (huggingface.co) is the primary source for GGUF-converted community models compatible with llama.cpp.

### 9.3 Model Selection Criteria

When selecting a foundation model for ThemisDB integration, the following criteria apply:

| Criterion | Rationale |
|---|---|
| License (Apache 2.0 / MIT preferred) | Required for commercial deployment |
| GGUF availability on HuggingFace | Required for llama.cpp integration |
| Context window ≥ 8K tokens | Supports RAG payloads of ≥10 retrieved chunks |
| Instruction-tuned variant available | Essential for prompt-following behavior |
| Benchmark scores (MMLU, HumanEval) | Proxy for general capability |

---

## 10. Model Evaluation & Benchmarking

### 10.1 Standard Benchmarks

| Benchmark | Measures | Relevance to ThemisDB |
|---|---|---|
| MMLU [31] | Multi-task language understanding (57 subjects) | General capability baseline |
| HumanEval | Code generation correctness | SQL / AQL query generation |
| HellaSwag | Commonsense reasoning | Natural language query understanding |
| TruthfulQA | Factual accuracy, hallucination rate | Critical for database Q&A |
| MT-Bench [32] | Multi-turn conversational quality | Conversational database queries |
| RAGAS [26] | RAG pipeline faithfulness & relevancy | Direct ThemisDB RAG evaluation |

### 10.2 LLM-as-Judge

Zheng et al. [32] introduced **MT-Bench** and the **LLM-as-Judge** paradigm: using a powerful LLM (GPT-4) as an automated evaluator for open-ended response quality. Liu et al. proposed **G-Eval** (LLM-based NLG evaluation with chain-of-thought scoring). These evaluation approaches are implemented in ThemisDB's feedback collection pipeline, enabling automated quality assessment without human annotation for every query.

### 10.3 Hallucination Detection

Hallucination is a first-class concern for database-integrated LLMs. Mitigation strategies supported in ThemisDB:

1. **RAG grounding:** Every LLM response is grounded in retrieved database content.
2. **RAGAS Faithfulness metric:** Automated post-hoc verification that claims are supported by context.
3. **Grammar-constrained generation:** GBNF grammars force structured output, eliminating formatting hallucinations.
4. **Source citation:** Responses include entity IDs and document references from the database.

### 10.4 ThemisDB LLM Benchmarking Suite

ThemisDB maintains an LLM benchmarking guide (`docs/en/llm/LLM_BENCHMARKING_GUIDE.md`) covering:

- Inference throughput (tokens/second) across quantization levels.
- Latency percentiles (P50, P95, P99) for single and batch requests.
- Memory footprint per model variant.
- Quality metrics (MMLU subset, RAGAS) for domain-adapted LoRA models.

---

## 11. Security Considerations for LLM-Powered Systems

### 11.1 Prompt Injection

Perez & Ribeiro [28] demonstrated **prompt injection attacks**: malicious content in user inputs overrides the system prompt, hijacking the LLM's behavior. In a database context, an attacker could inject instructions that cause the LLM to exfiltrate data or bypass access controls embedded in the system prompt.

**ThemisDB mitigations:**
- Input sanitization before prompt assembly (`src/llm/` security layer).
- System prompt isolation: user input is never concatenated directly into the instruction field.
- Output validation: LLM-generated queries are validated against an AQL/SQL schema before execution.
- Principle of least privilege: the LLM backend operates with read-only database access by default.

### 11.2 Indirect Prompt Injection

Greshake et al. [29] extended prompt injection to **indirect prompt injection**, where malicious instructions are embedded in retrieved content (web pages, documents) that the LLM processes as context. In ThemisDB's RAG pipeline, retrieved database content could contain adversarial instructions.

**Mitigations:**
- Content trust levels: internally authored content is trusted; externally ingested content is sandboxed.
- Structured context format: retrieved chunks are enclosed in explicit XML-like delimiters that the system prompt instructs the model to treat as data, not instructions.
- Output safety filters applied regardless of generation path.

### 11.3 Model Extraction & Inversion Attacks

Fine-tuned LoRA adapters trained on proprietary data represent intellectual property. Model extraction attacks attempt to reconstruct training data or model weights through repeated querying.

**Mitigations:**
- Rate limiting on the ThemisDB LLM endpoint.
- Differential privacy during LoRA fine-tuning (DP-SGD) for sensitive training corpora.
- Adapter encryption at rest using existing ThemisDB encryption infrastructure.

### 11.4 Data Leakage Through LLM Memorization

LLMs can memorize and reproduce training data verbatim (Carlini et al., 2021). For ThemisDB deployments where LLMs are fine-tuned on database contents, this creates a risk of data leakage through model outputs.

**Mitigations:**
- Deduplicate training data to reduce memorization.
- Apply membership inference testing before model release.
- Access-control-aware RAG: only retrieve chunks the querying user is authorized to read.

### 11.5 OWASP Top 10 for LLM Applications

The OWASP Foundation released the **Top 10 for Large Language Model Applications** (2023), identifying the most critical risks:

| Rank | Risk | ThemisDB Mitigation |
|---|---|---|
| LLM01 | Prompt Injection | Input sanitization + output validation |
| LLM02 | Insecure Output Handling | Schema-constrained generation + GBNF |
| LLM03 | Training Data Poisoning | Dataset provenance tracking |
| LLM04 | Model Denial of Service | Request rate limiting + token budget |
| LLM05 | Supply Chain Vulnerabilities | Model hash verification + signed GGUF |
| LLM06 | Sensitive Information Disclosure | Access-control-aware retrieval |
| LLM07 | Insecure Plugin Design | Plugin sandboxing in ThemisDB plugin system |
| LLM08 | Excessive Agency | Minimal-permission LLM tools |
| LLM09 | Overreliance | Confidence scoring + human-in-the-loop alerts |
| LLM10 | Model Theft | Adapter encryption + rate limiting |

---

## 12. Regulatory & Compliance Aspects

### 12.1 EU AI Act (2024)

The **European Union Artificial Intelligence Act** [33] establishes a risk-based framework for AI systems deployed in the EU. Database-integrated AI systems may fall under several categories:

**Risk Classification for ThemisDB Use Cases:**

| Use Case | AI Act Risk Level | Requirements |
|---|---|---|
| Legal document analysis (judicial decisions) | **High Risk** (Annex III §8) | Conformity assessment, logging, human oversight |
| Scientific literature search | **Limited Risk** | Transparency obligations (disclose AI involvement) |
| General database Q&A (internal tools) | **Minimal Risk** | No specific obligations |
| Real-time automated decision support | **High Risk** (context-dependent) | Human review before consequential decisions |

**Compliance requirements for High Risk systems:**
- Maintain detailed technical documentation.
- Implement robust logging of LLM inputs and outputs.
- Provide users with the ability to opt out of AI-assisted processing.
- Register the system in the EU AI Act database before deployment.

### 12.2 GDPR Considerations for LLM Processing

Under the **General Data Protection Regulation (GDPR)**, personal data processed by LLMs is subject to the same rights as any other processing:

- **Article 22 (Automated Decision-Making):** Individuals have the right not to be subject to solely automated decisions with significant effects. LLM-generated recommendations must be reviewable by a human.
- **Right to Erasure (Article 17):** If personal data was used in LoRA fine-tuning, the model may need to be retrained if the data subject invokes the right to erasure (machine unlearning).
- **Data Minimization (Article 5):** LLM context windows must not contain more personal data than necessary for the query.
- **Purpose Limitation:** Personal data collected for primary database storage cannot be reused to train LLM adapters without explicit legal basis.

**ThemisDB implementation:**
- Audit log for all LLM invocations (`src/llm/monitoring/`).
- Personal data masking in prompt assembly for non-anonymized datasets.
- LoRA training datasets are tracked with provenance metadata.

### 12.3 ISO/IEC Standards for AI

| Standard | Scope | Relevance |
|---|---|---|
| ISO/IEC 42001:2023 | AI Management System | Governance framework for AI deployment |
| ISO/IEC 23053:2022 | Framework for AI using ML | Risk assessment methodology |
| ISO/IEC TR 24028:2020 | AI trustworthiness | Bias, robustness, explainability |
| ISO/IEC 5259 series | Data quality for AI | Training data governance |

### 12.4 NIST AI Risk Management Framework

The **NIST AI RMF** (NIST AI 100-1, 2023) defines four functions for AI risk management: **GOVERN, MAP, MEASURE, MANAGE**. For ThemisDB LLM integration:

- **GOVERN:** Define organizational roles for LLM oversight; establish acceptable use policies.
- **MAP:** Identify AI contexts (which queries invoke LLM); catalog data flows.
- **MEASURE:** Instrument RAGAS metrics, hallucination rates, latency SLAs.
- **MANAGE:** Implement circuit breakers, model rollback, and human escalation paths.

---

## 13. ThemisDB Integration Architecture

### 13.1 LLM Subsystem Component Map

```
┌─────────────────────────────────────────────────────────┐
│                    ThemisDB LLM Subsystem                │
├─────────────────────────────────────────────────────────┤
│  API Layer         │ OpenAI-Compatible HTTP API          │
│                    │ (docs/en/llm/HTTP_SERVER_INTEGRATION)│
├─────────────────────────────────────────────────────────┤
│  Prompt Layer      │ PromptManager + PromptEnhancementEngine│
│                    │ (src/prompt_engineering/)           │
├─────────────────────────────────────────────────────────┤
│  RAG Layer         │ Vector Search + Context Assembly    │
│                    │ (src/rag/, src/vector/)             │
├─────────────────────────────────────────────────────────┤
│  Inference Layer   │ llama.cpp (LlamaWrapper)            │
│                    │ + Multi-LoRA Manager                │
│                    │ (src/llm/)                          │
├─────────────────────────────────────────────────────────┤
│  Monitoring Layer  │ Grafana Metrics + Audit Log         │
│                    │ (src/llm/monitoring/)               │
├─────────────────────────────────────────────────────────┤
│  Security Layer    │ Input Sanitizer + Output Validator  │
│                    │ + Access-Control-Aware Retrieval    │
└─────────────────────────────────────────────────────────┘
```

### 13.2 Research-to-Module Mapping

| Research Area | Key Papers | ThemisDB Module |
|---|---|---|
| Transformer inference | [1], [22], [23], [27] | `src/llm/` (llama.cpp wrapper) |
| Embeddings | [4], [5], [18] | `src/vector/`, `src/rag/` |
| Prompt engineering | [6], [7], [8], [11] | `src/prompt_engineering/`, `config/prompts/` |
| Prompt optimization | [9], [10], [12], [13], [14], [15] | `src/prompt_engineering/PromptEnhancementEngine` |
| RAG | [24], [25], [26] | `src/rag/` |
| LoRA / fine-tuning | [16], [17], [19], [20] | `src/llm/lora/` |
| Edge inference | [21], [22], [23] | `src/llm/` (llama.cpp) |
| Model evaluation | [26], [31], [32] | `docs/en/llm/LLM_BENCHMARKING_GUIDE.md` |
| Security | [28], [29], OWASP | `src/llm/` (security layer) |
| Compliance | [33], GDPR | `src/llm/monitoring/` |

---

## 14. References (IEEE Format)

### Transformer Architecture & Scaling

[1] A. Vaswani, N. Shazeer, N. Parmar, J. Uszkoreit, L. Jones, A. N. Gomez, Ł. Kaiser, and I. Polosukhin, "Attention is all you need," in *Advances in Neural Information Processing Systems (NeurIPS)*, vol. 30, 2017. [Online]. Available: https://arxiv.org/abs/1706.03762

[2] J. Kaplan, S. McCandlish, T. Henighan, T. B. Brown, B. Chess, R. Child, S. Gray, A. Radford, J. Wu, and D. Amodei, "Scaling laws for neural language models," *arXiv preprint arXiv:2001.08361*, Jan. 2020. [Online]. Available: https://arxiv.org/abs/2001.08361

[3] J. Hoffmann, S. Borgeaud, A. Mensch, E. Buchatskaya, T. Cai, E. Rutherford, D. de las Casas, L. A. Hendrycks, J. Welbl, A. Clark *et al.*, "Training compute-optimal large language models," in *Advances in Neural Information Processing Systems (NeurIPS)*, vol. 35, 2022. [Online]. Available: https://arxiv.org/abs/2203.15556

### Embeddings & Representations

[4] J. Devlin, M.-W. Chang, K. Lee, and K. Toutanova, "BERT: Pre-training of deep bidirectional transformers for language understanding," in *Proc. NAACL-HLT*, Minneapolis, MN, USA, 2019, pp. 4171–4186. doi: 10.18653/v1/N19-1423

[5] N. Reimers and I. Gurevych, "Sentence-BERT: Sentence embeddings using Siamese BERT-networks," in *Proc. EMNLP*, Hong Kong, China, 2019, pp. 3982–3992. doi: 10.18653/v1/D19-1410

### Prompt Engineering

[6] T. B. Brown, B. Mann, N. Ryder, M. Subbiah, J. Kaplan, P. Dhariwal, A. Neelakantan, P. Shyam, G. Sastry, A. Askell *et al.*, "Language models are few-shot learners," in *Advances in Neural Information Processing Systems (NeurIPS)*, vol. 33, 2020, pp. 1877–1901. [Online]. Available: https://arxiv.org/abs/2005.14165

[7] J. Wei, X. Wang, D. Schuurmans, M. Bosma, B. Ichter, F. Xia, E. Chi, Q. Le, and D. Zhou, "Chain-of-thought prompting elicits reasoning in large language models," in *Advances in Neural Information Processing Systems (NeurIPS)*, vol. 35, 2022. [Online]. Available: https://arxiv.org/abs/2201.11903

[8] J. White, Q. Fu, S. Hays, M. Sandborn, C. Olea, H. Gilbert, A. Elnashar, J. Spencer-Smith, and D. C. Schmidt, "A prompt pattern catalog to enhance prompt engineering with ChatGPT," *arXiv preprint arXiv:2302.11382*, Feb. 2023. [Online]. Available: https://arxiv.org/abs/2302.11382

### Autonomous Prompt Optimization

[9] Y. Zhou, A. I. Muresanu, Z. Han, K. Paster, S. Pitis, H. Chan, and J. Ba, "Large language models are human-level prompt engineers," in *Proc. ICLR*, Kigali, Rwanda, 2023. [Online]. Available: https://arxiv.org/abs/2211.01910

[10] R. Pryzant, D. Iter, J. Li, L. Y. Lee, C. Zhu, and M. Zeng, "Automatic prompt optimization with 'gradient descent' and beam search," in *Proc. EMNLP*, Singapore, 2023, pp. 7957–7968. doi: 10.18653/v1/2023.emnlp-main.494

[11] O. Rubin, J. Herzig, and J. Berant, "Learning to retrieve prompts for in-context learning," in *Proc. NAACL-HLT*, Seattle, WA, USA, 2022, pp. 1523–1535. doi: 10.18653/v1/2022.naacl-main.191

[12] Q. Guo, R. Wang, J. Guo, B. Li, K. Song, X. Tan, G. Liu, J. Bian, and Y. Yang, "Connecting large language models with evolutionary algorithms yields powerful prompt optimizers," in *Proc. ICLR*, Vienna, Austria, 2024. [Online]. Available: https://arxiv.org/abs/2309.08532

[13] A. Madaan, N. Tandon, P. Gupta, S. Hallinan, L. Gao, S. Wiegreffe, U. Alon, N. Dziri, S. Prabhumoye, Y. Yang *et al.*, "Self-refine: Iterative refinement with self-feedback," in *Advances in Neural Information Processing Systems (NeurIPS)*, vol. 36, 2023. [Online]. Available: https://arxiv.org/abs/2303.17651

[14] N. Shinn, F. Cassano, A. Gopinath, K. Narasimhan, and S. Yao, "Reflexion: Language agents with verbal reinforcement learning," in *Advances in Neural Information Processing Systems (NeurIPS)*, vol. 36, 2023. [Online]. Available: https://arxiv.org/abs/2303.11366

[15] C. Fernando, D. Banarse, H. Michalewski, S. Osindero, and T. Rocktäschel, "Promptbreeder: Self-referential self-improvement via prompt evolution," *arXiv preprint arXiv:2309.16797*, Sep. 2023. [Online]. Available: https://arxiv.org/abs/2309.16797

### Parameter-Efficient Fine-Tuning

[16] E. J. Hu, Y. Shen, P. Wallis, Z. Allen-Zhu, Y. Li, S. Wang, L. Wang, and W. Chen, "LoRA: Low-rank adaptation of large language models," in *Proc. ICLR*, Virtual Conference, 2022. [Online]. Available: https://arxiv.org/abs/2106.09685

[17] T. Dettmers, A. Pagnoni, A. Holtzman, and L. Zettlemoyer, "QLoRA: Efficient finetuning of quantized LLMs," in *Advances in Neural Information Processing Systems (NeurIPS)*, vol. 36, 2023. [Online]. Available: https://arxiv.org/abs/2305.14314

[18] A. Kusupati, G. Bhatt, A. Rege, M. Wallingford, A. Sinha, V. Ramanujan, W. Howard-Snyder, K. Chen, S. Kakade, P. Jain, and A. Farhadi, "Matryoshka representation learning," in *Advances in Neural Information Processing Systems (NeurIPS)*, vol. 35, 2022. [Online]. Available: https://arxiv.org/abs/2205.13147

[19] B. Lester, R. Al-Rfou, and N. Constant, "The power of scale for parameter-efficient prompt tuning," in *Proc. EMNLP*, Online/Punta Cana, Dominican Republic, 2021, pp. 3045–3059. doi: 10.18653/v1/2021.emnlp-main.243

[20] X. L. Li and P. Liang, "Prefix-tuning: Optimizing continuous prompts for generation," in *Proc. ACL*, Online, 2021, pp. 4582–4597. doi: 10.18653/v1/2021.acl-long.353

### Edge AI & Inference Optimization

[21] G. Gerganov, "llama.cpp: LLaMA inference in pure C/C++," GitHub Repository, 2023. [Online]. Available: https://github.com/ggerganov/llama.cpp

[22] T. Dao, D. Y. Fu, S. Ermon, A. Rudra, and C. Ré, "FlashAttention: Fast and memory-efficient exact attention with IO-awareness," in *Advances in Neural Information Processing Systems (NeurIPS)*, vol. 35, 2022. [Online]. Available: https://arxiv.org/abs/2205.14135

[23] C. Chen, S. Borgeaud, G. Irving, J.-B. Lespiau, L. Sifre, and J. Jumper, "Accelerating large language model decoding with speculative sampling," *arXiv preprint arXiv:2302.01318*, Feb. 2023. [Online]. Available: https://arxiv.org/abs/2302.01318

### Retrieval-Augmented Generation

[24] P. Lewis, E. Perez, A. Piktus, F. Petroni, V. Karpukhin, N. Goyal, H. Küttler, M. Lewis, W.-T. Yih, T. Rocktäschel, S. Riedel, and D. Kiela, "Retrieval-augmented generation for knowledge-intensive NLP tasks," in *Advances in Neural Information Processing Systems (NeurIPS)*, vol. 33, 2020, pp. 9459–9474. [Online]. Available: https://arxiv.org/abs/2005.11401

[25] V. Karpukhin, B. Oğuz, S. Min, P. Lewis, L. Wu, S. Edunov, D. Chen, and W.-T. Yih, "Dense passage retrieval for open-domain question answering," in *Proc. EMNLP*, Online, 2020, pp. 6769–6781. doi: 10.18653/v1/2020.emnlp-main.550

[26] S. Es, J. James, L. Espinosa-Anke, and S. Schockaert, "RAGAS: Automated evaluation of retrieval augmented generation," in *Proc. EACL*, Malta, 2024, pp. 150–153. [Online]. Available: https://arxiv.org/abs/2309.15217

[27] W. Kwon, Z. Li, S. Zhuang, Y. Sheng, L. Zheng, C. H. Yu, J. Gonzalez, H. Zhang, and I. Stoica, "Efficient memory management for large language model serving with PagedAttention," in *Proc. SOSP*, Koblenz, Germany, 2023, pp. 611–626. doi: 10.1145/3600006.3613165

### Security

[28] F. Perez and I. Ribeiro, "Ignore previous prompt: Attack techniques for language models," *arXiv preprint arXiv:2211.09527*, Nov. 2022. [Online]. Available: https://arxiv.org/abs/2211.09527

[29] K. Greshake, S. Abdelnabi, S. Mishra, C. Endres, T. Holz, and M. Fritz, "Not what you've signed up for: Compromising real-world LLM-integrated applications with indirect prompt injection," in *Proc. AISec Workshop (ACM CCS)*, Copenhagen, Denmark, 2023. [Online]. Available: https://arxiv.org/abs/2302.12173

[30] T. Wolf, L. Debut, V. Sanh, J. Chaumond, C. Delangue, A. Moi, P. Cistac, T. Rault, R. Louf, M. Funtowicz *et al.*, "Transformers: State-of-the-art natural language processing," in *Proc. EMNLP (Systems Demonstrations)*, Online, 2020, pp. 38–45. doi: 10.18653/v1/2020.emnlp-demos.6

### Model Evaluation

[31] D. Hendrycks, C. Burns, S. Basart, A. Zou, M. Mazeika, D. Song, and J. Steinhardt, "Measuring massive multitask language understanding," in *Proc. ICLR*, Virtual Conference, 2021. [Online]. Available: https://arxiv.org/abs/2009.03300

[32] L. Zheng, W.-L. Chiang, Y. Sheng, S. Zhuang, Z. Wu, Y. Zhuang, Z. Li, Z. Li, D. Li, E. P. Xing, H. Zhang, J. E. Gonzalez, and I. Stoica, "Judging LLM-as-a-judge with MT-Bench and Chatbot Arena," in *Advances in Neural Information Processing Systems (NeurIPS)*, vol. 36, 2023. [Online]. Available: https://arxiv.org/abs/2306.05685

### Regulatory & Compliance

[33] European Parliament and the Council of the European Union, "Regulation (EU) 2024/1689 of the European Parliament and of the Council laying down harmonised rules on artificial intelligence (Artificial Intelligence Act)," *Official Journal of the European Union*, vol. L, 2024. [Online]. Available: https://eur-lex.europa.eu/legal-content/EN/TXT/?uri=CELEX:32024R1689

---

## See Also

### ThemisDB LLM Documentation

- [`docs/en/llm/README.md`](../en/llm/README.md) — LLM module overview
- [`docs/en/llm/LLM_LORA_LLAMACPP_INTEGRATION.md`](../en/llm/LLM_LORA_LLAMACPP_INTEGRATION.md) — LoRA + llama.cpp integration
- [`docs/en/llm/LLM_BENCHMARKING_GUIDE.md`](../en/llm/LLM_BENCHMARKING_GUIDE.md) — Benchmarking guide
- [`docs/en/llm/FLASH_ATTENTION_IMPLEMENTATION.md`](../en/llm/FLASH_ATTENTION_IMPLEMENTATION.md) — Flash Attention
- [`docs/en/llm/SPECULATIVE_DECODING_IMPLEMENTATION.md`](../en/llm/SPECULATIVE_DECODING_IMPLEMENTATION.md) — Speculative Decoding
- [`docs/en/llm/GRAMMAR_CONSTRAINED_GENERATION.md`](../en/llm/GRAMMAR_CONSTRAINED_GENERATION.md) — Grammar-constrained generation
- [`docs/en/llm/LORA_TRAINING_GUIDE.md`](../en/llm/LORA_TRAINING_GUIDE.md) — LoRA training pipeline

### Related Research

- [`docs/research/PROMPT_ENHANCEMENT_ENGINE_OPTIMIZATION_RESEARCH.md`](PROMPT_ENHANCEMENT_ENGINE_OPTIMIZATION_RESEARCH.md) — Prompt optimization deep-dive
- [`docs/research/PROMPT_OPTIMIZATION_IMPLEMENTATION_STRATEGY.md`](PROMPT_OPTIMIZATION_IMPLEMENTATION_STRATEGY.md) — Implementation strategy
- [`docs/research/KNOWLEDGE_GRAPH_EMBEDDINGS_RESEARCH.md`](KNOWLEDGE_GRAPH_EMBEDDINGS_RESEARCH.md) — Knowledge graph embeddings
- [`docs/research/GPU_VECTOR_INDEXING_RESEARCH.md`](GPU_VECTOR_INDEXING_RESEARCH.md) — GPU vector indexing

---

*Last Updated: April 2026*  
*Next Review: September 2026*
