# ThemisDB RAG Module Implementation

## Module Purpose

Implements the Retrieval-Augmented Generation pipeline for ThemisDB, combining vector similarity search, LLM inference, and hybrid retrieval to answer queries from stored documents.

## Subsystem Scope

**In scope:** Vector retrieval from ThemisDB index, LLM integration for answer generation, context window management, hybrid search (vector + BM25), re-ranking.

**Out of scope:** LLM model management (handled by llm module), full-text index construction (handled by search module), embedding generation (handled by LLM module).

## Relevant Interfaces

- `rag_pipeline.cpp` — orchestrates retrieval → augmentation → generation
- `llm_integration.cpp` — LLM connector for RAG
- `context_manager.cpp` — context window management
- `retriever.cpp` — vector and hybrid retrieval

## Current Delivery Status

**Maturity:** 🟡 Beta — Basic RAG pipeline with vector retrieval and LLM integration operational; hybrid search and re-ranking in progress.

## Overview

Implementation files for ThemisDB's Retrieval-Augmented Generation (RAG) system providing intelligent document retrieval, quality evaluation, knowledge gap detection, and ethical compliance checking.

## Implementation Files (19 files, ~7,600 LOC)

### Core Components
1. **rag_judge.cpp** - Main orchestrator for multi-dimensional evaluation
2. **knowledge_gap_detector.cpp** - Three-level gap detection system
3. **llm_integration.cpp** - Bridge to LLM inference engine

### Evaluators
4. **faithfulness_evaluator.cpp** - Fact-checking against sources
5. **relevance_evaluator.cpp** - Query-answer alignment
6. **completeness_evaluator.cpp** - Query aspect coverage
7. **coherence_evaluator.cpp** - Structure and readability
8. **bias_detector.cpp** - Ethical compliance checking

### Support Components
9. **claim_extractor.cpp** - Extract atomic claims from answers
10. **response_parser.cpp** - Parse LLM evaluation responses
11. **prompt_templates.cpp** - Template and few-shot management
12. **judge_config.cpp** - Configuration validation
13. **rubric_evaluator.cpp** - Custom rubric evaluation

### Advanced Components
14. **judge_ensemble.cpp** - Multi-judge voting strategies
15. **pairwise_comparator.cpp** - Head-to-head comparisons
16. **cot_evaluator.cpp** - Chain-of-thought evaluation
17. **geval_evaluator.cpp** - G-Eval framework (Liu et al., 2023)
18. **llm_judge_integration.cpp** - Judge orchestration
19. **llm_meta_analyzer.cpp** - Performance meta-analysis

## Performance Characteristics

| Mode | Latency | Use Case |
|------|---------|----------|
| Fast | ~100ms | High-throughput production |
| Balanced | ~500ms | Standard RAG pipeline |
| Thorough | ~2s | Research, benchmarking |

## Testing

```bash
./build/tests/test_rag_judge
./build/tests/test_knowledge_gap_detector
./build/tests/test_rag_pipeline_integration
./build/benchmarks/bench_rag_evaluation
```

## See Also

- Headers: `../../include/rag/README.md`
- Documentation: `../../docs/src/rag/`
- Examples: `../../examples/rag/`

---

*19 files | ~7,600 lines | MIT License*

## Scientific References

1. Lewis, P., Perez, E., Piktus, A., Petroni, F., Karpukhin, V., Goyal, N., … Kiela, D. (2020). **Retrieval-Augmented Generation for Knowledge-Intensive NLP Tasks**. *Advances in Neural Information Processing Systems (NeurIPS)*, 33, 9459–9474. https://arxiv.org/abs/2005.11401

2. Gao, Y., Xiong, Y., Gao, X., Jia, K., Pan, J., Bi, Y., … Wang, H. (2023). **Retrieval-Augmented Generation for Large Language Models: A Survey**. *arXiv preprint*. https://arxiv.org/abs/2312.10997

3. Karpukhin, V., Oğuz, B., Min, S., Lewis, P., Wu, L., Edunov, S., … Yih, W.-t. (2020). **Dense Passage Retrieval for Open-Domain Question Answering**. *Proceedings of EMNLP 2020*, 6769–6781. https://doi.org/10.18653/v1/2020.emnlp-main.550

4. Ma, X., Guo, J., Zhang, R., Fan, Y., Cheng, X., & Cheng, X. (2022). **Pre-train, Prompt, and Predict: A Systematic Survey of Prompting Methods in Natural Language Processing**. *ACM Computing Surveys*, 55(9), 195:1–195:35. https://doi.org/10.1145/3560815

5. Borgeaud, S., Mensch, A., Hoffmann, J., Cai, T., Rutherford, E., Millican, K., … Sifre, L. (2022). **Improving Language Models by Retrieving from Trillions of Tokens**. *Proceedings of the 39th International Conference on Machine Learning (ICML)*, 2206–2240. https://arxiv.org/abs/2112.04426
