# Ethics AI Plugin – Roadmap

## Current Status

**Status:** 🔧 WIP / Implemented (native C++)

Entry-point: `plugins/ethics_ai/ethics_ai_plugin.cpp`

| Component | File | Status |
|-----------|------|--------|
| Plugin entry-point | `ethics_ai_plugin.cpp` | ✅ Implemented |
| Philosophy Loader | `philosophy_loader.cpp` | ✅ Implemented |
| Argument Store | `argument_store.cpp` | ✅ Implemented |
| Ethical Discourse Engine | `discourse_engine.cpp` | ✅ Implemented |
| Ethics Evaluator | `ethics_evaluator.cpp` | ✅ Implemented |
| RAG Context Engine | `rag_context_engine.cpp` | ✅ Implemented |

---

## In Progress

- [~] Unit tests for all six components (coverage target ≥ 80 %)
- [~] Prometheus metrics wiring into ThemisDB `/metrics` endpoint
- [~] YAML philosophy profiles CI validation
- [~] Removal of remaining stubs identified in `STUB_REMOVAL_PLAN.md`

## Planned Features

- [ ] ≥ 15 philosophy schools (currently 10) (Target: Q3 2026)
- [ ] Python bindings via pybind11 (Target: Q3 2026)
- [ ] AQL-based querying of stored ethical arguments from external clients (Target: Q3 2026)
- [ ] LLM integration: generate natural-language explanations of ethical decisions (Target: Q4 2026)
- [ ] Multi-agent debate simulation: multiple plugin instances negotiate (Target: 2027)
- [ ] Formal verification of ethical decision consistency via property-based testing (Target: 2027)

---

## Short-term Goals (next 1–2 sprints)

- [ ] Add unit tests for all six components (coverage target ≥ 80 %).
- [ ] Wire Prometheus metrics into the ThemisDB metrics endpoint (`/metrics`).
- [ ] Validate YAML philosophy profiles compile and load without errors in CI.
- [ ] Remove any remaining stubs identified in `STUB_REMOVAL_PLAN.md`.

## Mid-term Goals (1–3 months)

- [ ] Extend to **≥ 15 philosophy schools** (currently 10).
- [ ] Benchmark discourse-engine latency for 100-argument debates.
- [ ] Add Python bindings (pybind11) so the plugin can be called from Python scripts.
- [ ] Support AQL-based querying of stored ethical arguments from external clients.

## Long-term Goals (3–12 months)

- [ ] Formal verification of ethical decision consistency (property-based testing).
- [ ] Integration with LLM layer to generate natural-language explanations of decisions.
- [ ] Multi-agent debate simulation: multiple `EthicsAIPlugin` instances negotiate.
- [ ] Production hardening: concurrency safety, resource limits, graceful degradation.

## Milestones

| Milestone | Target | Status |
|-----------|--------|--------|
| Unit test coverage ≥ 80 % | TODO | 🔲 Planned |
| 15 philosophy schools | TODO | 🔲 Planned |
| Python bindings | TODO | 🔲 Planned |
| LLM explanation layer | TODO | 🔲 Planned |

## Implementation Phases

### Phase 1 – Test Coverage ≥ 80 %
- [ ] Write unit tests for `ethics_ai_plugin.cpp`, `philosophy_loader.cpp`, `argument_store.cpp`
- [ ] Write unit tests for `discourse_engine.cpp`, `ethics_evaluator.cpp`, `rag_context_engine.cpp`
- [ ] Wire Prometheus metrics; verify `/metrics` endpoint in CI
- [ ] YAML philosophy profile schema validation in CI; remove stubs from `STUB_REMOVAL_PLAN.md`

### Phase 2 – 15 Philosophy Schools & Python Bindings
- [ ] Add 5+ additional philosophy schools to YAML profiles (reaching ≥ 15 total)
- [ ] Implement pybind11 bindings exposing `evaluate()` and `discourse()` APIs
- [ ] AQL query support for stored ethical arguments (`SELECT … FROM ethics.arguments`)
- [ ] Benchmark discourse-engine latency for 100-argument debates; document SLA

### Phase 3 – LLM Integration
- [ ] Interface with ThemisDB LLM layer to produce natural-language decision explanations
- [ ] Streaming explanation output via Server-Sent Events
- [ ] Graceful degradation when LLM is unavailable (return raw scores)

### Phase 4 – Multi-Agent Simulation & Formal Verification
- [ ] Multi-agent debate: orchestrate N `EthicsAIPlugin` instances with configurable positions
- [ ] Property-based tests (e.g., rapidcheck) verifying decision consistency invariants
- [ ] Concurrency safety audit; resource limits and graceful degradation under load

---

## Dependencies

- ThemisDB `BaseEntity` storage layer
- ThemisDB `QueryEngine` + AQL
- `RocksDBWrapper`
- YAML parser (already in ThemisDB)
- Prometheus client library

## Open Questions

- [ ] Should ethical profiles be user-extensible at runtime (hot-reload)?
- [ ] What is the required decision latency SLA for production use?

---

## Production Readiness Checklist

| Item | Status |
|------|--------|
| 6 core components implemented | ✅ Done |
| Unit test coverage ≥ 80 % | ❌ Pending |
| Prometheus metrics wired | ❌ Pending |
| YAML philosophy profiles validated in CI | ❌ Pending |
| Stubs removed (`STUB_REMOVAL_PLAN.md`) | ❌ Pending |
| Decision latency SLA defined | ❌ Undefined |

## Known Issues & Limitations

- Stubs listed in `STUB_REMOVAL_PLAN.md` have not yet been removed from production code
- Decision latency SLA is undefined; no benchmark baseline established
- Prometheus metrics exist in code but are not yet wired to the ThemisDB `/metrics` endpoint
- Python bindings not yet available; plugin is C++-only

---

*See also: [`future_enhancements.md`](future_enhancements.md) · [`FUTURE_WORK.md`](FUTURE_WORK.md)*
