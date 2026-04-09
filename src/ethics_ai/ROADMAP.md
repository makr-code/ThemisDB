<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Ethics AI Module Roadmap

## Current Status

v0.0.1 — Core plugin skeleton operational. `EthicalDiscourseEngine` orchestrates
debates across any number of philosophy schools; `ArgumentStore` persists entities
via BaseEntity; `RAGContextEngine` provides 7 AQL retrieval patterns. Argument text
generation and scoring are placeholder-quality; real LLM integration is planned for
v0.1.0.

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
- [ ] Dynamic `confidence` score computed from argument strength distribution (Target: Q3 2026)
- [ ] Dynamic `consensus_level` score from inter-philosophy agreement analysis (Target: Q3 2026)
- [ ] Real embedding generation for `vectorSemanticSearch` (sentence-transformers or ONNX) (Target: Q3 2026)

### v0.2.0 — Advanced RAG and Evaluation (Target: Q4 2026)

- [ ] Philosophy profile hot-reload without server restart (Target: Q4 2026)
- [ ] Multi-round debates: `continueDebate()` with counter-argument generation (Target: Q4 2026)
- [ ] Configurable aggregation weights for EthicsEvaluator dimensions (Target: Q4 2026)
- [ ] Prometheus metrics: decisions/sec, avg confidence, RAG hit rate (Target: Q4 2026)
- [ ] Performance benchmark: full decision pipeline ≤ 200 ms (excl. LLM) at p99 (Target: Q4 2026)

### v0.3.0 — Philosophy Library (Target: Q1 2027)

- [ ] Ship built-in YAML profiles: utilitarianism, Kantian ethics, virtue ethics, care ethics, contractualism (Target: Q1 2027)
- [ ] Compliance ethics profiles: GDPR, ISO 42001, IEEE 7000 (Target: Q1 2027)
- [ ] Argument chain visualisation (DOT/Mermaid export) (Target: Q1 2027)

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
- [x] Unit tests for `PhilosophyLoader` (directory, file, invalid YAML)
- [x] Unit tests for `ArgumentStore` standalone mode
- [x] Unit tests for `EthicalDiscourseEngine` decision flow
- [x] Unit tests for `RAGContextEngine` focused query patterns
- [x] Unit tests for `EthicsAIPlugin` lifecycle and metrics API
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

### Phase 5: Performance / Hardening [ ]
- [ ] Embedding generation integration (Target: Q3 2026)
- [ ] LLM argument content generation (Target: Q3 2026)
- [ ] Benchmark: decision pipeline ≤ 200 ms at p99 (excl. LLM) (Target: Q4 2026)

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
| Argument content | ⚠️ | Template strings only; LLM generation planned Q3 2026 |
| Confidence scoring | ⚠️ | Static placeholder values; real scoring planned Q3 2026 |
| Embedding search | ⚠️ | Stubs only; real model integration planned Q3 2026 |
| Unit test coverage | ✅ | 5 focused unit suites + 1 integration suite; 99 tests total, 1 env-skipped |
| Performance benchmarks | ❌ | Not yet measured |
| Prometheus metrics | ❌ | Planned Q4 2026 |

---

## Known Issues & Limitations

- Argument `content` is generated from philosophy `main_theses[0]` with a static
  template; semantic quality depends on YAML profile authorship.
- `confidence = 0.75` and `consensus_level = 0.70` are hardcoded placeholders.
- `generateEmbedding()` in `RAGContextEngine` returns a zero vector; ANN search
  results are meaningless until a real embedding model is wired.
- No built-in philosophy YAML profiles are shipped; operators must provide them.
