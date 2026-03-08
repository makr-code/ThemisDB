# Core Scientific Foundations for RAG (Retrieval-Augmented Generation) in ThemisDB

## Overview

This document surveys the peer-reviewed literature and state-of-the-art techniques that underpin the Retrieval-Augmented Generation (RAG) subsystem in ThemisDB. References are formatted in **IEEE citation style**. Topic areas include hybrid retrieval, neural scoring, LLM integration, faithfulness/relevance/completeness evaluation, knowledge gap detection, bias mitigation, multi-modal RAG architectures, and automated evaluation frameworks.

---

## Table of Contents

1. [Foundational RAG Architecture](#1-foundational-rag-architecture)
2. [Retrieval Techniques — Sparse, Dense, and Hybrid](#2-retrieval-techniques--sparse-dense-and-hybrid)
3. [Neural Scoring and Re-Ranking](#3-neural-scoring-and-re-ranking)
4. [Faithfulness, Relevance, and Completeness](#4-faithfulness-relevance-and-completeness)
5. [Knowledge Gap Detection](#5-knowledge-gap-detection)
6. [LLM Integration and LLM-as-Judge](#6-llm-integration-and-llm-as-judge)
7. [Multi-Modal RAG Architectures](#7-multi-modal-rag-architectures)
8. [Bias Mitigation in RAG Systems](#8-bias-mitigation-in-rag-systems)
9. [Evaluation Frameworks and Metrics](#9-evaluation-frameworks-and-metrics)
10. [Agentic and Self-Improving RAG](#10-agentic-and-self-improving-rag)
11. [Full Reference List (IEEE)](#11-full-reference-list-ieee)

---

## 1. Foundational RAG Architecture

The standard RAG paradigm combines a **retriever** (which fetches relevant passages from a document store) with a **generator** (an LLM that conditions its output on those passages). This architecture was formally introduced by Lewis et al. [1] and has since become the dominant approach for knowledge-intensive NLP tasks.

### 1.1 Retriever–Generator Coupling

The canonical formulation marginalises over a set of retrieved documents *z*:

```text
p(y | x) = Σ_z  p_η(z | x) · p_θ(y | x, z)
```

where *x* is the query, *y* is the generated answer, *η* are retriever parameters, and *θ* are generator parameters. End-to-end training of both components simultaneously was demonstrated in [1] and later refined by Izacard and Grave [2] (Fusion-in-Decoder).

### 1.2 Pre-Training with Retrieval

Guu et al. [3] (REALM) showed that retrieval can be incorporated during **pre-training**, allowing the language model to learn latent representations that are inherently aligned with document retrieval. This improves few-shot and zero-shot generalisation on downstream tasks.

### 1.3 In-Context Retrieval

Ram et al. [4] introduced *In-Context RAG*, where retrieved passages are prepended directly to the prompt at inference time without any additional training, making the approach model-agnostic. ThemisDB supports this pattern via the `themis::rag` namespace.

---

## 2. Retrieval Techniques — Sparse, Dense, and Hybrid

### 2.1 Sparse Retrieval: BM25

BM25, described formally by Robertson and Zaragoza [5], models term-frequency saturation and document-length normalisation:

```text
BM25(D, Q) = Σ_i  IDF(q_i) · (f(q_i, D) · (k₁ + 1)) / (f(q_i, D) + k₁ · (1 − b + b · |D| / avgdl))
```

BM25 remains a strong baseline and is the retriever of choice in production systems where low latency is critical (e.g., ThemisDB's `NeuralSparseRetriever`).

### 2.2 Dense Retrieval: DPR and Sentence-BERT

Karpukhin et al. [6] (DPR — Dense Passage Retrieval) showed that bi-encoder models trained with in-batch negatives significantly outperform BM25 on open-domain QA benchmarks. Reimers and Gurevych [7] (Sentence-BERT) introduced the Siamese network fine-tuning approach that enables semantically meaningful cosine-similarity comparisons.

### 2.3 Hybrid Retrieval

Neither sparse nor dense retrieval dominates across all query types [8]. Hybrid strategies combine both:

- **Score fusion**: Reciprocal Rank Fusion (RRF) linearly combines ranked lists from multiple retrievers without requiring score normalisation [9].
- **Learned fusion**: A learned weighting function is trained to optimise retrieval recall end-to-end.
- **Neural sparse retrieval**: SPLADE [10] generates sparse, token-weighted representations from a BERT encoder, offering the interpretability of sparse retrieval with the semantic richness of dense models.

ThemisDB implements hybrid retrieval in `include/search/neural_sparse_retrieval.h` and `benchmarks/bench_rag_hybrid_retriever.cpp`.

---

## 3. Neural Scoring and Re-Ranking

### 3.1 Cross-Encoder Re-Ranking

After an initial recall step, a cross-encoder re-ranker reads the concatenation `[query; document]` and produces a scalar relevance score. This approach enables full bidirectional attention between query and document, yielding substantially higher precision. Nogueira and Cho [11] demonstrated large improvements on MS MARCO using BERT-based cross-encoders.

### 3.2 Mono- and Duo-Transformer Architectures

Nogueira et al. [12] introduced a pipeline of:

1. **MonoBERT** — point-wise scoring of individual passages.
2. **DuoBERT** — pairwise scoring, comparing two passages relative to the query.

This cascade approach trades latency for precision, and is suitable for offline batch evaluation in ThemisDB's evaluation pipeline.

### 3.3 Listwise Ranking with LLMs

Zhuang et al. [13] showed that instruction-tuned LLMs can perform **listwise re-ranking** — ranking a complete list of candidates in a single forward pass — outperforming cross-encoders on several benchmarks while remaining efficient due to prefix caching.

---

## 4. Faithfulness, Relevance, and Completeness

These three dimensions form the primary quality axes evaluated by the `themis::rag::judge` component.

### 4.1 Faithfulness

Faithfulness measures whether every factual claim in the generated answer is **entailed** by the retrieved context, with no hallucinated information. Formal treatment appears in:

- Manakul et al. [14] (SelfCheckGPT): hallucination detection via sampling consistency.
- Liu et al. [15]: verifiability evaluation for generative search engines — each sentence in the answer is attributed to a source passage and checked for support.
- Es et al. [16] (RAGAS): defines faithfulness as the fraction of claims in the answer that can be inferred from the context:

```text
Faithfulness = |{claims inferred from context}| / |{total claims in answer}|
```

### 4.2 Relevance

Answer Relevance measures how closely the generated response addresses the original query, independent of factual correctness. RAGAS [16] computes relevance by generating synthetic questions from the answer and measuring the mean cosine similarity back to the original question.

Context Relevance measures whether retrieved passages are on-topic, penalising the inclusion of irrelevant noise documents that may distract the generator [17].

### 4.3 Completeness

Completeness (also called *context recall* in RAGAS) quantifies how much of the reference answer's information can be attributed to the retrieved context. Formally:

```text
Context Recall = |{ground truth sentences attributable to context}| / |{total ground truth sentences}|
```

Ensuring high completeness requires sufficient corpus coverage and robust retrieval diversity, topics discussed in Shi et al. [18] and Mallen et al. [19].

---

## 5. Knowledge Gap Detection

Knowledge Gap Detection identifies cases where the retrieved documents are **insufficient** to support a reliable answer, triggering additional retrieval, query rewriting, or an explicit abstention signal.

### 5.1 Forward-Looking Active Retrieval (FLARE)

Jiang et al. [20] introduced FLARE, which monitors the generation probability of upcoming tokens. When the model's confidence drops below a threshold, it pauses generation, uses the low-confidence tokens as a retrieval query, and fetches supplementary context before resuming:

```text
If min_i P(t_i | context) < τ:
    retrieve(query derived from {t_i})
```

### 5.2 Self-RAG

Asai et al. [21] trained a model end-to-end with four types of **reflection tokens** (`[Retrieve]`, `[IsRel]`, `[IsSup]`, `[IsUse]`) that control retrieval decisions and evaluate whether retrieved passages support the generated text. Self-RAG outperforms standard RAG on seven tasks while reducing unnecessary retrieval calls.

### 5.3 Uncertainty Quantification

Confidence-based gap detection relies on:

- **Token entropy**: `H = −Σ_i p_i log p_i` over the next-token distribution.
- **Perplexity**: `PP(y) = exp(−(1/N) Σ_i log p(y_i | y_{<i}, context))`.
- **Self-consistency** [22]: sampling multiple generations and measuring agreement; low consistency implies high uncertainty.

### 5.4 Coverage Analysis

Entity and concept coverage checks verify that all key aspects of the query are addressed by at least one retrieved document. The `themis::rag::knowledge_gap` component implements similarity-based, document-count, and entropy-based detectors across `FAST`, `BALANCED`, and `THOROUGH` detection modes.

---

## 6. LLM Integration and LLM-as-Judge

### 6.1 LLM-as-Judge

Using a powerful LLM to evaluate the quality of another LLM's output was formalised by Zheng et al. [23] (MT-Bench / Chatbot Arena). Key findings:

- GPT-4 as a judge achieves over **80 % agreement** with human preferences on most categories.
- Position bias, verbosity bias, and self-enhancement bias are significant failure modes that require mitigation.

### 6.2 G-Eval

Liu et al. [24] introduced G-Eval, which uses chain-of-thought (CoT) prompting to generate evaluation steps prior to scoring. The final score is computed as the probability-weighted sum over discrete score levels:

```text
Score = Σ_s  s · p(score = s | CoT steps, evaluation criteria)
```

G-Eval achieves Spearman correlations with human judgements that surpass prior learned metrics on summarisation and dialogue benchmarks.

### 6.3 GPTScore

Fu et al. [25] proposed GPTScore, which scores generated text by measuring the log-probability of the evaluation target given a task-specific prompt, making it training-free and generalising to arbitrary evaluation aspects.

### 6.4 Constitutional AI and RLHF

Constitutional AI [26] trains LLMs to self-critique and revise outputs against a set of principles. In a RAG context, constitutional principles can be applied to enforce faithfulness constraints during the judge step, similar to the approach used in `plugins/ethics_ai/rag_context_engine`.

### 6.5 Bias Mitigation in LLM-as-Judge

To mitigate the biases documented in [23] and [24]:

| Bias Type | Mitigation Strategy |
| --- | --- |
| **Position bias** | Swap answer order; average both orderings |
| **Verbosity bias** | Length-normalise scores; use CoT instructions |
| **Self-enhancement bias** | Use a different judge model from the generator |
| **Sycophancy** | Calibrate with human-labelled anchor examples |

ThemisDB's `RagJudge` ensemble evaluator implements swap-consistency checks and cross-model judging to reduce these biases.

---

## 7. Multi-Modal RAG Architectures

### 7.1 Vision-Language RAG

Multi-modal RAG extends retrieval to include image, audio, and video modalities. Chen et al. [27] (MuRAG) introduced a retrieval-augmented model that retrieves both text passages and images, fusing them via a shared encoder. This is foundational for ThemisDB's `multimodal_rag` module (`include/rag/multimodal_rag.h`).

### 7.2 Document-Level Multi-Modal Retrieval

Yasunaga et al. [28] (Re-Imagen) demonstrated that jointly retrieving multi-modal context and conditioning image generation on retrieved examples substantially improves fidelity and semantic coherence. The underlying principle — grounding generation on heterogeneous retrieved evidence — applies equally to text-centric RAG systems that incorporate figure, table, or chart retrieval.

### 7.3 Audio and Video Modalities

Multi-modal RAG is being extended to audio transcription retrieval (speech-to-text indexing), video keyframe retrieval, and cross-modal consistency checks. ThemisDB's voice pipeline (`include/voice/voice_audio_storage.h`) and vision support (`docs/en/llm/VISION_SUPPORT_IMPLEMENTATION.md`) provide the building blocks for these extensions.

### 7.4 Late vs. Early Fusion

| Strategy | Description | Strength |
| --- | --- | --- |
| **Early fusion** | Modalities are concatenated before encoding | Richer cross-modal attention |
| **Late fusion** | Separate encoders; scores combined at ranking stage | Modular, easy to extend |
| **Intermediate fusion** | Cross-attention between modality-specific encoder outputs | Balance of both |

---

## 8. Bias Mitigation in RAG Systems

### 8.1 Retrieval Bias

If the document corpus over-represents certain viewpoints, the retrieved context will be systematically biased. Mitigation strategies include:

- **Corpus diversification**: maintaining balanced coverage across sources, dates, and perspectives.
- **Maximal Marginal Relevance (MMR)** [29]: selects documents that are relevant to the query but dissimilar to already-selected documents, increasing diversity.
- **Proportional representation constraints**: enforcing source-level quotas at retrieval time.

### 8.2 Generator Bias

LLMs trained on web-scale corpora inherit social and cultural biases. Addressing these in RAG:

- **Counterfactual data augmentation**: training on examples where sensitive attributes are varied to ensure output stability [30].
- **Differential privacy**: adding calibrated noise to embedding representations to prevent membership inference.
- **Fairness-aware fine-tuning**: incorporating demographic parity or equal opportunity objectives into the loss function.

### 8.3 Faithfulness as a Bias Mitigation Tool

Grounding answers strictly in retrieved context limits the extent to which the generator's intrinsic biases can contaminate the output. High faithfulness scores (Section 4.1) are therefore also an indirect measure of bias reduction.

### 8.4 Ethical RAG

ThemisDB's `ethics_ai` plugin (`plugins/ethics_ai/rag_context_engine`) integrates ethical evaluation dimensions into the retrieval-generation pipeline, enforcing guidelines derived from the EU AI Act and the GDPR. Scientific backing includes Weidinger et al. [31] (taxonomy of language model harms) and Bender et al. [32] (stochastic parrots).

---

## 9. Evaluation Frameworks and Metrics

### 9.1 RAGAS

Shahul Es et al. [16] introduced RAGAS, a reference-free evaluation framework computing four metrics:

| Metric | Definition |
| --- | --- |
| **Faithfulness** | Fraction of answer claims supported by context |
| **Answer Relevance** | Mean cosine similarity of synthetic re-generated questions to original query |
| **Context Precision** | Fraction of retrieved context that is relevant |
| **Context Recall** | Fraction of ground-truth answer attributable to context |

### 9.2 BERTScore

Zhang et al. [33] compute BERTScore by matching contextual embeddings between generated and reference tokens via greedy matching, providing a soft n-gram overlap metric that is more semantically sensitive than BLEU or ROUGE.

### 9.3 ROUGE and BLEU

ROUGE [34] measures n-gram recall between generated and reference text and is the standard metric for summarisation. BLEU [35] measures n-gram precision, primarily used for machine translation. Both are complementary baselines in ThemisDB's evaluation report exporter (`test_rag_evaluation_report_exporter.cpp`).

### 9.4 MT-Bench and Chatbot Arena

Zheng et al. [23] propose two complementary evaluation protocols: **MT-Bench** (automated multi-turn benchmark, judge by GPT-4) and **Chatbot Arena** (human pairwise comparison via ELO ratings). Together they provide a robust proxy for real-world user preference.

### 9.5 TruLens and ARES

- **TruLens** wraps any LLM application and measures RAG triad (answer relevance, context relevance, groundedness) in production.
- **ARES** [36] trains lightweight classifier judges from a small set of human-labelled examples, enabling scalable domain-specific RAG evaluation without relying on GPT-4.

---

## 10. Agentic and Self-Improving RAG

### 10.1 Agentic RAG

ReAct [37] interleaves LLM reasoning (Thought) with tool-use actions (Act), enabling multi-step retrieval workflows in which the agent iteratively refines its queries. ThemisDB's `AgenticRAG` module (`include/rag/agentic_rag.h`) implements the ReAct-style loop for multi-hop question answering.

### 10.2 Self-Reflective and Corrective RAG

Yan et al. [38] (CRAG — Corrective RAG) adds a lightweight retrieval evaluator that classifies retrieved documents into three quality bands (Correct, Incorrect, Ambiguous) and triggers retrieval correction actions accordingly, improving robustness on noisy corpora.

### 10.3 Iterative Retrieval

Shao et al. [39] (IRCoT — Interleaved Retrieval with Chain-of-Thought) show that interleaving retrieval steps with CoT reasoning chains substantially improves multi-hop QA performance by allowing each reasoning step to trigger a targeted retrieval action.

### 10.4 RAG-Fusion

RAG-Fusion [40] generates multiple query variants, retrieves independently for each, and re-ranks the merged result list via Reciprocal Rank Fusion. This improves recall especially for ambiguous or underspecified queries.

---

## 11. Full Reference List (IEEE)

### Core RAG Architecture

[1] P. Lewis, E. Perez, A. Piktus, F. Petroni, V. Karpukhin, N. Goyal, H. Küttler, M. Lewis, W.-t. Yih, T. Rocktäschel, S. Riedel, and D. Kiela, "Retrieval-Augmented Generation for Knowledge-Intensive NLP Tasks," in *Proc. 34th Conf. Neural Inf. Process. Syst. (NeurIPS)*, vol. 33, Virtual, Dec. 2020, pp. 9459–9474. [Online]. Available: https://proceedings.neurips.cc/paper/2020/hash/6b493230205f780e1bc26945df7481e5-Abstract.html

[2] G. Izacard and E. Grave, "Leveraging Passage Retrieval with Generative Models for Open Domain Question Answering," in *Proc. 16th Conf. European Chapter Assoc. Comput. Linguistics (EACL)*, Kyiv, Ukraine, Apr. 2021, pp. 874–880, doi: 10.18653/v1/2021.eacl-main.74. [Online]. Available: https://aclanthology.org/2021.eacl-main.74

[3] K. Guu, K. Lee, Z. Tung, P. Pasupat, and M.-W. Chang, "REALM: Retrieval-Augmented Language Model Pre-Training," in *Proc. 37th Int. Conf. Machine Learning (ICML)*, vol. 119, Vienna, Austria, Jul. 2020, pp. 3929–3938. [Online]. Available: http://proceedings.mlr.press/v119/guu20a.html

[4] O. Ram, Y. Levine, I. Dalmedigos, D. Muhlgay, A. Shashua, K. Leyton-Brown, and Y. Shoham, "In-Context Retrieval-Augmented Language Models," *Trans. Assoc. Comput. Linguistics*, vol. 11, pp. 1316–1331, Oct. 2023, doi: 10.1162/tacl_a_00605. [Online]. Available: https://arxiv.org/abs/2302.00083

### Retrieval Techniques

[5] S. Robertson and H. Zaragoza, "The Probabilistic Relevance Framework: BM25 and Beyond," *Found. Trends Inf. Retr.*, vol. 3, no. 4, pp. 333–389, Apr. 2009, doi: 10.1561/1500000019.

[6] V. Karpukhin, B. Oğuz, S. Min, P. Lewis, L. Wu, S. Edunov, D. Chen, and W.-t. Yih, "Dense Passage Retrieval for Open-Domain Question Answering," in *Proc. 2020 Conf. Empirical Methods Natural Language Process. (EMNLP)*, Virtual, Nov. 2020, pp. 6769–6781, doi: 10.18653/v1/2020.emnlp-main.550.

[7] N. Reimers and I. Gurevych, "Sentence-BERT: Sentence Embeddings using Siamese BERT-Networks," in *Proc. 2019 Conf. Empirical Methods Natural Language Process. 9th Int. Joint Conf. Natural Language Process. (EMNLP-IJCNLP)*, Hong Kong, China, Nov. 2019, pp. 3982–3992, doi: 10.18653/v1/D19-1410.

[8] J. Lin and X. Ma, "A Few Brief Notes on DeepImpact, COIL, and a Conceptual Framework for Information Retrieval Techniques," *arXiv preprint arXiv:2106.14807*, Jun. 2021. [Online]. Available: https://arxiv.org/abs/2106.14807

[9] G. V. Cormack, C. L. A. Clarke, and S. Buettcher, "Reciprocal Rank Fusion Outperforms Condorcet and Individual Rank Learning Methods," in *Proc. 32nd Int. ACM SIGIR Conf. Res. Dev. Inf. Retr.*, Boston, MA, USA, Jul. 2009, pp. 758–759, doi: 10.1145/1571941.1572114.

[10] T. Formal, B. Piwowarski, J. Piwowarski, and S. Clinchant, "SPLADE: Sparse Lexical and Expansion Model for First Stage Ranking," in *Proc. 44th Int. ACM SIGIR Conf. Res. Dev. Inf. Retr.*, Virtual, Jul. 2021, pp. 2288–2292, doi: 10.1145/3404835.3463098. [Online]. Available: https://arxiv.org/abs/2107.05720

### Neural Scoring and Re-Ranking

[11] R. Nogueira and K. Cho, "Passage Re-ranking with BERT," *arXiv preprint arXiv:1901.04085*, Jan. 2019. [Online]. Available: https://arxiv.org/abs/1901.04085

[12] R. Nogueira, W. Yang, J. Lin, and K. Cho, "Document Ranking with a Pretrained Sequence-to-Sequence Model," in *Proc. 2020 Conf. Findings Assoc. Comput. Linguistics: EMNLP 2020*, Virtual, Nov. 2020, pp. 708–718, doi: 10.18653/v1/2020.findings-emnlp.63.

[13] S. Zhuang, H. Zhuang, B. Koopman, and G. Zuccon, "A Setwise Approach for Effective and Highly Efficient Zero-shot Ranking with Large Language Models," in *Proc. 46th Int. ACM SIGIR Conf. Res. Dev. Inf. Retr.*, Taipei, Taiwan, Jul. 2023, pp. 38–47, doi: 10.1145/3539618.3591930. [Online]. Available: https://arxiv.org/abs/2310.09497

### Faithfulness, Relevance, and Completeness

[14] P. Manakul, A. Liusie, and M. J. F. Gales, "SelfCheckGPT: Zero-Resource Black-Box Hallucination Detection for Generative Large Language Models," in *Proc. 2023 Conf. Empirical Methods Natural Language Process. (EMNLP)*, Singapore, Dec. 2023, pp. 9004–9017. [Online]. Available: https://arxiv.org/abs/2303.08896

[15] N. Liu, T. Zhang, and P. Liang, "Evaluating Verifiability in Generative Search Engines," *arXiv preprint arXiv:2304.09848*, Apr. 2023. [Online]. Available: https://arxiv.org/abs/2304.09848

[16] S. Es, J. James, L. Espinosa-Anke, and S. Schockaert, "RAGAS: Automated Evaluation of Retrieval Augmented Generation," *arXiv preprint arXiv:2309.15217*, Sep. 2023. [Online]. Available: https://arxiv.org/abs/2309.15217

[17] H. Zhao, Z. Zhang, R. Zhang, Z. Liu, B. Du, and Y. Yao, "Knowing What LLMs DO NOT Know: A Simple Yet Effective Self-Detection Method," *arXiv preprint arXiv:2310.18477*, Oct. 2023. [Online]. Available: https://arxiv.org/abs/2310.18477

[18] F. Shi, X. Chen, K. Misra, N. Scales, D. Dohan, E. H. Chi, N. Schärli, and D. Zhou, "Large Language Models Can Be Easily Distracted by Irrelevant Context," in *Proc. 40th Int. Conf. Machine Learning (ICML)*, vol. 202, Honolulu, HI, USA, Jul. 2023, pp. 31210–31227. [Online]. Available: https://arxiv.org/abs/2302.00093

[19] A. Mallen, A. Asai, V. Zhong, R. Das, D. Khashabi, and H. Hajishirzi, "When Not to Trust Language Models: Investigating Effectiveness of Parametric and Non-Parametric Memories," in *Proc. 61st Annu. Meeting Assoc. Comput. Linguistics (ACL)*, Toronto, Canada, Jul. 2023, pp. 9802–9822, doi: 10.18653/v1/2023.acl-long.546.

### Knowledge Gap Detection

[20] Z. Jiang, F. F. Xu, L. Gao, Z. Sun, Q. Liu, J. Dwivedi-Yu, Y. Yang, J. Callan, and G. Neubig, "Active Retrieval Augmented Generation," in *Proc. 2023 Conf. Empirical Methods Natural Language Process. (EMNLP)*, Singapore, Dec. 2023, pp. 7969–7992. [Online]. Available: https://aclanthology.org/2023.emnlp-main.495

[21] A. Asai, Z. Wu, Y. Wang, A. Sil, and H. Hajishirzi, "Self-RAG: Learning to Retrieve, Generate, and Critique through Self-Reflection," *arXiv preprint arXiv:2310.11511*, Oct. 2023. [Online]. Available: https://arxiv.org/abs/2310.11511

[22] X. Wang, J. Wei, D. Schuurmans, Q. Le, E. Chi, S. Narang, A. Chowdhery, and D. Zhou, "Self-Consistency Improves Chain of Thought Reasoning in Language Models," in *Proc. 11th Int. Conf. Learning Representations (ICLR)*, Kigali, Rwanda, May 2023. [Online]. Available: https://openreview.net/forum?id=1PL1NIMMrw

### LLM Integration and LLM-as-Judge

[23] L. Zheng, W.-L. Chiang, Y. Sheng, S. Zhuang, Z. Wu, Y. Zhuang, Z. Lin, Z. Li, D. Li, E. P. Xing, H. Zhang, J. E. Gonzalez, and I. Stoica, "Judging LLM-as-a-Judge with MT-Bench and Chatbot Arena," in *Proc. 37th Conf. Neural Inf. Process. Syst. (NeurIPS)*, Datasets and Benchmarks Track, New Orleans, LA, USA, Dec. 2023. [Online]. Available: https://arxiv.org/abs/2306.05685

[24] Y. Liu, D. Iter, Y. Xu, S. Wang, R. Xu, and C. Zhu, "G-Eval: NLG Evaluation using GPT-4 with Better Human Alignment," *arXiv preprint arXiv:2303.16634*, May 2023. [Online]. Available: https://arxiv.org/abs/2303.16634

[25] J. Fu, S.-K. Ng, Z. Jiang, and P. Liu, "GPTScore: Evaluate as You Desire," *arXiv preprint arXiv:2302.04166*, Jun. 2023. [Online]. Available: https://arxiv.org/abs/2302.04166

[26] Y. Bai, A. Jones, K. Ndousse, A. Askell, A. Chen, N. DasSarma, D. Drain, S. Fort, D. Ganguli, T. Henighan, N. Joseph, S. Kadavath, J. Kernion, T. Conerly, S. El-Showk, N. Elhage, Z. Hatfield-Dodds, D. Hernandez, T. Hume, S. Johnston, S. Kravec, L. Lovitt, N. Nanda, C. Olsson, D. Amodei, T. Brown, J. Clark, S. McCandlish, C. Olah, B. Mann, and J. Kaplan, "Constitutional AI: Harmlessness from AI Feedback," Anthropic Technical Report, Dec. 2022. [Online]. Available: https://arxiv.org/abs/2212.08073

### Multi-Modal RAG Architectures

[27] H. Chen, R. Zhong, X. Pan, T. Jiang, Y. Song, R. Wang, X. Liu, and W. Wang, "MuRAG: Multimodal Retrieval-Augmented Generator for Open Question Answering over Images and Text," in *Proc. 2022 Conf. Empirical Methods Natural Language Process. (EMNLP)*, Abu Dhabi, UAE, Dec. 2022, pp. 5558–5570, doi: 10.18653/v1/2022.emnlp-main.375. [Online]. Available: https://aclanthology.org/2022.emnlp-main.375

[28] M. Yasunaga, A. Aghajanyan, W. Shi, R. James, J. Leskovec, P. Liang, M. Lewis, L. Zettlemoyer, and W.-t. Yih, "Retrieval-Augmented Multimodal Language Modeling," in *Proc. 40th Int. Conf. Machine Learning (ICML)*, vol. 202, Honolulu, HI, USA, Jul. 2023, pp. 39755–39769. [Online]. Available: https://arxiv.org/abs/2211.12561

### Bias Mitigation

[29] J. Carbonell and J. Goldstein, "The Use of MMR, Diversity-Based Reranking for Reordering Documents and Producing Summaries," in *Proc. 21st Annu. Int. ACM SIGIR Conf. Res. Dev. Inf. Retr.*, Melbourne, Australia, Aug. 1998, pp. 335–336, doi: 10.1145/290941.291025.

[30] T. Lu, D. Mardziel, F. Wu, P. Amancharla, and A. Datta, "Gender Bias in Neural Natural Language Processing," in *Logic, Language, and Security: Essays Dedicated to Andre Scedrov*, Cham, Switzerland: Springer, 2020, pp. 189–202, doi: 10.1007/978-3-030-62077-6_14. [Online]. Available: https://arxiv.org/abs/1807.11714

[31] L. Weidinger, J. Mellor, M. Rauh, C. Griffin, J. Uesato, P.-S. Huang, M. Cheng, M. Glaese, B. Balle, A. Kasirzadeh, Z. Kenton, S. Brown, W. Hawkins, T. Stepleton, C. Biles, A. Birhane, J. Haas, L. Rimell, L. A. Hendricks, W. Isaac, S. Legassick, G. Irving, and I. Gabriel, "Ethical and Social Risks of Harm from Language Models," *arXiv preprint arXiv:2112.04359*, Dec. 2021. [Online]. Available: https://arxiv.org/abs/2112.04359

[32] E. M. Bender, T. Gebru, A. McMillan-Major, and S. Shmitchell, "On the Dangers of Stochastic Parrots: Can Language Models Be Too Big?" in *Proc. 2021 ACM Conf. Fairness, Accountability, Transparency (FAccT)*, Virtual, Mar. 2021, pp. 610–623, doi: 10.1145/3442188.3445922.

### Evaluation Frameworks and Metrics

[33] T. Zhang, V. Kishore, F. Wu, K. Q. Weinberger, and Y. Artzi, "BERTScore: Evaluating Text Generation with BERT," in *Proc. 8th Int. Conf. Learning Representations (ICLR)*, Addis Ababa, Ethiopia, Apr. 2020. [Online]. Available: https://openreview.net/forum?id=SkeHuCVFDr

[34] C.-Y. Lin, "ROUGE: A Package for Automatic Evaluation of Summaries," in *Text Summarization Branches Out*, Barcelona, Spain, Jul. 2004, pp. 74–81. [Online]. Available: https://aclanthology.org/W04-1013

[35] K. Papineni, S. Roukos, T. Ward, and W.-J. Zhu, "BLEU: a Method for Automatic Evaluation of Machine Translation," in *Proc. 40th Annu. Meeting Assoc. Comput. Linguistics*, Philadelphia, PA, USA, Jul. 2002, pp. 311–318, doi: 10.3115/1073083.1073135.

[36] J. Saad-Falcon, O. Khattab, C. Potts, and M. Zaharia, "ARES: An Automated Evaluation Framework for Retrieval-Augmented Generation Systems," *arXiv preprint arXiv:2311.09476*, Nov. 2023. [Online]. Available: https://arxiv.org/abs/2311.09476

### Agentic and Self-Improving RAG

[37] S. Yao, J. Zhao, D. Yu, N. Du, I. Shafran, K. Narasimhan, and Y. Cao, "ReAct: Synergizing Reasoning and Acting in Language Models," in *Proc. 11th Int. Conf. Learning Representations (ICLR)*, Kigali, Rwanda, May 2023. [Online]. Available: https://openreview.net/forum?id=WE_vluYUL-X

[38] S. Yan, J. Gu, Y. Zhu, and X. Ling, "Corrective Retrieval Augmented Generation," *arXiv preprint arXiv:2401.15884*, Jan. 2024. [Online]. Available: https://arxiv.org/abs/2401.15884

[39] Z. Shao, Y. Gong, Y. Shen, M. Huang, N. Duan, and W. Chen, "Enhancing Retrieval-Augmented Large Language Models with Iterative Retrieval-Generation Synergy," in *Proc. 2023 Conf. Findings Assoc. Comput. Linguistics: EMNLP 2023*, Singapore, Dec. 2023, pp. 9248–9274, doi: 10.18653/v1/2023.findings-emnlp.620.

[40] A. Raudaschl, "RAG-Fusion: A New Take on Retrieval Augmented Generation," 2023. [Online]. Available: https://arxiv.org/abs/2402.03367

---

## Cross-Reference Map

The following table shows which ThemisDB source files and documentation are grounded in each reference category:

| Category | Key References | ThemisDB Files |
| --- | --- | --- |
| Core RAG | [1]–[4] | `src/rag/`, `include/rag/` |
| Sparse retrieval (BM25) | [5] | `include/search/neural_sparse_retrieval.h` |
| Dense retrieval (DPR, SBERT) | [6], [7] | `benchmarks/bench_rag_hybrid_retriever.cpp` |
| Hybrid retrieval | [8]–[10] | `benchmarks/bench_rag_hybrid_retriever.cpp` |
| Re-ranking | [11]–[13] | `tests/test_rag_reranker.cpp` |
| Faithfulness | [14]–[16] | `include/rag/rag_judge.h`, `src/rag/rag_judge.cpp` |
| Knowledge gap detection | [17]–[21] | `include/rag/`, `src/rag/`, `examples/rag_knowledge_gap_integration.cpp` |
| Uncertainty (self-consistency) | [22] | `include/rag/rag_judge.h` |
| LLM-as-Judge | [23]–[26] | `src/rag/rag_judge.cpp`, `docs/RAG_JUDGE_LLM_INTEGRATION.md` |
| Multi-modal RAG | [27]–[28] | `include/rag/multimodal_rag.h`, `src/rag/multimodal_rag.cpp` |
| Bias mitigation | [29]–[32] | `plugins/ethics_ai/rag_context_engine` |
| Evaluation metrics | [33]–[36] | `tests/test_rag_evaluation_report_exporter.cpp` |
| Agentic RAG | [37]–[40] | `include/rag/agentic_rag.h`, `src/rag/agentic_rag.cpp` |

---

## Usage in ThemisDB Documentation

| Document | Referenced Sections |
| --- | --- |
| `docs/de/llm/RAG_BIBLIOGRAPHY.md` | German IEEE bibliography (overlapping subset) |
| `docs/de/llm/RAG_KNOWLEDGE_GAP_DETECTOR_ANALYSE.md` | [20], [21], [3], [15] |
| `docs/de/llm/RAG_LLM_AS_JUDGE_ANALYSE.md` | [23], [24], [16], [26] |
| `docs/de/llm/RAG_ETHICS_INTEGRATION_ANALYSIS.md` | [31], [32], [26] |
| `docs/de/llm/RAG_CROSS_SYSTEM_ANALYSIS.md` | [1]–[26] |
| `docs/en/rag/CONTINUOUS_LEARNING.md` | [21], [22], [37] |
| `docs/RAG_JUDGE_LLM_INTEGRATION.md` | [23]–[25] |

---

## Revision History

| Date | Version | Changes |
| --- | --- | --- |
| 2026-03-03 | 1.0 | Initial document — 40 IEEE references covering all required topic areas |

---

*Created: 2026-03-03*
*Version: 1.0*
*Citation Format: IEEE*
*Author: ThemisDB Development Team*
