## Wissenschaftliche Referenzen (IEEE Style)

[1] S. Gehman et al., “RealToxicityPrompts: Evaluating Neural Toxic Degeneration in Language Models,” Findings of ACL: EMNLP, pp. 3356–3369, Nov. 2020, doi: 10.18653/v1/2020.findings-emnlp.301.

[2] E. M. Smith et al., “I’m sorry to hear that: Finding New Biases in Language Models with a Holistic Descriptor Dataset,” Proc. EMNLP, pp. 9180–9211, Dec. 2022, doi: 10.18653/v1/2022.emnlp-main.625.

Entry-point: `plugins/ethics_ai/CMakeLists.txt` (compatibility shim) · implementation: `src/ethics_ai/` · public API: `include/plugins/ethics_ai/`

| Component | File | Status |
|-----------|------|--------|
| Plugin entry-point | `src/ethics_ai/ethics_ai_plugin.cpp` | ✅ Implemented |
| Philosophy Loader | `src/ethics_ai/philosophy_loader.cpp` | ✅ Implemented |
| Argument Store | `src/ethics_ai/argument_store.cpp` | ✅ Implemented |
| Ethical Discourse Engine | `src/ethics_ai/discourse_engine.cpp` | ✅ Implemented |
| Ethics Evaluator | `src/ethics_ai/ethics_evaluator.cpp` | ✅ Implemented |
| RAG Context Engine | `src/ethics_ai/rag_context_engine.cpp` | ✅ Implemented |
[3] A. Parrish et al., “BBQ: A Hand-Built Bias Benchmark for Question Answering,” Findings of ACL, pp. 2086–2105, May 2022, doi: 10.18653/v1/2022.findings-acl.165.

[4] D. Hendrycks et al., “Aligning AI With Shared Human Values,” arXiv:2008.02275, Aug. 2020 (ICLR 2021 “ETHICS” dataset).

[5] W. M. P. van der Aalst and A. J. M. M. Weijters, “Process mining: A research agenda,” Computers in Industry, vol. 53, no. 3, pp. 231–244, Jun. 2004, doi: 10.1016/j.compind.2003.10.001.

[6] A. K. A. de Medeiros, W. M. P. van der Aalst, and A. J. M. M. Weijters, “Extending the Alpha-Algorithm to Mine Short Loops,” BETA Working Papers, vol. 113, 2004.

- [~] Unit tests for all six components (coverage target ≥ 80 %)
- [~] Unit tests for all six components (coverage target ≥ 80 %) — discourse_engine test added
- [ ] Prometheus metrics wiring into ThemisDB `/metrics` endpoint
- [ ] YAML philosophy profiles CI validation
- [ ] Removal of remaining stubs identified in `STUB_REMOVAL_PLAN.md`

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
- [x] `test_ethics_ai_types.cpp` — type conversion tests (exists, excluded from main runner)
- [x] `test_ethics_evaluator.cpp` — evaluator tests (exists, excluded from main runner)
- [~] `test_discourse_engine.cpp` — `initializeDebate`, `makeDecision`, standalone mode (added 2026-04-08; registered as `test_discourse_engine_focused`)
- [~] `test_philosophy_loader.cpp` — YAML load/parse tests (exists; registered as `test_philosophy_loader_focused`)
- [~] `PhilosophyLoader::addProfile()` added for test injection
- [ ] `test_argument_store.cpp` — standalone + RocksDB-backed store tests
- [ ] `test_rag_context_engine.cpp` — RAGContextEngine tests
- [ ] `test_ethics_ai_plugin.cpp` — plugin lifecycle and initialize/shutdown tests
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
### Zuordnung zu ThemisDB-Validierung
- **Toxizität / Content Safety**: [1] als Regression-Benchmark für `/ethics/decision` Safeguards.
- **Bias-Messung**: [2] (HolisticBias) & [3] (BBQ) für demographische Fairness-Gates.
- **Moralisches Reasoning**: [4] ETHICS-Dataset für 5-Dimensionen-Ethik-CI-Checks.
- **Workflow-/Verwaltungsprozesse**: [5], [6] als methodische Basis für Process-Mining-Abweichungsanalysen (Soll/Ist) in Verwaltungsverfahren.
