# ReAct: Synergizing Reasoning and Acting in Language Models

**Metadaten:**
- Author(en): Shunyu Yao, Jeffrey Zhao, Dian Yu, Nan Du, Izhak Shafran, Karthik Narasimhan, Yuan Cao
- Konferenz/Journal: ICLR 2023
- Jahr: 2022/2023
- Link: [arXiv:2210.03629](https://arxiv.org/abs/2210.03629) · [ICLR 2023 OpenReview](https://openreview.net/forum?id=WE_vluYUL-X)
- Zitierweise: `yao2022react`
- Tags: `react`, `reasoning-acting`, `tool-use`, `agentic-rag`, `iterative-retrieval`, `chain-of-thought`, `action-observation`, `thought-action-observation`, `multi-hop`, `llm`
- ThemisDB-Versionen: v1.8.0+ (implemented in `src/rag/agentic_rag.cpp`)
- Status: [x] Fully Implemented

## 📋 Executive Summary

ReAct (Reasoning + Acting) introduces the **Thought-Action-Observation** (TAO) loop: the LLM generates an explicit reasoning trace ("Thought"), executes a tool action ("Action"), and incorporates the result ("Observation") before the next reasoning step. This interleaving of reasoning and acting enables multi-hop retrieval, self-correction, and tool-use in a single prompt-driven loop — without any weight updates. ReAct achieves +34% absolute improvement on HotpotQA (multi-hop reasoning) and +20% on WebShop (web navigation) over CoT-only baselines. ThemisDB implements ReAct as `AgenticRAG` with configurable `max_iterations`, a pluggable tool registry (`RetrievalFn`, `ToolFn`), an explicit `AgentTrace` for observability, and `KnowledgeGapDetector`-driven termination.

## 🎯 Key Findings

- **Thought-Action-Observation loop**: Interleaving reasoning ("I need to find X because…") with tool calls (search, lookup, calculate) and observations (tool results) enables multi-hop reasoning that neither CoT nor pure action-based approaches achieve alone.
- **Multi-hop QA**: ReAct+CoT achieves 39.6% on HotpotQA vs. 29.4% CoT-only — +34% relative improvement.
- **Grounding**: Observations from tool calls ground the LLM's reasoning in retrieved facts, reducing hallucination by ~24% on knowledge-intensive tasks.
- **Convergence**: ReAct loops typically converge in 3–5 iterations on multi-hop tasks; a hard max-iterations cap prevents infinite loops.
- **ReAct + Reflexion**: Combining ReAct with Reflexion-style episodic memory achieves +45% on AlfWorld decision-making tasks.
- **No weight updates**: ReAct is a pure prompting + tool-use technique; works with any black-box LLM.

## 🔗 Direct Influence on ThemisDB

### Affected Modules

- [x] RAG → `src/rag/agentic_rag.cpp` (`AgenticRAG`: full TAO loop; `AgentTrace`; tool registry; `KnowledgeGapDetector`-driven termination)
- [x] RAG → `src/rag/multi_step_rag.cpp` (`MultiStepRAG`: multi-step retrieval strategy using ReAct-style iterative queries)
- [x] RAG → `src/rag/knowledge_gap_detector.cpp` (`KnowledgeGapDetector`: three-level gap detection — drives ReAct loop iteration decision)
- [x] RAG → `src/rag/knowledge_graph_retriever.cpp` (`KnowledgeGraphRetriever`: `retrieve(entities)` — the graph-traversal tool action in the ReAct loop)

### What Was Adopted?

1. **Thought-Action-Observation (TAO) loop**: `AgenticRAG::run()` iterates `{generate_thought → select_action → execute_tool → record_observation}` up to `max_iterations` — a direct implementation of the paper's loop structure.
2. **Tool registry**: `AgenticRAG` accepts `RetrievalFn` (dense/BM25/graph retrieval) and `ToolFn` (calculator, date lookup, entity resolver) as callable objects — the paper's "action space."
3. **Explicit reasoning trace**: `AgentTrace` stores the full sequence of `{thought, action, observation}` triples per iteration — reproducing the paper's trace format for observability and debugging.
4. **Convergence / termination**: Loop terminates on `(accumulated_docs.size() >= max_docs) || (gap_detector.gapLevel() == NO_GAP) || (iteration >= max_iterations)` — combining the paper's implicit convergence with ThemisDB's explicit `KnowledgeGapDetector` signal.
5. **Deduplication across iterations**: `mergeDocuments()` tracks `seen_ids` and skips already-retrieved documents across iterations — ensuring the loop doesn't re-fetch the same content.
6. **Final answer synthesis**: After loop termination, `AgenticRAG::run()` assembles all accumulated documents and returns a `AgenticResult` with the full trace — enabling downstream explanation and citation generation.

### How Was It Adapted?

| ReAct Concept | ThemisDB Adaptation | Rationale |
|---|---|---|
| LLM generates Thought (natural language) | `AgenticRAG` supports heuristic thought-generation fallback | Offline deployments without LLM; gap detector provides implicit thought |
| Action space: Search / Lookup / Finish | `RetrievalFn` + `ToolFn` + termination condition | ThemisDB-specific action space: dense retrieval, graph traversal, entity linking |
| Observation: raw tool output | `AgentTrace::observations` with structured `RetrievedDocument` list | Structured observations enable downstream `CitationHighlighter` and `ExplainabilityReasonBuilder` |
| Single-step termination ("Finish") | `KnowledgeGapDetector::gapLevel()` multi-level signal | Three gap levels (HIGH/MEDIUM/NONE) allow progressive resource allocation |
| No loop memory | `AgentTrace` + `ContinuousLearningOrchestrator` Loop 1 feedback | Traces feed `LearningMetrics` for Bayesian retrieval parameter optimisation |
| Python `langchain` / `ReAct` harness | Native C++ `AgenticRAG` | No Python dependency; integrates directly with `HybridRetriever` + `KnowledgeGraphRetriever` |

### Performance Impact

| Metric | Paper Claim | ThemisDB Target | Delta | Reason |
|--------|-------------|-----------------|-------|--------|
| Multi-hop QA quality | +34% vs. CoT-only (HotpotQA) | +15–25% on multi-hop AQL/admin queries | -9–19 pp | ThemisDB domain is narrower; DB query decomposition differs from general QA |
| Hallucination reduction | ~24% vs. CoT-only | +10% faithfulness on RAGJudge THOROUGH | n/a | Direct comparison pending benchmark |
| Convergence iterations | 3–5 typical | max_iterations default = 5 | 0 | Matches paper observation |
| Loop overhead (no LLM, no retrieval) | n/a | < 1 ms P99 per iteration | n/a | Pure in-memory loop; retrieval and LLM are the bottleneck |
| Max docs accumulated | n/a | configurable (default 20) | n/a | Context window budget via `ContextWindowBudgetManager` |

## ⚠️ Limitations & Open Questions

- ReAct requires a capable LLM for reasoning traces; heuristic-only loops lack the adaptive reasoning of the full TAO approach.
  - ThemisDB solution: `KnowledgeGapDetector` provides an LLM-independent gap signal; `AgenticRAG` functions without LLM reasoning traces in offline mode.
- The action space must be explicitly defined; actions outside the registered tools silently fail.
  - ThemisDB solution: `AgenticRAG` validates all tool names at construction time; unknown actions are logged and skip to the next iteration.
- Context window grows with each Observation; long agentic traces exceed context window limits.
  - ThemisDB solution: `ContextWindowBudgetManager` compresses earlier observations when the window is full; `PromptCompressor` with SUMMARIZE strategy applied to the trace.
- No empirical benchmark of ThemisDB's `AgenticRAG` vs. standard single-pass `HybridRetriever` on ThemisDB domain queries.
  - Open: Build a multi-hop benchmark on ThemisDB administrative law question pairs (Target: Q3 2026).

## 🔬 Validation

- [x] Code reviewed against ReAct paper TAO loop algorithm
- [x] Tool registry, `AgentTrace`, deduplication implemented in `AgenticRAG`
- [x] `KnowledgeGapDetector` termination condition implemented
- [x] Unit tests written (`tests/test_agentic_rag.cpp`)
- [ ] Benchmark executed (AgenticRAG vs. single-pass HybridRetriever on ThemisDB multi-hop queries)
- [x] Documentation updated (`src/rag/ROADMAP.md` Agentic RAG section)
- [ ] Module README linked with paper reference
- [x] implementation_influence index updated

## 📚 Related Work

- [Tree of Thoughts — Yao et al. (2023)](yao_tree_of_thoughts_2023.md) — ToT expands the thought space; ReAct grounds each thought in tool observations — composable
- [Self-Refine + Reflexion — Madaan & Shinn (2023)](madaan_self_refine_2023.md) — Reflexion adds episodic memory to ReAct; ReAct+Reflexion achieves +45% on AlfWorld
- [Best Practice: LLM-as-Judge RAG Evaluation](../best_practices/llm_as_judge_rag_evaluation.md) — Judge evaluates AgenticRAG result quality
- [Speculative RAG — Wang et al. (2024)](wang_speculative_rag_2024.md) — Speculative RAG is a performance-oriented complement to AgenticRAG's quality-oriented iterative approach
- [`src/rag/agentic_rag.cpp`](../../../src/rag/agentic_rag.cpp)
- [`src/rag/multi_step_rag.cpp`](../../../src/rag/multi_step_rag.cpp)
- [`src/rag/knowledge_gap_detector.cpp`](../../../src/rag/knowledge_gap_detector.cpp)

---
**Last Updated:** 2026-04-27
**Next Review:** 2026-10-31
