# Tree of Thoughts: Deliberate Problem Solving with Large Language Models

**Metadaten:**
- Author(en): Shunyu Yao, Dian Yu, Jeffrey Zhao, Izhak Shafran, Tom Griffiths, Yuan Cao, Karthik Narasimhan
- Konferenz/Journal: NeurIPS 2023 (Spotlight)
- Jahr: 2023
- Link: [arXiv:2305.10601](https://arxiv.org/abs/2305.10601) · [GitHub: princeton-nlp/tree-of-thought-llm](https://github.com/princeton-nlp/tree-of-thought-llm)
- Zitierweise: `yao2023tot`
- Tags: `tree-of-thoughts`, `deliberate-reasoning`, `beam-search`, `BFS`, `DFS`, `thought-evaluation`, `multi-path-reasoning`, `ToT`, `llm`
- ThemisDB-Versionen: v2.0.0+ (implemented in `src/prompt_engineering/tree_of_thoughts.cpp`)
- Status: [x] Fully Implemented

## 📋 Executive Summary

Tree of Thoughts (ToT) generalizes Chain-of-Thought prompting to a **tree search over intermediate reasoning steps**: instead of one linear reasoning path, ToT explores a tree of thoughts (partial solutions), scores each node with an evaluator, and applies BFS, DFS, or Beam Search to navigate the tree. This enables deliberate, look-ahead, and backtracking reasoning that CoT cannot express. ThemisDB implements ToT as `TreeOfThoughtsBuilder` with all three search strategies, a pluggable `IToTThoughtGenerator`/`IToTEvaluator` interface, depth-bounded pruning, and answer synthesis from leaf nodes.

## 🎯 Key Findings

- **Thoughts as tree nodes**: Each node is a partial reasoning path; "thoughts" are typically 1–3 sentences. The tree explores different reasoning strategies in parallel.
- **Three search strategies**: BFS (layer-by-layer optimal for narrow trees), DFS with backtracking (sequential decision chains), Beam Search (top-B nodes per layer — quality–compute trade-off).
- **Evaluator-driven pruning**: A separate evaluator prompt scores each thought as *sure/maybe/impossible*; pruning before expansion prevents wasted LLM calls on dead-end paths.
- **Dramatic improvement on hard tasks**: Game of 24 (arithmetic puzzle): GPT-4 direct = 4%, CoT = 11%, ToT = **74%**. Creative Writing coherence: +27% vs. CoT.
- **Composable with Self-Refine**: ToT for exploration, Self-Refine/Reflexion for refining the best path found.
- **No weight updates**: ToT is a pure prompting technique; only forward-pass LLM inference is required.

## 🔗 Direct Influence on ThemisDB

### Affected Modules

- [x] Prompt Engineering → `src/prompt_engineering/tree_of_thoughts.cpp` (`TreeOfThoughtsBuilder`, `HeuristicThoughtGenerator`, BFS/DFS/BEAM)
- [x] Prompt Engineering → `src/prompt_engineering/chain_of_thought.cpp` (`ChainOfThoughtBuilder`: CoT is the linear degenerate case of ToT with branching_factor=1)
- [x] Prompt Engineering → `src/prompt_engineering/prompt_engineering_integration.cpp` (optional ToT activation via `IntegrationConfig::enable_tot`)
- [x] Prompt Engineering → `src/prompt_engineering/context_window_manager.cpp` (`ContextWindowBudgetManager` limits path length to token budget)

### What Was Adopted?

1. **Thought generation**: `IToTThoughtGenerator::generate(problem, path, k)` produces k new thoughts from the current reasoning path, mapping directly to the paper's thought generator.
2. **Thought evaluation + pruning**: `IToTEvaluator::evaluate(thought, problem, path)` returns a 0.0–1.0 score; nodes below `pruning_threshold` (default 0.3) are discarded before expansion.
3. **BFS implementation**: Layer-by-layer expansion with evaluator-based pruning after each layer; terminates at `max_depth` or when no node exceeds the threshold.
4. **DFS with backtracking**: Explicit stack-based traversal; backtracks when the evaluator rejects a node; depth limit enforced.
5. **Beam search**: Top-`beam_width` nodes retained per layer (default 4); per-layer expansion and scoring follow the paper's beam strategy.
6. **Answer synthesis**: `ToTResult` supports `BEST_LEAF` (highest-score leaf), `MAJORITY_VOTE` (closed answer sets), and `WEIGHTED` ensemble (open-ended answers).

### How Was It Adapted?

| ToT Concept | ThemisDB Adaptation | Rationale |
|---|---|---|
| GPT-4 as thought generator | `IToTThoughtGenerator` interface + `HeuristicThoughtGenerator` fallback | LLM-agnostic; heuristic fallback for edge/offline deployments |
| GPT-4 as evaluator | `IToTEvaluator` interface with score-based pruning | Flexible: `PromptEvaluator`, cosine-similarity, or LLM-based evaluator |
| Game of 24, creative writing benchmarks | AQL query decomposition, multi-step legal reasoning | ThemisDB domains: complex administrative law questions, multi-step DB queries |
| Flat evaluation prompt | `IToTEvaluator::evaluate()` receives full reasoning path as context | Full path context improves evaluation accuracy for ThemisDB's chained reasoning |
| Global problem context | `ToTConfig` + `PromptManager` integration | Thought-generation templates loaded from `PromptManager`; reuse existing template infrastructure |

### Performance Impact

| Metric | Paper Claim | ThemisDB Target | Delta | Reason |
|--------|-------------|-----------------|-------|--------|
| Game of 24 success rate (GPT-4) | CoT 11% → ToT 74% | +20–50 pp on complex AQL reasoning tasks | n/a | Domain differs; relative gain expected to be similar |
| Creative writing coherence | +27% vs. CoT | +10% on administrative-law explanations | -17 pp | Narrower output space reduces absolute CoT gap |
| Search latency (no LLM, BEAM B=4, depth=3) | n/a | < 2 ms P99 (pure traversal) | n/a | In-process heuristic; no I/O |
| Token overhead vs. CoT | branching_factor × max_depth × inference calls | configurable; opt-in only | n/a | Standard inference path uses CoT; ToT is opt-in |

## ⚠️ Limitations & Open Questions

- ToT multiplies LLM call overhead by `branching_factor × max_depth` generations plus `n_nodes` evaluations.
  - ThemisDB solution: ToT is opt-in via `IntegrationConfig::enable_tot`; standard inference uses CoT. Micro-caching of `(problem, path)` → thought pairs reduces redundant calls.
- Evaluator quality is critical: heuristic fallback (`HeuristicThoughtGenerator`) leads to suboptimal pruning.
  - Open: Production evaluator based on `PromptEvaluator::score()` + domain LoRA fine-tuned for ThemisDB query tasks.
- Context window limits maximum path depth, since the full path is embedded in each generation prompt.
  - ThemisDB solution: `ContextWindowBudgetManager` limits path embedding to the token budget; older path nodes are compressed with `PromptCompressor`.

## 🔬 Validation

- [x] Code reviewed against paper algorithm (BFS, DFS, Beam Search)
- [x] Unit tests written (30 tests, AC-01..AC-30 in `tests/test_tree_of_thoughts.cpp`)
- [ ] Benchmark executed (ToT vs. CoT on ThemisDB multi-step AQL reasoning tasks)
- [x] Documentation updated (`src/prompt_engineering/ROADMAP.md` Phase 6)
- [ ] Module README linked with paper reference
- [x] implementation_influence index updated

## 📚 Related Work

- [Self-Refine / Reflexion — Madaan & Shinn (2023)](madaan_self_refine_2023.md) — ToT for exploration, Self-Refine for refinement of the best path
- [ProTeGi — Pryzant et al. (2023)](pryzant_protegi_prompt_optimization_2023.md) — ProTeGi-optimized prompts feed the ToT thought generator
- [Khattab et al. (2023) — DSPy](khattab_dspy_2023.md) — DSPy pipeline layer can integrate ToT as a reasoning module
- [Wei et al. (2022) — Chain-of-Thought](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#42-chain-of-thought-cot-prompting) — CoT is the linear degenerate case of ToT
- [Best Practice: LLM Prompt Enhancement Pipeline](../best_practices/llm_prompt_enhancement_pipeline.md)
- [`src/prompt_engineering/tree_of_thoughts.cpp`](../../../src/prompt_engineering/tree_of_thoughts.cpp)

---
**Last Updated:** 2026-04-27
**Next Review:** 2026-10-31
