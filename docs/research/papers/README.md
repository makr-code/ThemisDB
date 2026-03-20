# Scientific Papers Index

This directory contains documentation of all scientific papers that have influenced ThemisDB's implementation.

## Purpose

Each paper that serves as a foundation for a ThemisDB algorithm, data structure, or design decision should be documented here using the [template](_template_paper.md).

## Index

| Paper | Module(s) | Version | Status |
|-------|-----------|---------|--------|
| [Attention Is All You Need — Vaswani et al. (2017)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#2-transformer-architecture--foundation-models) | `src/llm/` | v1.3.0+ | ✅ Implemented |
| [BERT — Devlin et al. (2019)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#3-embeddings--semantic-representations) | `src/vector/`, `src/rag/` | v1.0.0+ | ✅ Implemented |
| [Sentence-BERT — Reimers & Gurevych (2019)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#32-sentence-embeddings-sbert) | `src/vector/`, `src/rag/` | v1.0.0+ | ✅ Implemented |
| [GPT-3 Few-Shot Learning — Brown et al. (2020)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#41-few-shot-in-context-learning) | `src/prompt_engineering/` | v1.2.0+ | ✅ Implemented |
| [Chain-of-Thought Prompting — Wei et al. (2022)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#42-chain-of-thought-cot-prompting) | `src/prompt_engineering/` | v1.2.0+ | ✅ Implemented |
| [LoRA — Hu et al. (2022)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#71-low-rank-adaptation-lora) | `src/llm/lora/` | v1.3.0+ | ✅ Implemented |
| [QLoRA — Dettmers et al. (2023)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#72-qlora) | `src/llm/lora/` | v1.3.0+ | ✅ Implemented |
| [FlashAttention — Dao et al. (2022)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#83-flash-attention) | `src/llm/` (CUDA) | v1.4.0-alpha+ | ✅ Implemented |
| [Speculative Decoding — Chen et al. (2023)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#84-speculative-decoding) | `src/llm/` | v1.4.0-alpha+ | ✅ Implemented |
| [PagedAttention — Kwon et al. (2023)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#85-pagedattention--continuous-batching) | `src/llm/` | v1.4.0-alpha+ | ✅ Implemented |
| [RAG — Lewis et al. (2020)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#61-original-rag-framework) | `src/rag/` | v1.2.0+ | ✅ Implemented |
| [RAGAS — Es et al. (2023)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#63-rag-evaluation-ragas) | `src/llm/monitoring/` | v1.3.0+ | ✅ Implemented |
| [APE — Zhou et al. (2022)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#51-automatic-prompt-engineer-ape) | `src/prompt_engineering/` | v1.4.0-alpha+ | ✅ Implemented |
| [Matryoshka Representation Learning — Kusupati et al. (2022)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#34-matryoshka-representation-learning) | `src/vector/` | v1.4.1+ | ⏳ Planned |
| [Scaling Laws — Kaplan et al. (2020)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#22-scaling-laws) | `src/llm/` (model selection) | v1.3.0+ | ✅ Implemented |
| [Graph-Process Schema and Hybrid Vector+Graph Retrieval — ThemisDB (2026)](process_graph_vector_ai_2026.md) | `src/graph/`, `src/vector/`, `src/rag/`, `src/llm/` | v1.9.0+ | 🔄 Partially Implemented |

> For full IEEE citations and implementation details, see [LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md).

## Adding a New Paper

1. Copy [_template_paper.md](_template_paper.md) to a new file named `<short_key>_<year>.md`  
   Example: `hnsw_efficient_ann_2018.md`
2. Fill in all required fields (see [TEMPLATES.md](TEMPLATES.md))
3. Link the paper in the relevant module README under *Wissenschaftliche Grundlagen & Einflüsse*
4. Register it in [implementation_influence/README.md](../implementation_influence/README.md)

## Naming Convention

```
<topic>_<year>.md
```

Examples:
- `hnsw_efficient_ann_2018.md`
- `lsm_tree_rocksdb_2016.md`
- `raft_consensus_2014.md`

## See Also

- [TEMPLATES.md](TEMPLATES.md) — required fields and formatting rules
- [_template_paper.md](_template_paper.md) — copy-paste starter template
- [../RESEARCH_GUIDE.md](../RESEARCH_GUIDE.md) — end-to-end contributor workflow
- [../implementation_influence/README.md](../implementation_influence/README.md) — master cross-reference index
