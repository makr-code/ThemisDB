# ThemisDB RAG Module Implementation

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
