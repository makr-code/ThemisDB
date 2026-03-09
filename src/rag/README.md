# ThemisDB RAG Module Implementation

**Last Updated:** March 2026

## Module Purpose

Implements the Retrieval-Augmented Generation pipeline for ThemisDB, combining vector similarity search, LLM integration, hybrid retrieval, multi-judge evaluation, hallucination detection, citation highlighting, and continuous learning.

## Subsystem Scope

**In scope:** Vector retrieval from ThemisDB index, LLM integration for answer generation, context window management, hybrid search (vector + BM25), re-ranking, faithfulness/relevance/completeness/coherence evaluation, bias detection, knowledge-graph augmented retrieval, agentic/multi-modal RAG, continuous learning orchestration, A/B testing, and per-query evaluation reporting.

**Out of scope:** LLM model management (handled by llm module), full-text index construction (handled by search module), embedding generation (handled by LLM module).

## Relevant Interfaces

- `rag_judge.cpp` — main orchestrator for multi-dimensional evaluation
- `llm_integration.cpp` — LLM connector for RAG
- `hybrid_retriever.cpp` — BM25 + vector fusion with RRF weighting
- `streaming_retriever.cpp` — incremental context window filling
- `continuous_learning_orchestrator.cpp` — adaptive learning from evaluation feedback

## Current Delivery Status

**Maturity:** 🟢 Production Ready — Full RAG pipeline with evaluation, hallucination detection, hybrid retrieval, re-ranking, citation highlighting, knowledge-graph retrieval, and continuous learning operational.

## Overview

Implementation files for ThemisDB's Retrieval-Augmented Generation (RAG) system providing intelligent document retrieval, quality evaluation, knowledge gap detection, ethical compliance checking, and adaptive continuous learning.

## Implementation Files (41 files, ~17,400 LOC)

### Core Components
1. **rag_judge.cpp** - Main orchestrator for multi-dimensional evaluation
2. **knowledge_gap_detector.cpp** - Three-level gap detection system
3. **llm_integration.cpp** - Bridge to LLM inference engine

### Streaming Retrieval
4. **streaming_retriever.cpp** - Incremental context window filling with token-budget
   enforcement, relevance-ordered streaming, MMR deduplication, and cancellation support

### Hybrid & Graph Retrieval
5. **hybrid_retriever.cpp** - BM25 + vector fusion with configurable RRF weights
6. **knowledge_graph_retriever.cpp** - Knowledge graph-augmented retrieval with entity linking
7. **reranker.cpp** - Cross-encoder re-ranking with heuristic scorer and ONNX stub

### Evaluators
8. **faithfulness_evaluator.cpp** - Fact-checking against sources
9. **relevance_evaluator.cpp** - Query-answer alignment
10. **completeness_evaluator.cpp** - Query aspect coverage
11. **coherence_evaluator.cpp** - Structure and readability
12. **bias_detector.cpp** - Ethical compliance checking
13. **cot_evaluator.cpp** - Chain-of-thought evaluation
14. **geval_evaluator.cpp** - G-Eval framework (Liu et al., 2023)
15. **rubric_evaluator.cpp** - Custom rubric evaluation

### Support Components
16. **claim_extractor.cpp** - Extract atomic claims from answers
17. **nli_faithfulness_verifier.cpp** - NLI entailment-based claim verification
18. **response_parser.cpp** - Parse LLM evaluation responses
19. **prompt_templates.cpp** - Template and few-shot management
20. **judge_config.cpp** - Configuration validation
21. **onnx_model_loader.cpp** - ONNX runtime model loading for NLI/cross-encoder

### Advanced Evaluation
22. **judge_ensemble.cpp** - Multi-judge voting strategies
23. **pairwise_comparator.cpp** - Head-to-head comparisons
24. **llm_judge_integration.cpp** - Judge orchestration
25. **llm_judge_client.cpp** - HTTP client for remote LLM judge endpoints
26. **llm_meta_analyzer.cpp** - Performance meta-analysis

### Document Processing
27. **document_splitter.cpp** - Configurable chunk size, overlap, and strategy
28. **document_summarizer.cpp** - Multi-document summarization before context injection
29. **citation_highlighter.cpp** - Map answer sentences to source chunks

### Reporting & Monitoring
30. **evaluation_report_exporter.cpp** - Per-query evaluation report export (JSON / HTML)
31. **hallucination_dashboard.cpp** - Rolling-window hallucination rate tracking
32. **http_metrics_client.cpp** - HTTP metrics export

### Continuous Learning
33. **continuous_learning_orchestrator.cpp** - Adaptive learning from evaluation feedback with LoRA retraining
34. **continuous_learning_client.cpp** - Client interface for learning orchestrator
35. **bayesian_optimizer.cpp** - Bayesian optimization over retrieval parameters
36. **ab_testing_framework.cpp** - Traffic-splitting A/B testing with statistical validation
37. **learning_metrics.cpp** - Sliding-window metrics with mean/std-dev/trend export

### Quality Control
38. **quality_control_pipeline.cpp** - Automated quality control pipeline
39. **quality_control_factory.cpp** - Factory for quality control components

### Agentic & Multi-Modal
40. **agentic_rag.cpp** - Agentic RAG with iterative retrieval loops
41. **multimodal_rag.cpp** - Multi-modal RAG (image + text retrieval)

### Header-Only Components
- **batch_evaluator.h** - Batch evaluation pipeline (header-only)
- **calibration_manager.h** - Evaluator score calibration (header-only)
- **evaluation_cache.h** - Evaluation result caching (header-only)
- **rag_integration_helpers.h** - Integration helper utilities (header-only)

## Performance Characteristics

| Mode | Latency | Use Case |
|------|---------|----------|
| Fast | ~100ms | High-throughput production |
| Balanced | ~500ms | Standard RAG pipeline |
| Thorough | ~2s | Research, benchmarking |

## Testing

```bash
./build/tests/test_rag_judge
./build/tests/test_rag_judge_phase1
./build/tests/test_rag_judge_phase2
./build/tests/test_rag_judge_phase3
./build/tests/test_rag_judge_phase4
./build/tests/test_knowledge_gap_detector
./build/tests/test_rag_streaming_retriever
./build/tests/test_rag_hybrid_retriever
./build/tests/test_rag_reranker
./build/tests/test_rag_document_splitter
./build/tests/test_rag_document_summarizer
./build/tests/test_rag_citation_highlighter
./build/tests/test_rag_hallucination_dashboard
./build/tests/test_rag_evaluation_report_exporter
./build/tests/test_rag_agentic
./build/tests/test_rag_multimodal
./build/tests/test_rag_knowledge_graph_retriever
./build/tests/test_rag_ethics
./build/benchmarks/bench_rag_hybrid_retriever
./build/benchmarks/bench_rag_ethics
```

## Wissenschaftliche Grundlagen

Die Implementierung basiert auf folgenden peer-reviewten Forschungsarbeiten:

### Retrieval-Augmented Generation (Grundlagen)

1. **Lewis, P., Perez, E., Piktus, A., Petroni, F., Karpukhin, V., Goyal, N., … Kiela, D. (2020).**
   *Retrieval-Augmented Generation for Knowledge-Intensive NLP Tasks.*
   Advances in Neural Information Processing Systems (NeurIPS), 33, 9459–9474.
   arXiv: [2005.11401](https://arxiv.org/abs/2005.11401)
   > Grundlegendes RAG-Framework: Kombination von Dense-Retrieval (DPR) mit Seq2Seq-Generierung.
   > Direkte Grundlage für das Retrieval → Augmentation → Generation-Muster in `rag_judge.cpp` und `llm_integration.cpp`.

### Streaming- und Inkrementelles Retrieval

2. **Jiang, Z., Xu, F. F., Gao, L., Sun, Z., Liu, Q., Dwivedi-Yu, J., … Neubig, G. (2023).**
   *Active Retrieval Augmented Generation.*
   Proceedings of the 2023 Conference on Empirical Methods in Natural Language Processing (EMNLP), 7969–7992.
   arXiv: [2305.06983](https://arxiv.org/abs/2305.06983)
   > FLARE-Ansatz: Iteratives, vorausschauendes Retrieval statt einmaligem Batch-Abruf.
   > Motiviert das inkrementelle Füllen des Kontextfensters in `streaming_retriever.cpp`:
   > Dokumente werden schrittweise hinzugefügt, bis das Token-Budget erschöpft ist.

3. **Liu, N. F., Lin, K., Hewitt, J., Paranjape, A., Bevilacqua, M., Petroni, F., & Liang, P. (2023).**
   *Lost in the Middle: How Language Models Use Long Contexts.*
   Transactions of the Association for Computational Linguistics, 12, 157–173.
   arXiv: [2307.03172](https://arxiv.org/abs/2307.03172)
   > Zeigt, dass LLMs relevante Informationen am Anfang und Ende des Kontextfensters besser
   > verarbeiten als in der Mitte. Begründet die Relevanz-sortierte Reihenfolge
   > (`sort_by_relevance = true`) in `StreamingRetrieverConfig`: hochrelevante Dokumente werden
   > zuerst in das Kontextfenster geladen.

### Diversitätsbasierte Dokumentenauswahl (MMR)

4. **Carbonell, J., & Goldstein, J. (1998).**
   *The Use of MMR, Diversity-Based Reranking for Reordering Documents and Producing Summaries.*
   Proceedings of the 21st Annual International ACM SIGIR Conference on Research and Development
   in Information Retrieval (SIGIR), 335–336.
   DOI: [10.1145/290941.291025](https://doi.org/10.1145/290941.291025)
   > Ursprüngliche Formulierung von Maximal Marginal Relevance (MMR): Balance zwischen
   > Relevanz und Diversität bei der Dokumentauswahl. Direkte Grundlage für die
   > Jaccard-basierte MMR-Deduplizierung (`enable_mmr_deduplication`,
   > `mmr_similarity_threshold`) in `StreamingRetriever::Impl::isDuplicate()`.

### In-Context Retrieval und Token-Budget

5. **Ram, O., Levine, Y., Dalmedigos, I., Muhlgay, D., Shashua, A., Leyton-Brown, K., & Shoham, Y. (2023).**
   *In-Context Retrieval-Augmented Language Models.*
   Transactions of the Association for Computational Linguistics, 11, 1316–1331.
   arXiv: [2302.00083](https://arxiv.org/abs/2302.00083)
   > Untersucht die optimale Nutzung des Kontextfensters für RAG: wie viele Dokumente
   > eingebettet werden sollen und wie das Token-Budget aufgeteilt wird.
   > Begründet die `max_context_tokens`-Konfiguration und das Token-Schätzverfahren
   > (`estimateTokens()`) in `ContextWindowFiller`.

## See Also

- Headers: `../../include/rag/`
- Secondary docs (DE): `../../docs/de/rag/README.md`
- Examples: `../../examples/rag/`

---

*41 files | ~17,400 lines | MIT License*

## Scientific References

1. Lewis, P., Perez, E., Piktus, A., Petroni, F., Karpukhin, V., Goyal, N., … Kiela, D. (2020). **Retrieval-Augmented Generation for Knowledge-Intensive NLP Tasks**. *Advances in Neural Information Processing Systems (NeurIPS)*, 33, 9459–9474. https://arxiv.org/abs/2005.11401

2. Gao, Y., Xiong, Y., Gao, X., Jia, K., Pan, J., Bi, Y., … Wang, H. (2023). **Retrieval-Augmented Generation for Large Language Models: A Survey**. *arXiv preprint*. https://arxiv.org/abs/2312.10997

3. Karpukhin, V., Oğuz, B., Min, S., Lewis, P., Wu, L., Edunov, S., … Yih, W.-t. (2020). **Dense Passage Retrieval for Open-Domain Question Answering**. *Proceedings of EMNLP 2020*, 6769–6781. https://doi.org/10.18653/v1/2020.emnlp-main.550

4. Ma, X., Guo, J., Zhang, R., Fan, Y., Cheng, X., & Cheng, X. (2022). **Pre-train, Prompt, and Predict: A Systematic Survey of Prompting Methods in Natural Language Processing**. *ACM Computing Surveys*, 55(9), 195:1–195:35. https://doi.org/10.1145/3560815

5. Borgeaud, S., Mensch, A., Hoffmann, J., Cai, T., Rutherford, E., Millican, K., … Sifre, L. (2022). **Improving Language Models by Retrieving from Trillions of Tokens**. *Proceedings of the 39th International Conference on Machine Learning (ICML)*, 2206–2240. https://arxiv.org/abs/2112.04426
