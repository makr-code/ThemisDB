> **Build:** `cmake --preset linux-release && cmake --build build/linux-release --target themis_core`

# Ethics AI Module — Public Headers

**Module Path:** `include/ethics_ai/`
**Implementation Overview:** `../../src/ethics_ai/README.md`

## Purpose

Public C++ APIs and shared data contracts for the Ethics AI module. The headers define
plugin-facing operations for ethical debate orchestration, decision synthesis, RAG context
integration, profile handling, and structured debate artifacts used by the source module in
`src/ethics_ai/`.

## Header Entry-Points

| Header | Primary API | Runtime Role |
|--------|-------------|--------------|
| `ethics_ai_plugin_interface.h` | `IEthicsAIPlugin` | Main plugin contract (`initializeDebate`, `makeDecision`, storage, metrics, config) |
| `ethics_ai_types.h` | Domain structs/enums (`EthicalArgument`, `EthicalDecision`, `PhilosophyProfile`, `RAGContext`, `Status`) | Shared data model across plugin, store, RAG, and debate flow |
| `ethics_profile_registry.h` | `EthicsProfileRegistry` | Lazy-load profile index + cache for many ethics schools |
| `ethics_selection_router.h` | `IEthicsSelectionRouter`, `EthicsSelectionRouter` | Multi-stage school pre-selection for large profile sets |
| `prior_round_compressor.h` | `PriorRoundCompressor` | Context compression of previous rounds |
| `cross_school_tension_resolver.h` | `CrossSchoolTensionResolver` | Selection of high-signal opposing schools/arguments |
| `convergence_marker_engine.h` | `ConvergenceMarkerEngine` | Convergence marker extraction and preamble generation |
| `synthesis_matrix_builder.h` | `SynthesisMatrixBuilder` | Compact matrix view for final synthesis stage |
| `llm_cascade_router.h` | `ILlmCascadeRouter` | Model routing per discourse stage/token budget |
| `tournament_mode_selector.h` | `TournamentModeSelector` | Tournament-style opponent injection support |
| `position_abstract_validator.h` | `PositionAbstractValidator` | Schema checks for round position abstracts |
| `discourse_memory_store.h` | `DiscourseMemoryStore` | Episodic discourse memory storage/retrieval |

## Public API Behavior

### Lifecycle and Access

- `IEthicsAIPlugin::initialize()` must be called before operational API calls.
- Most operational methods return `Status::Error("Plugin not initialized")` when used before initialization.
- `shutdown()` releases owned components and resets runtime state.

### Error Handling Contract

- Public APIs consistently return `std::variant<..., Status>` (or `Status`) rather than throwing.
- Typical errors surfaced from underlying components include:
  - unknown/missing philosophy profile
  - empty or invalid IDs in persistence operations
  - missing debate/decision/chain entities
  - profile directory/path or YAML parsing failures

### Runtime Characteristics

- Metrics updates are guarded by internal mutexes in the plugin implementation.
- Debate, decision, and argument flows are composed from `PhilosophyLoader`, `ArgumentStore`, `RAGContextEngine`, and `EthicsEvaluator` in `src/ethics_ai/ethics_ai_plugin.cpp`.
- Prometheus text export and dashboard JSON export are available through the interface.

## Configuration and Limits

### Supported Runtime Configuration Keys

| Key | Type | Effect |
|-----|------|--------|
| `philosophy_dir` | string | Optional profile directory loaded during plugin initialization |

Additional key/value pairs can be stored via `setConfig()` and retrieved via `getConfig()`, but only keys consumed by source components influence behavior.

### Known Limits

- Full LLM-backed argument generation and real embedding integration are still roadmap/future-enhancement topics.
- Behavior without initialized storage/profile inputs is intentionally fail-fast via `Status::Error`.

## Installation

The headers are part of the repository include tree. Ensure your target includes
the project `include/` directory (or the exported ThemisDB include path) and links
against the ThemisDB plugin/runtime components used by your deployment setup.

## Usage

```cpp
#include "ethics_ai/ethics_ai_plugin_interface.h"

using themis::plugins::ethics::IEthicsAIPlugin;
using themis::plugins::ethics::EthicalDecision;

// In production this instance is typically obtained from ThemisDB's plugin manager.
IEthicsAIPlugin* plugin = /* injected by plugin manager */ nullptr;

if (plugin && plugin->initialize(R"({"philosophy_dir":"plugins/ethics_ai/philosophies"})")) {
    auto decisionOrStatus = plugin->makeDecision(
        "Should an autonomous system prioritize safety over task completion?",
        {"utilitarianism", "kantian_ethics", "virtue_ethics"},
        "autonomous_systems",
        true
    );

    if (std::holds_alternative<EthicalDecision>(decisionOrStatus)) {
        const auto& decision = std::get<EthicalDecision>(decisionOrStatus);
        // consume decision.decision_text / decision.confidence / decision.consensus_level
    }
}
```

## Troubleshooting

- `Plugin not initialized`: call `initialize()` successfully before using debate/storage/query APIs.
- `Philosophy profile not found`: ensure profile IDs used in requests exist in loaded profile set.
- YAML/Directory errors in profile loading: validate `philosophy_dir` path and profile syntax.
- Empty retrieval results: verify arguments/decisions are stored and filters (`school`, `type`, `limit`) are not too strict.

## Related Docs

- Implementation overview: [`../../src/ethics_ai/README.md`](../../src/ethics_ai/README.md)
- Architecture: [`../../src/ethics_ai/ARCHITECTURE.md`](../../src/ethics_ai/ARCHITECTURE.md)
- Security: [`../../src/ethics_ai/SECURITY.md`](../../src/ethics_ai/SECURITY.md)
- Roadmap: [`../../src/ethics_ai/ROADMAP.md`](../../src/ethics_ai/ROADMAP.md)
- Future enhancements: [`../../src/ethics_ai/FUTURE_ENHANCEMENTS.md`](../../src/ethics_ai/FUTURE_ENHANCEMENTS.md)
- Module index (DE): [`../../docs/de/ethics_ai/README.md`](../../docs/de/ethics_ai/README.md)
- Primary sources (DE): [`../../docs/de/ethics_ai/PRIMARY_SOURCES.md`](../../docs/de/ethics_ai/PRIMARY_SOURCES.md)
- Primary sources (EN): [`../../docs/en/ethics_ai/PRIMARY_SOURCES.md`](../../docs/en/ethics_ai/PRIMARY_SOURCES.md)
