<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# ThemisDB Ethics AI Module

**Version:** 0.0.1
**Status:** 🔴 Alpha (v0.0.1)
**Last Updated:** 2026-03-22
**Module Path:** `src/ethics_ai/`
**Namespace:** `themis::plugins::ethics`

---

## Module Purpose

The Ethics AI module implements a multi-philosophy ethical discourse engine for ThemisDB.
It orchestrates structured ethical debates across multiple philosophical frameworks,
generates pro/con arguments per philosophy, synthesises decisions, and enriches the
discourse with Retrieval-Augmented Generation (RAG) context drawn from the ThemisDB
argument store.

The module is designed as a ThemisDB plugin (`IThemisPlugin`). It exposes a
decision-making API that callers invoke with a dilemma description, a set of philosophy
schools, and a category. The engine loads the relevant philosophy profiles from YAML
files, generates arguments from each profile, optionally queries the RAG context engine
for similar past dilemmas, and synthesises a final ethical decision with confidence and
consensus scores.

---

## Component Table

| File | Class / Role |
|------|-------------|
| `ethics_evaluator.h` / `.cpp` | `EthicsEvaluator` — scores decisions across 5 dimensions |
| `discourse_engine.h` / `.cpp` | `EthicalDiscourseEngine` — orchestrates multi-philosophy debates |
| `rag_context_engine.h` / `.cpp` | `RAGContextEngine` — 7 AQL query patterns for contextual retrieval |
| `argument_store.h` / `.cpp` | `ArgumentStore` — ThemisDB BaseEntity storage for arguments/decisions |
| `philosophy_loader.h` / `.cpp` | `PhilosophyLoader` — loads/validates philosophy profiles from YAML |
| `ethics_ai_plugin.cpp` | `EthicsAiPlugin` — IThemisPlugin entry point, wires all components |
| `ethics_ai_types.h` | Shared types: `EthicalArgument`, `EthicalDecision`, `PhilosophyProfile`, `RAGContext`, `ArgumentType`, `ArgumentStrength` |
| `ethics_aql_queries.h` | AQL query string constants for all 7 RAG retrieval patterns |
| `ethics_base_entity_adapter.h` | Adapter mapping ethics domain types to ThemisDB BaseEntity |
| `ethics_ai_types.cpp` | Type implementations and helpers |
| `argument_store.cpp` | `ArgumentStore` implementation |
| `philosophy_loader.cpp` | YAML parsing and profile validation |
| `rag_context_engine.cpp` | AQL execution for 7 retrieval patterns |

---

## Quick-Start Example

```cpp
#include "ethics_ai_plugin.h"

// 1. Instantiate the plugin
EthicsAiPlugin plugin;
plugin.initialize(R"({"philosophy_dir": "config/philosophies/"})");

// 2. Obtain the discourse engine
auto* engine = static_cast<EthicalDiscourseEngine*>(plugin.getInstance());

// 3. Run a multi-philosophy decision
auto result = engine->makeDecision(
    "Should AI systems be allowed to make binding legal decisions?",
    {"utilitarianism", "kantian_ethics", "virtue_ethics"},
    "governance",
    /* use_rag = */ true
);

if (auto* decision = std::get_if<EthicalDecision>(&result)) {
    std::cout << decision->decision_text << "\n";
    std::cout << "Confidence: " << decision->confidence << "\n";
    std::cout << "Consensus:  " << decision->consensus_level << "\n";
}
```

---

## Evaluation Dimensions

`EthicsEvaluator` scores each decision on five independent dimensions:

| Dimension | Description |
|-----------|-------------|
| Decision Quality | Relevance and completeness of the decision relative to the dilemma |
| Consistency | Internal logical consistency of the supporting arguments |
| Fairness | Equal treatment of perspectives; absence of systematic bias |
| Alignment | Alignment with the stated philosophy and its main theses |
| Transparency | Explainability and traceability of the decision path |

---

## RAG Query Patterns

`RAGContextEngine` implements seven AQL-based retrieval patterns:

| # | Pattern | Method |
|---|---------|--------|
| 1 | Textual similarity search | `findSimilarDilemmas()` |
| 2 | Philosophy-specific argument retrieval | `getArgumentsByPhilosophy()` |
| 3 | Best-practice synthesis | `getBestPractices()` |
| 4 | Vector semantic search | `vectorSemanticSearch()` |
| 5 | Argument chain traversal | `traverseArgumentChain()` |
| 6 | Temporal filtering | (built into `buildContext`) |
| 7 | Multi-philosophy consensus | (built into `buildContext`) |

---

## Storage Layout

```
entity:ethics_arguments:{id}   – EthicalArgument as BaseEntity
entity:ethics_decisions:{id}   – EthicalDecision as BaseEntity
entity:ethics_debates:{id}     – Debate session as BaseEntity
entity:ethics_profiles:{id}    – PhilosophyProfile as BaseEntity
```

---

## See Also

- `ARCHITECTURE.md` — component diagram and design principles
- `ROADMAP.md` — implementation phases and production checklist
- `SECURITY.md` — threat model and security controls
- `FUTURE_ENHANCEMENTS.md` — planned features

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.

## Usage

The implementation files in this module are compiled into the ThemisDB library.
See [`../../include/ethics_ai/README.md`](../../include/ethics_ai/README.md) for the public API.
