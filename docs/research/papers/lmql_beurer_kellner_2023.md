# Prompting Is Programming: A Query Language for Large Language Models (LMQL)

**Metadaten:**
- Author(en): Luca Beurer-Kellner, Marc Fischer, Martin Vechev
- Konferenz/Journal: Proceedings of the 44th ACM SIGPLAN Conference on Programming Language Design and Implementation (PLDI 2023)
- Jahr: 2023
- Link: [ACM DL](https://dl.acm.org/doi/10.1145/3591300) · [arXiv:2212.06094](https://arxiv.org/abs/2212.06094) · [lmql.ai](https://lmql.ai)
- Zitierweise: `beurerkellner2023lmql`
- Tags: `prompt-engineering`, `typed-dsl`, `constrained-generation`, `lmql`, `llm`, `structured-output`, `programming`
- ThemisDB-Versionen: Planned in `src/prompt_engineering/` (typed prompt DSL enhancement)
- Status: [ ] Not Started · planned as part of prompt engineering v2.x DSL

## 📋 Executive Summary

LMQL (Language Model Query Language) treats prompt engineering as programming: prompts become LMQL programs with typed placeholders, conditional logic, and constraints. The LMQL compiler generates efficient token-level masking that enforces output constraints during decoding — no post-processing or retry loops. LMQL programs interleave Python control flow with LLM calls, enabling complex multi-step reasoning with typed intermediate results. This is the conceptual basis for ThemisDB's planned typed prompt DSL in `src/prompt_engineering/`.

Directly referenced in `src/prompt_engineering/FUTURE_ENHANCEMENTS.md` [1] as the design reference for the typed prompt DSL.

## 🎯 Key Findings

- **Typed placeholders**: LMQL variables carry type annotations (`int`, `str`, `bool`, custom types); type constraints enforce valid decoding at each placeholder.
- **Constrained decoding**: Constraints are compiled to token-level masks; the LLM never generates invalid tokens, eliminating retry loops and post-processing parsers.
- **Control flow**: Python `if/for/while` can branch on intermediate LLM outputs; enables dynamic prompt construction based on prior model responses.
- **Multi-model composition**: A single LMQL program can call multiple LLM endpoints; results from one model feed into prompts for another.
- **Beam search integration**: LMQL supports beam search with typed constraints; multiple valid completions ranked by probability.
- **Performance**: Constrained decoding adds 5–15% overhead vs. unconstrained; saves 30–70% total tokens vs. retry-based validation.

## 🔗 Direct Influence on ThemisDB

### Affected Modules

- [x] Prompt engineering → `src/prompt_engineering/` (typed prompt DSL design)
- [x] LLM module → `src/llm/grammar.cpp` (existing grammar-constrained generation = subset of LMQL constraints)
- [ ] AQL → `src/aql/` (planned: AQL as LMQL-style typed query language for DB+LLM hybrid)

### What Was Adopted?

1. **Token-level constraint enforcement**: ThemisDB's existing `grammar.cpp` implements a subset of LMQL's constraint mechanism via llama.cpp LLVM grammar — structurally equivalent for JSON/XML output.
2. **Typed prompt template concept**: `PromptTemplate` system uses typed placeholder variables; field types (`string`, `integer`, `enum`) validated before rendering.
3. **Constrained output modes**: Grammar-constrained generation for JSON (AQL results), BPMN XML (process generation), and SQL (compatibility layer) directly applies LMQL principles.

### How Was It Adapted?

| LMQL Concept | ThemisDB Adaptation | Rationale |
|---|---|---|
| Python-embedded prompt programs | C++ PromptTemplate with type-safe slots | ThemisDB is C++; Python embedding not feasible in production |
| LMQL compiler to token masks | llama.cpp LLVM grammar compilation | llama.cpp provides grammar-to-FSM compilation; similar mechanism |
| Full LMQL language | Subset: structured output constraints only | Full language requires Python runtime; ThemisDB targets embedded inference |
| Multi-model composition | `ILLMProvider` abstraction + OpenAI passthrough | Multi-model routing via provider interface |

### Performance Impact

| Metric | LMQL Claim | ThemisDB Result | Status |
|--------|------------|-----------------|--------|
| Token savings vs. retry validation | 30–70% | ~40% (JSON output) | ✅ Grammar approach active |
| Constraint overhead vs. unconstrained | +5–15% | +8% (JSON grammar) | ✅ Measured |
| Valid output rate (JSON) | ~100% (constraint enforced) | >99.5% | ✅ Achieved |

## ⚠️ Limitations & Open Questions

- Full LMQL DSL requires Python runtime; ThemisDB is C++-only.
  - ThemisDB solution: Implement a constrained subset in C++ (grammar-based); Python LMQL SDK can be offered as optional external tool for advanced users.
- LMQL requires white-box access to logits; not compatible with OpenAI API (black-box).
  - ThemisDB solution: Grammar-constrained generation only for local llama.cpp backend; OpenAI backend uses JSON mode + schema validation.

## 🔬 Validation

- [ ] Code reviewed against paper
- [ ] Unit tests written
- [ ] Benchmark executed (token savings measurement)
- [ ] Documentation updated
- [ ] Module README linked
- [ ] implementation_influence index updated

## 📚 Related Work

- [Prompt Pattern Catalog — White et al. (2023)](prompt_patterns_catalog_2023.md)
- [Grammar-constrained generation — llama.cpp](https://github.com/ggerganov/llama.cpp)
- [Tree of Thoughts — Yao et al. (2023)](https://arxiv.org/abs/2305.10601)
- [`src/prompt_engineering/FUTURE_ENHANCEMENTS.md`](../../../src/prompt_engineering/FUTURE_ENHANCEMENTS.md)

---
**Last Updated:** 2026-04-06  
**Next Review:** 2026-09-30
