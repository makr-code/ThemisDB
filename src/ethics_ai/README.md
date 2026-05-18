> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release --target <target>`

<!-- Status: current | validated: 2026-05-12 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# ThemisDB Ethics AI Module

**Version:** 0.3.x
**Status:** 🟡 Beta
**Last Updated:** 2026-05-12
**Module Path:** `src/ethics_ai/`
**Public Header Path:** `include/ethics_ai/`
**Namespace:** `themis::plugins::ethics`

---

## Module Purpose

The Ethics AI module provides a multi-philosophy decision pipeline for ThemisDB. It combines
profile-driven argument generation, argument persistence, context retrieval, and decision
synthesis into a plugin-compatible runtime component.

Core outcomes:
- initialise ethical debates over one or more philosophy schools,
- generate/store/retrieve argument and decision artifacts,
- enrich decisions with RAG context from stored argument data,
- evaluate decisions with a 5-dimension scoring model,
- expose runtime metrics for observability.

---

## Entry Points and Main Components

### Public API Entry Points (`include/ethics_ai/`)

| Header | Primary Contract |
|--------|------------------|
| `ethics_ai/ethics_ai_plugin_interface.h` | `IEthicsAIPlugin` lifecycle + decision/storage/retrieval/evaluation/config APIs |
| `ethics_ai/ethics_ai_types.h` | Shared enums/structs (`EthicalArgument`, `EthicalDecision`, `PhilosophyProfile`, `RAGContext`, `Status`, etc.) |
| `ethics_ai/*.h` (router/compressor/validator/memory helpers) | Optional strategy and budget components used by extended debate workflows |

### Source Components (`src/ethics_ai/`)

| File | Class / Role |
|------|--------------|
| `ethics_ai_plugin.cpp` | `EthicsAIPlugin` implementation of `IEthicsAIPlugin`, wiring loader/store/RAG/evaluator |
| `discourse_engine.cpp` | `EthicalDiscourseEngine` orchestration for debate init, rounds, and synthesis |
| `argument_store.cpp` | `ArgumentStore` persistence of arguments, decisions, profiles, and chains |
| `philosophy_loader.cpp` | `PhilosophyLoader` profile loading and YAML validation |
| `rag_context_engine.cpp` | `RAGContextEngine` context retrieval patterns |
| `ethics_evaluator.cpp` | `EthicsEvaluator` scoring and metrics helpers |
| `chain_visualizer.cpp` | DOT/Mermaid export for argument chain visualisation |
| `*_router/*.cpp`-style helpers | Selection/compression/convergence/tournament support in multi-school debates |

---

## Runtime Behavior

1. Plugin initialization creates and wires loader, store, RAG engine, discourse engine, and evaluator.
2. Optional `philosophy_dir` configuration triggers profile loading during initialization.
3. Operational calls are fail-fast when uninitialized (`Status::Error("Plugin not initialized")`).
4. Debate and decision methods update internal metrics counters.
5. Metrics are exposed as Prometheus text and dashboard JSON snapshots.

---

## Configuration

| Key | Type | Default | Effect |
|-----|------|---------|--------|
| `philosophy_dir` | string | unset | Loads philosophy profiles during `initialize()` if present |

Other keys can be stored via `setConfig()` / `getConfig()`, but only consumed keys affect runtime behavior.

---

## Installation

This module is built as part of ThemisDB. Build via the project presets and include
the module through the plugin system/runtime used in your environment.

---

## Error Cases and Limits

### Common Error Cases

- Plugin lifecycle misuse (API called before successful `initialize()`).
- Unknown or missing philosophy profile IDs.
- Invalid profile path / YAML parse errors.
- Missing argument/decision/chain IDs in store lookups.
- Empty school sets for debate workflows.

### Current Limits

- LLM-backed argument generation is not fully enabled by default module flow.
- Real embedding-backed semantic retrieval remains a planned enhancement.
- Runtime quality depends on the loaded profile quality/coverage.

For planned completion work, see roadmap and enhancement docs linked below.

---

## Usage Snippet

```cpp
#include "ethics_ai/ethics_ai_plugin_interface.h"

using themis::plugins::ethics::IEthicsAIPlugin;
using themis::plugins::ethics::EthicalDecision;

IEthicsAIPlugin* ethics = /* injected via plugin manager */ nullptr;

if (ethics && ethics->initialize(R"({"philosophy_dir":"plugins/ethics_ai/philosophies"})")) {
    auto result = ethics->makeDecision(
        "Should a medical triage AI prioritize maximum survival probability?",
        {"utilitarianism", "kantian_ethics", "care_ethics"},
        "bioethics",
        true
    );

    if (std::holds_alternative<EthicalDecision>(result)) {
        const auto& decision = std::get<EthicalDecision>(result);
        // decision.decision_text / decision.confidence / decision.consensus_level
    }
}
```

---

## Troubleshooting

- **`Plugin not initialized`**: verify successful `initialize()` call and configuration parsing.
- **Profile loading errors**: validate `philosophy_dir` path and YAML syntax/content.
- **`Philosophy profile not found`**: align request school IDs with loaded profiles.
- **Unexpectedly low consensus/confidence**: inspect school mix and argument strength distribution.
- **No useful RAG context**: ensure argument store has domain-relevant historical arguments.

---

## See Also

- Public headers overview: [`../../include/ethics_ai/README.md`](../../include/ethics_ai/README.md)
- Architecture: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- Security: [`SECURITY.md`](SECURITY.md)
- Roadmap: [`ROADMAP.md`](ROADMAP.md)
- Future enhancements: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)
- Module index (DE): [`../../docs/de/ethics_ai/README.md`](../../docs/de/ethics_ai/README.md)
- Primary sources (DE): [`../../docs/de/ethics_ai/PRIMARY_SOURCES.md`](../../docs/de/ethics_ai/PRIMARY_SOURCES.md)
- Primary sources (EN): [`../../docs/en/ethics_ai/PRIMARY_SOURCES.md`](../../docs/en/ethics_ai/PRIMARY_SOURCES.md)
