<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Ethics AI Module

All notable changes to this module are documented here.  
Format: [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), newest first.

---

## [Unreleased]

- LLM-based argument content generation (replacing template strings)
- Real embedding model integration for vector semantic search
- Dynamic confidence and consensus scoring
- Full integration tests for the decision pipeline
- Performance benchmarks for RAG context assembly

---

## [0.0.1] — 2026-03-22

### Added

- **`EthicsEvaluator`** (`ethics_evaluator.h/.cpp`):
  Scores ethical decisions across five independent dimensions: Decision Quality,
  Consistency, Fairness, Alignment, and Transparency. Returns a structured
  `EthicsEvaluationResult` with per-dimension scores and an overall aggregate.

- **`EthicalDiscourseEngine`** (`discourse_engine.h/.cpp`):
  Orchestrates multi-philosophy debates. Exposes `initializeDebate()` to create
  debate sessions and `makeDecision()` to generate a synthesised `EthicalDecision`
  across a set of philosophy schools. Integrates `PhilosophyLoader`,
  `ArgumentStore`, and `RAGContextEngine` via constructor injection.

- **`RAGContextEngine`** (`rag_context_engine.h/.cpp`):
  Implements seven AQL query patterns for contextual retrieval: textual similarity
  search, philosophy-specific argument retrieval, best-practice synthesis, vector
  semantic search, argument chain traversal, temporal filtering, and multi-philosophy
  consensus. Methods: `buildContext()`, `findSimilarDilemmas()`, `getBestPractices()`,
  `vectorSemanticSearch()`, `traverseArgumentChain()`.

- **`ArgumentStore`** (`argument_store.h/.cpp`):
  Persists `EthicalArgument`, `EthicalDecision`, `DebateSession`, and
  `PhilosophyProfile` entities as ThemisDB BaseEntity documents under the
  `entity:ethics_*:{id}` key namespace. Falls back to in-memory storage for
  standalone/testing mode when no `RocksDBWrapper` is supplied.

- **`PhilosophyLoader`** (`philosophy_loader.h/.cpp`):
  Loads and caches philosophy profiles from YAML files. Supports single-file and
  directory loading. Provides `hasProfile()`, `getProfile()`, `getSchoolIds()`,
  `getAllProfiles()`, and `clear()`. Validates required YAML fields on load.

- **`EthicsAiPlugin`** (`ethics_ai_plugin.cpp`):
  `IThemisPlugin` entry point. Wires `PhilosophyLoader`, `ArgumentStore`,
  `RAGContextEngine`, and `EthicalDiscourseEngine` on `initialize()`. Supports
  JSON config for `philosophy_dir` path.

- **Shared types** (`include/plugins/ethics_ai/ethics_ai_types.h` + `ethics_ai_types.cpp`):
  `EthicalArgument`, `EthicalDecision`, `PhilosophyProfile`, `DebateInitialization`,
  `RAGContext`, `EthicsEvaluationResult`, `ArgumentType` (`PRO`/`CON`/`NEUTRAL`),
  `ArgumentStrength` (`WEAK`/`MODERATE`/`STRONG`).

- **AQL query constants** (`ethics_aql_queries.h`):
  Named AQL string constants for all seven RAG retrieval patterns, used by
  `RAGContextEngine` to query the ThemisDB argument collections.

- **BaseEntity adapter** (`ethics_base_entity_adapter.h`):
  Maps ethics domain types to/from ThemisDB `BaseEntity` for unified storage
  and retrieval.
