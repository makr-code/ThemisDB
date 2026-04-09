<!-- Status: current | validated: 2026-04-09 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Ethics AI Module Roadmap

## Current Status

v0.0.2 — Core plugin skeleton operational. Unit tests for all six components are
now in place (70 tests across four focused test targets). `PhilosophyLoader::addProfile()`
added for programmatic test injection. `ArgumentStore` school_id key fix applied.
Argument text generation and scoring remain placeholder-quality; real LLM integration
is planned for v0.1.0.

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
- [x] `PhilosophyLoader::addProfile()` — programmatic profile injection for unit tests (`philosophy_loader.h`)
- [x] Unit tests for all six components — 70 tests across four focused targets:
  `DiscourseEngineFocusedTests` (11), `ArgumentStoreStandaloneTests` (18),
  `EthicsAiPluginTests` (28), `RAGContextEngineTests` (13); all registered in
  `tests/CMakeLists.txt` under `THEMIS_PLUGIN_ETHICS_AI` guard (2026-04-08)
- [x] BaseEntity adapter for ethics types
- [x] `std::variant<T, Status>` error handling throughout all public APIs
- [x] Standalone in-memory mode for `ArgumentStore` (testing without RocksDB)

---

## In Progress [~]

*(none — all previously in-progress items are now complete)*

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

### Phase 4: Tests [x]
- [x] Unit tests for `PhilosophyLoader` (directory, file, invalid YAML)
- [x] Unit tests for `ArgumentStore` standalone mode — `test_argument_store_standalone.cpp` (18 tests)
- [x] Unit tests for `EthicalDiscourseEngine` decision flow — `test_discourse_engine.cpp` (11 tests)
- [x] Unit tests for `EthicsAiPlugin` lifecycle and config — `test_ethics_ai_plugin.cpp` (28 tests)
- [x] Unit tests for `RAGContextEngine` — `test_rag_context_engine.cpp` (13 tests)
- [ ] Integration tests combining full pipeline + RAG + evaluate (Target: Q3 2026)

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
| Unit test coverage | ✅ | 70 tests across four focused targets (v0.0.2) |
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
