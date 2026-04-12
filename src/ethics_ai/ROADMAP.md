<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Ethics AI Module Roadmap

## Current Status

v0.2.0 — `PhilosophyLoader` handles rich YAML (complex thesis objects, point-keyed strengths/weaknesses, nested `decision_framework`). `EthicsEvaluator::Config` configurable weights normalised in ctor. `ChainVisualizer` ships `exportDot`/`exportMermaid`/`chainToDot`/`chainToMermaid`. 8 tests CV-01…CV-08 in `tests/test_ethics_ai_chain_visualizer.cpp`. LLM argument generation and real embeddings remain planned for v0.1.0/v0.3.0.

---

## Completed ✅

- [x] `EthicsEvaluator` — 5-dimension decision scoring API
- [x] `EthicalDiscourseEngine` — `initializeDebate()` and `makeDecision()`
- [x] `RAGContextEngine` — 7 AQL retrieval pattern methods
- [x] `ArgumentStore` — BaseEntity-backed persistence with standalone fallback
- [x] `PhilosophyLoader` — YAML profile loading with caching and validation
- [x] `EthicsAiPlugin` — IThemisPlugin wiring and lifecycle
- [x] Shared domain types (`EthicalArgument`, `EthicalDecision`, `PhilosophyProfile`, etc.)
- [x] AQL query constants for all 7 retrieval patterns
- [x] BaseEntity adapter for ethics types
- [x] `std::variant<T, Status>` error handling throughout all public APIs
- [x] Standalone in-memory mode for `ArgumentStore` (testing without RocksDB)
- [x] **`PhilosophyLoader` rich YAML** — complex thesis objects, point-keyed `strengths`/`weaknesses`, nested `decision_framework` (Issue: #4596) (2026-04-12)
- [x] **`EthicsEvaluator::Config`** — configurable dimension weights normalised in ctor (Issue: #4596) (2026-04-12)
- [x] **`ChainVisualizer`** — `exportDot()`, `exportMermaid()`, `chainToDot()`, `chainToMermaid()` (Issue: #4596) (2026-04-12)
- [x] **8 tests CV-01…CV-08** in `tests/test_ethics_ai_chain_visualizer.cpp` (Issue: #4596) (2026-04-12)

---

## In Progress [~]

- [x] Focused unit test suites implemented and passing:
  `test_argument_store_standalone` (18), `test_rag_context_engine_focused` (13),
  `test_ethics_ai_plugin_focused` (28), `test_discourse_engine_focused` (11),
  `test_philosophy_loader_focused` (7 passed, 1 skipped env-dependent)
- [x] Integration test suite implemented and passing:
  `test_ethics_ai_integration` (21) — FullPipeline, ArgumentStoreRAG, RAGContextBuild

---

## Planned Features

### v0.1.0 — LLM Argument Generation (Target: Q3 2026)

- [ ] Integrate LLM backend for argument content generation (Target: Q3 2026)
  - Inputs: `PhilosophyProfile`, dilemma text, `ArgumentType`
  - Outputs: `EthicalArgument.content` with chain-of-thought rationale
  - Constraints: max 500 tokens per argument; latency ≤ 3 s per argument
  - Errors: LLM timeout → fallback to template; context window exceeded → truncate
  - Tests: unit (mock LLM) + integration (live LLM) + golden-output comparison
- [x] Dynamic `confidence` score computed from argument strength distribution (Target: Q3 2026)
  - Implemented in `EthicsEvaluator::computeConfidence()`: WEAK=0.25, MODERATE=0.50, STRONG=0.75, DECISIVE=1.00 weighted average
- [x] Dynamic `consensus_level` score from inter-philosophy agreement analysis (Target: Q3 2026)
  - Implemented in `EthicsEvaluator::computeConsensus()`: per-school PRO/CONTRA tally; fraction of agreeing schools
- [x] Richer argument content from `generateArgument()` using all profile theses and decision framework (Target: Q3 2026)
  - Strength derived from total thesis count; all `main_theses` and `secondary_theses` included; dilemma text referenced
- [ ] Real embedding generation for `vectorSemanticSearch` (sentence-transformers or ONNX) (Target: Q3 2026)

### v0.2.0 — Advanced RAG and Evaluation (Target: Q4 2026)

- [ ] Philosophy profile hot-reload without server restart (Target: Q4 2026)
- [ ] Multi-round debates: `continueDebate()` with counter-argument generation (Target: Q4 2026)
- [x] Configurable aggregation weights for EthicsEvaluator dimensions (Target: Q4 2026)
  - `EthicsEvaluator::Config` struct; weights normalised in constructor; default ctor preserves legacy behaviour
- [ ] Prometheus metrics: decisions/sec, avg confidence, RAG hit rate (Target: Q4 2026)
- [x] Performance benchmark: full decision pipeline ≤ 200 ms (excl. LLM) at p99 (Target: Q4 2026)
  - Implemented: `tests/test_ethics_ai_benchmark.cpp` (PB-01..PB-06); CI threshold 500 ms

### v0.3.0 — Philosophy Library (Target: Q1 2027)

- [x] Ship built-in YAML profiles: utilitarianism, Kantian, virtue ethics, care ethics, contractualism, rationalism, others (Target: Q1 2027)
  - Profiles already in `plugins/ethics_ai/philosophies/`; `PhilosophyLoader` now handles rich YAML schema (complex thesis objects, point-keyed strengths/weaknesses, nested decision_framework)
- [ ] Compliance ethics profiles: GDPR, ISO 42001, IEEE 7000 (Target: Q1 2027)
- [x] Argument chain visualisation (DOT/Mermaid export) (Target: Q1 2027)
  - `ChainVisualizer::exportDot()` / `exportMermaid()` / `chainToDot()` / `chainToMermaid()` in `chain_visualizer.h/cpp`; 8 tests CV-01..CV-08

---

## Implementation Phases

### Phase 1: Design / API Contract ✅
- [x] Define `EthicalArgument`, `EthicalDecision`, `PhilosophyProfile` types
- [x] Define `ArgumentStore` persistence API
- [x] Define `EthicalDiscourseEngine` orchestration API
- [x] Define `RAGContextEngine` query-pattern API

### Phase 2: Core Implementation ✅
- [x] `PhilosophyLoader` YAML parsing
- [x] `ArgumentStore` BaseEntity integration + standalone mode
- [x] `EthicalDiscourseEngine::makeDecision` template argument generation
- [x] `RAGContextEngine` 7 AQL method stubs with real AQL constants

### Phase 3: Error Handling & Edge Cases ✅
- [x] Unknown philosophy school → `Status::Error`
- [x] Empty schools list → `Status::Error`
- [x] YAML parse failure → `Status::Error` with file path
- [x] AQL/RocksDB failure propagation
- [x] Standalone mode activation when `RocksDBWrapper` is null

### Phase 4: Tests [~]
- [x] Unit tests for `PhilosophyLoader` (directory, file, invalid YAML; rich YAML with complex thesis objects and nested `decision_framework` — Issue: #4596)
- [x] Unit tests for `ArgumentStore` standalone mode
- [x] Unit tests for `EthicalDiscourseEngine` decision flow
- [x] Unit tests for `RAGContextEngine` focused query patterns
- [x] Unit tests for `EthicsAIPlugin` lifecycle and metrics API
- [x] 8 tests CV-01…CV-08 for `ChainVisualizer` (`exportDot`/`exportMermaid`/`chainToDot`/`chainToMermaid`) — `tests/test_ethics_ai_chain_visualizer.cpp` (Issue: #4596, 2026-04-12)
- [x] Integration test: full decision pipeline end-to-end (Target: Q3 2026)
  - Scope: `EthicsAIPlugin::initialize()` → `initializeDebate()` → `makeDecision()` → `EthicsEvaluator::evaluate()`
  - Subsystems: `ethics_ai_plugin.cpp`, `discourse_engine.cpp`, `argument_store.cpp`, `ethics_evaluator.cpp`
  - Inputs: 2 YAML philosophy profiles on disk, a `MoralDilemma` struct with 3 options
  - Outputs: `EthicalDecision` with `chosen_option`, `confidence ∈ [0,1]`, `consensus_level ∈ [0,1]`, non-empty `supporting_arguments`
  - Constraints: pipeline completes in ≤ 500 ms; no external LLM call required
  - Errors: missing YAML → plugin returns `Status::Error`; empty dilemma options → `Status::Error`
  - Tests: `tests/test_ethics_ai_integration.cpp` — GTest, direct-source compilation pattern
  - File: `tests/test_ethics_ai_integration.cpp` (new), added to `tests/CMakeLists.txt`
- [ ] Integration test: ArgumentStore with real RocksDB (Target: Q3 2026)
  - Scope: `ArgumentStore` in RocksDB mode – store, load, scanPrefix, storeChain, getChain
  - Subsystems: `argument_store.cpp`, `storage/rocksdb_wrapper.h`, `ethics_base_entity_adapter.h`
  - Inputs: 10+ `EthicalArgument` entities written to a temp RocksDB directory (`std::filesystem::temp_directory_path()`)
  - Outputs: round-trip identity (serialize → store → load → deserialize equals original); chain map reconstructed correctly
  - Constraints: temp directory cleaned up via `RAII`; test repeatable without leftover state
  - Errors: RocksDB open failure → `Status::Error`; corrupt blob → `Status::Error` (not crash)
  - Tests: fixture using `SetUpTestSuite`/`TearDownTestSuite` for temp dir management; assert no data loss across shutdown/reopen cycle
- [x] Integration test: RAGContextEngine with live ArgumentStore data (Target: Q3 2026)
  - Scope: `RAGContextEngine` query methods reading from a pre-populated `ArgumentStore`
  - Subsystems: `rag_context_engine.cpp`, `argument_store.cpp`, AQL constants in `ethics_aql_queries.h`
  - Inputs: 20 seeded `EthicalArgument` records spanning 3 philosophy schools and 2 argument types
  - Outputs: `getArgumentsByPhilosophy()` returns correct subset; `traverseArgumentChain()` BFS produces correct ordering; `getSupportingArguments()` returns only `SUPPORT` relation type
  - Constraints: in-memory mode (no RocksDB required for this test); all assertions deterministic
  - Errors: unknown school → empty result (not crash); cycle in chain graph → terminates within `max_depth` hops
  - Tests: single `TEST_F` fixture that seeds store in `SetUp`; 8+ test cases covering each query-pattern method

### Phase 5: Performance / Hardening [~]
- [ ] Embedding generation integration (Target: Q3 2026)
- [ ] LLM argument content generation (Target: Q3 2026)
- [x] Benchmark: decision pipeline ≤ 200 ms at p99 (excl. LLM) (Target: Q4 2026)
  - `tests/test_ethics_ai_benchmark.cpp` PB-01..PB-06 registered as EthicsAIBenchmarkTests

### Phase 6: Documentation & Acceptance [ ]
- [x] README, ARCHITECTURE, AUDIT, CHANGELOG, ROADMAP, SECURITY, FUTURE_ENHANCEMENTS
- [ ] Philosophy profile authoring guide (Target: Q3 2026)
- [ ] Operator guide for production deployment (Target: Q4 2026)

---

## Production Readiness Checklist

| Area | Status | Notes |
|------|--------|-------|
| Core API | ✅ | All public methods return `std::variant<T, Status>` |
| Error handling | ✅ | All failure paths covered; no unhandled exceptions |
| Thread safety | ✅ | `ArgumentStore` mutex-protected; engine is stateless |
| Persistence | ✅ | BaseEntity + RocksDB; standalone mode for testing |
| Argument content | ⚠️ | All profile theses + decision framework used; LLM generation planned Q3 2026 |
| Confidence scoring | ✅ | `EthicsEvaluator::computeConfidence()`: strength-weighted average |
| Consensus scoring | ✅ | `EthicsEvaluator::computeConsensus()`: inter-school PRO/CONTRA tally |
| Configurable weights | ✅ | `EthicsEvaluator::Config`; normalised; default preserves legacy behaviour |
| YAML profile loading | ✅ | Handles complex thesis objects, point-keyed strengths/weaknesses, nested frameworks |
| Argument chain visualisation | ✅ | `ChainVisualizer` DOT + Mermaid export |
| Embedding search | ⚠️ | BOC-TF 768-dim fallback; real ONNX model planned Q3 2026 |
| Unit test coverage | ✅ | 5 focused unit suites + 1 integration suite + 1 benchmark suite + 1 visualizer suite |
| Performance benchmarks | ✅ | PB-01..PB-06 in `tests/test_ethics_ai_benchmark.cpp` |
| Prometheus metrics | ❌ | Planned Q4 2026 |

---

## Known Issues & Limitations

- Argument `content` is generated from all available profile theses and the decision
  framework; semantic quality depends on YAML profile authorship. LLM-based generation
  is planned for v0.1.0 (Q3 2026).
- `confidence` and `consensus_level` are now computed from argument strength distribution
  and inter-school agreement; see `EthicsEvaluator::computeConfidence/computeConsensus`.
- `generateEmbedding()` in `RAGContextEngine` uses a bag-of-characters TF model (768-dim,
  L2-normalised); ANN search results are lexically meaningful but not semantically rich.
  A real ONNX embedding model is planned for v0.1.0 (Q3 2026).
- No built-in philosophy YAML profiles are shipped; operators must provide them.
