> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/ethics_ai/ARCHITECTURE.md -->

# Ethics AI Module — Public Header Architecture

**Module Path:** `include/ethics_ai/`  
**Implementation:** `../../src/ethics_ai/`  
**Canonical architecture doc:** [`../../src/ethics_ai/ARCHITECTURE.md`](../../src/ethics_ai/ARCHITECTURE.md)

---

## 1. Overview

`include/ethics_ai/` defines the **public multi-school ethical reasoning, argument synthesis, discourse memory, profile registry, and LLM cascade routing API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/ethics_ai/ARCHITECTURE.md`](../../src/ethics_ai/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Core Ethics Types and Registry

| Header | Public Type | Purpose |
|--------|------------|---------|
| `ethics_ai_types.h` | `EthicsAITypes` | Common type definitions for ethical reasoning |
| `ethics_profile_registry.h` | `EthicsProfileRegistry` | School-of-thought profile registry |
| `ethics_ai_plugin_interface.h` | `IEthicsAIPlugin` | Plugin interface for ethics modules |
### 2.2 Reasoning Engines

| Header | Public Type | Purpose |
|--------|------------|---------|
| `ethics_selection_router.h` | `EthicsSelectionRouter` | Route queries to applicable ethical frameworks |
| `convergence_marker_engine.h` | `ConvergenceMarkerEngine` | Cross-school convergence detection |
| `cross_school_tension_resolver.h` | `CrossSchoolTensionResolver` | Resolve conflicts between ethical frameworks |
| `position_abstract_validator.h` | `PositionAbstractValidator` | Validate abstract ethical positions |
### 2.3 Synthesis and Discourse

| Header | Public Type | Purpose |
|--------|------------|---------|
| `synthesis_matrix_builder.h` | `SynthesisMatrixBuilder` | Build position-comparison matrices |
| `discourse_memory_store.h` | `DiscourseMemoryStore` | Persistent multi-turn discourse memory |
| `prior_round_compressor.h` | `PriorRoundCompressor` | Compress prior reasoning rounds for context |
### 2.4 Tournament and Cascade

| Header | Public Type | Purpose |
|--------|------------|---------|
| `tournament_mode_selector.h` | `TournamentModeSelector` | Tournament-style framework selection |
| `llm_cascade_router.h` | `LLMCascadeRouter` | Cascade routing across LLM providers |

---

## 3. Namespace Layout

All public types reside in the `themis::ethics_ai` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/ethics_ai/` expose the **stable public API**; internal types live in `src/ethics_ai/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **LLM**.
