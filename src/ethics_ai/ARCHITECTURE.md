<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Ethics AI Module — Architecture Guide

**Version:** 0.0.1
**Last Updated:** 2026-04-06
**Module Path:** `src/ethics_ai/`

---

## 1. Overview

The Ethics AI module is a self-contained ThemisDB plugin that provides structured
ethical reasoning over arbitrary dilemmas. It loads philosophy profiles from YAML,
generates per-philosophy arguments, applies RAG context from past decisions stored in
ThemisDB, and synthesises scored decisions. All persistent state uses ThemisDB's native
BaseEntity layer and RocksDB backend — the module adds no external storage dependency.

---

## 2. Design Principles

- **Philosophy as configuration** — philosophy profiles are YAML files loaded at
  startup by `PhilosophyLoader`; adding a new school requires no code changes.
- **RAG-first enrichment** — `RAGContextEngine` queries the ThemisDB argument store
  via AQL before synthesis, ensuring decisions improve over time as more cases
  accumulate.
- **BaseEntity storage** — all arguments, decisions, and profiles are persisted as
  standard ThemisDB BaseEntity documents, making them queryable via AQL without
  special extensions.
- **Standalone mode** — `ArgumentStore` falls back to in-memory storage when no
  `RocksDBWrapper` is provided, enabling unit-testing without a live database.
- **Variant error handling** — all public APIs return `std::variant<T, Status>`;
  no exceptions are used in the business logic paths.

---

## 3. Component Architecture

### 3.1 Component Diagram

```
┌───────────────────────────────────────────────────────────────┐
│                    EthicsAiPlugin                             │
│             (IThemisPlugin entry point)                       │
│   initialize() ─ wires all sub-components                     │
└───────────────────────────┬───────────────────────────────────┘
                            │
              ┌─────────────▼──────────────┐
              │   EthicalDiscourseEngine   │
              │   (discourse_engine.cpp)   │
              │                            │
              │  initializeDebate()        │
              │  makeDecision()            │
              └──┬──────────┬─────────────┘
                 │          │
    ┌────────────▼─┐   ┌────▼─────────────────┐
    │ Philosophy   │   │   RAGContextEngine    │
    │ Loader       │   │   (rag_context_       │
    │ (YAML files) │   │    engine.cpp)        │
    │              │   │                       │
    │ getProfile() │   │ buildContext()        │
    │ getSchoolIds()│  │ findSimilarDilemmas() │
    └──────┬───────┘   │ vectorSemanticSearch()│
           │           └────────┬──────────────┘
           │                    │
           └──────────┬─────────┘
                      │
              ┌───────▼────────────┐
              │   ArgumentStore    │
              │ (argument_store.   │
              │  cpp)              │
              │                    │
              │ storeArgument()    │
              │ storeDecision()    │
              │ getArgument()      │
              └───────┬────────────┘
                      │
              ┌───────▼────────────┐
              │  RocksDBWrapper /  │
              │  QueryEngine (AQL) │
              │  (ThemisDB core)   │
              └────────────────────┘

              ┌────────────────────┐
              │  EthicsEvaluator   │
              │ (ethics_evaluator. │
              │  cpp)              │
              │                    │
              │ evaluateDecision() │
              │  5 dimension funcs │
              └────────────────────┘
```

### 3.2 Component Table

| Component | File(s) | Responsibility |
|-----------|---------|----------------|
| `EthicsAiPlugin` | `ethics_ai_plugin.cpp` | Plugin lifecycle, component wiring |
| `EthicalDiscourseEngine` | `discourse_engine.h/.cpp` | Debate orchestration, decision synthesis |
| `EthicsEvaluator` | `ethics_evaluator.h/.cpp` | 5-dimension decision scoring |
| `RAGContextEngine` | `rag_context_engine.h/.cpp` | 7 AQL retrieval patterns, embedding search |
| `ArgumentStore` | `argument_store.h/.cpp` | Persist/retrieve arguments, decisions, profiles |
| `PhilosophyLoader` | `philosophy_loader.h/.cpp` | Load/cache/validate YAML philosophy profiles |

---

## 4. Data Flow

### 4.1 `makeDecision()` Flow

```
Caller → EthicalDiscourseEngine::makeDecision(dilemma, schools, category, use_rag)
  │
  ├─ Validate: all schools present in PhilosophyLoader
  │
  ├─ [if use_rag] RAGContextEngine::buildContext(dilemma, schools, category)
  │     ├─ findSimilarDilemmas()  [AQL pattern 1]
  │     ├─ getArgumentsByPhilosophy() [AQL pattern 2]
  │     └─ vectorSemanticSearch() [AQL pattern 4]
  │
  ├─ For each school:
  │     PhilosophyLoader::getProfile(school)
  │     → generateArgument(profile, dilemma, PRO)
  │     → ArgumentStore::storeArgument(arg, store_vector=true)
  │
  ├─ synthesizeDecision(arguments, primary_philosophy)
  │
  ├─ Construct EthicalDecision {id, text, confidence, consensus_level}
  │
  └─ ArgumentStore::storeDecision(decision)
       └─ RocksDB: entity:ethics_decisions:{id}
```

### 4.2 Evaluation Flow

```
EthicsEvaluator::evaluateDecision(decision, arguments)
  ├─ evaluateDecisionQuality()  → score ∈ [0, 1]
  ├─ evaluateConsistency()      → score ∈ [0, 1]
  ├─ evaluateFairness()         → score ∈ [0, 1]
  ├─ evaluateAlignment()        → score ∈ [0, 1]
  └─ evaluateTransparency()     → score ∈ [0, 1]
       → EthicsEvaluationResult { scores[], overall }
```

---

## 5. Storage Schema

| Key Pattern | Value Type | Description |
|-------------|-----------|-------------|
| `entity:ethics_arguments:{id}` | BaseEntity (JSON) | Ethical argument |
| `entity:ethics_decisions:{id}` | BaseEntity (JSON) | Ethical decision |
| `entity:ethics_debates:{id}` | BaseEntity (JSON) | Debate session metadata |
| `entity:ethics_profiles:{id}` | BaseEntity (JSON) | Philosophy profile |

AQL Collections (logical view over RocksDB):

- `ethics_arguments`
- `ethics_decisions`
- `ethics_debates`
- `ethics_profiles`

---

## 6. Integration Points

| Direction | Module | Interface |
|-----------|--------|-----------|
| **Implements** | `plugins/ethics_ai_interface.h` | `IThemisPlugin` |
| **Uses** | `storage/rocksdb_wrapper.h` | Persistent argument store |
| **Uses** | `query/query_engine.h` | AQL execution for RAG patterns |
| **Uses** | `storage/base_entity.h` | BaseEntity serialisation |

---

## 7. Threading & Concurrency

- `PhilosophyLoader` profiles are loaded once at startup; thereafter read-only.
- `ArgumentStore` guards all writes with `std::mutex mutex_`.
- `EthicalDiscourseEngine` is stateless per call; safe for concurrent invocations
  provided the injected `ArgumentStore` is the shared instance.
- `RAGContextEngine` delegates to `ArgumentStore`; thread safety is inherited.

---

## 8. Error Handling

| Scenario | Behaviour |
|----------|-----------|
| Unknown philosophy school | `Status::Error("Philosophy profile not found: X")` |
| Empty philosophy school list | `Status::Error("At least one philosophy school required")` |
| RocksDB write failure | `Status::Error` propagated from `ArgumentStore::storeArgument` |
| AQL query failure | `Status::Error` propagated from `RAGContextEngine::buildContext` |
| YAML parse error | `Status::Error` from `PhilosophyLoader::loadFromFile` |
| No `RocksDBWrapper` supplied | Standalone in-memory mode activated |

---

## 9. Known Limitations

- Argument content generation uses template strings; LLM-based generation is planned
  for v0.1.0 (see `FUTURE_ENHANCEMENTS.md`).
- Vector embeddings for arguments are placeholder stubs; a real embedding model is
  required for production semantic search.
- `confidence` and `consensus_level` are static placeholders (0.75 / 0.70); a proper
  scoring model is planned for v0.2.0.

---

## 10. References

- `src/ethics_ai/README.md` — module overview
- `ARCHITECTURE.md` (root) — full system architecture
- `include/plugins/ethics_ai/ethics_ai_types.h` — canonical type definitions
