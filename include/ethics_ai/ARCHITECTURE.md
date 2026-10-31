---
Author: ThemisDB Team
Created: 2026-06-01
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

Exact model backend not verifiable from final document text alone.

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | header inventory reviewed: 2026-10-31 -->
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
| `ethics_ai_types.h` | `EthicalArgument`, `EthicalDecision`, `MetaVerdict`, and related types | Common ethical reasoning and LDM data types |
| `ethics_profile_registry.h` | `IEthicsProfileRegistry`, `EthicsProfileRegistry` | Profile index and lookup |
| `ethics_ai_plugin_interface.h` | `IEthicsAIPlugin` | Plugin interface |
### 2.2 Reasoning Engines

| Header | Public Type | Purpose |
|--------|------------|---------|
| `ethics_selection_router.h` | `EthicsSelectionRouter` | Route queries to applicable ethical frameworks |
| `convergence_marker_engine.h` | `ConvergenceMarkerEngine` | Cross-school convergence detection |
| `cross_school_tension_resolver.h` | `CrossSchoolTensionResolver` | Resolve conflicts between ethical frameworks |
| `position_abstract_validator.h` | `PositionAbstractValidator` | Validate abstract ethical positions |
| `discourse_orchestrator.h` | `DiscourseOrchestrator` | LDM Ebene-1/2 helper API |
| `meta_verdict_builder.h` | `MetaVerdictBuilder` | Assemble a verdict from supplied inputs |
| `mirror_school_handler.h` | `MirrorSchoolHandler` | Mirror-school helper API |
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
| `llm_cascade_router.h` | `ILlmCascadeRouter`, `LlmCascadeRouter` | Cascade routing helper |

---

## 3. Namespace Layout

Public types use the `themis::plugins::ethics` namespace (or a nested namespace), as declared in the headers.

---

## 4. Contract Notes

- Headers in `include/ethics_ai/` expose public interfaces; “public” describes path/visibility and does not guarantee ABI stability.
- Clients should use declarations in these headers; consult `VERSIONING.md` for compatibility policy.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **LLM**.
