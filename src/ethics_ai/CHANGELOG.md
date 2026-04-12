<!-- Status: current | validated: 2026-04-09 -->
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

## [0.0.2] — 2026-04-08

### Added

- **Unit tests for all six core components** (11 + 18 + 28 + 13 = 70 tests):
  - `tests/test_discourse_engine.cpp` (11 tests, `DiscourseEngineFocusedTests`):
    `initializeDebate` and `makeDecision` flows in standalone mode, including
    unknown-school error path, unique debate IDs, argument storage, and multi-school
    consensus check.
  - `tests/test_argument_store_standalone.cpp` (18 tests, `ArgumentStoreStandaloneTests`):
    Full CRUD coverage for `EthicalArgument`, `EthicalDecision`, `DebateSession`, and
    `PhilosophyProfile` in in-memory standalone mode (no RocksDB required).
  - `tests/test_ethics_ai_plugin.cpp` (28 tests, `EthicsAiPluginTests`):
    Plugin lifecycle (`initialize` / `shutdown`), config handling (JSON and default
    paths), metrics retrieval, and all `IThemisPlugin` contract methods.
  - `tests/test_rag_context_engine.cpp` (13 tests, `RAGContextEngineTests`):
    `buildContext`, `findSimilarDilemmas`, `getBestPractices`, `vectorSemanticSearch`,
    and `traverseArgumentChain` with both null and in-memory `QueryEngine`.
  - All four targets registered in `tests/CMakeLists.txt` under
    `THEMIS_PLUGIN_ETHICS_AI` guard.

- **`PhilosophyLoader::addProfile(const PhilosophyProfile&)`**
  (`philosophy_loader.h`): programmatic profile injection for unit testing without
  touching the filesystem; profiles added via this method are treated identically to
  YAML-loaded profiles.

### Fixed

- **`ArgumentStore::storePhilosophyProfile`**: profile lookup and key derivation now
  use `profile.school_id` (was incorrectly using `profile.school`), aligning with the
  `EthicsBaseEntityAdapter::makeProfileKey` contract.

### Changed

- `ethics_evaluator.h`, `philosophy_loader.h`: added `#include <variant>` to satisfy
  headers that include these files without pulling in the variant header transitively.
- `argument_store.cpp`: added `#include <set>` required by the updated duplicate-check
  logic.
- `cmake/CMakeLists.txt`, `cmake/features/PluginFeatures.cmake`: ethics-AI plugin
  build now correctly propagates include paths and compile definitions to all four new
  focused test targets.

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
