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

*See also: [`future_enhancements.md`](future_enhancements.md) · [`FUTURE_WORK.md`](FUTURE_WORK.md)*
