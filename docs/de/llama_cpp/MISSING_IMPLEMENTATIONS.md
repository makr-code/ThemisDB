[docs](../../index.md) > [de](../index.md) > [llama_cpp](./index.md) > [reference](./MISSING_IMPLEMENTATIONS.md)
**Datum:** 2026-04-16
**Status:** review
**Primary (Quelle der Wahrheit):**
- `src/llama_cpp/ROADMAP.md`
- `src/llama_cpp/FUTURE_ENHANCEMENTS.md`
- `src/llama_cpp/llama_cpp_plugin.cpp`
- `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`

**Bezug / Reference:**
- Issue: `[MODULE] llama_cpp`
- Kontext: Task 4 — Report zu offenen Lücken (Impact, Evidence, Priorisierung, Folge-Issues).

---

# Missing Implementations — llama_cpp

## Offene Lücken

| ID | Lücke | Impact | Evidence | Priorität | Folge-Issue (Vorschlag) |
|---|---|---|---|---|---|
| LC-MI-01 | Concurrency-Hardening für `loadModel`/`generate` fehlt | Risiko für Race-Conditions unter Last; potenziell instabile Runtime-States | `src/llama_cpp/ROADMAP.md` (Phase 5 offen), keine Concurrency-Tests in `src/llama_cpp/tests/test_llama_cpp_plugin.cpp` | Hoch | `[llama_cpp] Add concurrent loadModel/generate race coverage and synchronization hardening` |
| LC-MI-02 | Parallel-Lasttest für `generateBatch` fehlt | Performance-/Threading-Verhalten unter Mehrfachaufrufen nicht abgesichert | `src/llama_cpp/ROADMAP.md` (Phase 5 offen), keine Parallel-Caller-Tests | Mittel | `[llama_cpp] Add parallel caller stress tests for generateBatch` |
| LC-MI-03 | Function/Tool Calling nicht implementiert | API-Capability bleibt eingeschränkt (`supports_function_call=false`) | `src/llama_cpp/ROADMAP.md` Planned Features, `src/llama_cpp/llama_cpp_plugin.cpp:316` | Mittel | `[llama_cpp] Implement JSON-schema constrained function/tool calling` |
| LC-MI-04 | Request-Cancellation nicht implementiert | Laufende Inferenz nicht sauber abbrechbar; Ressourcenbindung bei langen Requests | `src/llama_cpp/ROADMAP.md` Planned Features | Mittel | `[llama_cpp] Add per-request cancellation token support` |

## Dokumentationsbezogene Konsistenzlücken (Primary)

Diese Punkte sind keine fehlenden Runtime-Features, aber blockieren eine saubere Primary→Secondary-Konsolidierung:

- `README.md`, `ARCHITECTURE.md`, `AUDIT.md`, `FUTURE_ENHANCEMENTS.md` sind teils nicht auf dem Stand der implementierten v2.1.0-Funktionalität.
- `FUTURE_ENHANCEMENTS.md` verwendet ein abweichendes Makro (`THEMIS_ENABLE_LLAMA_CPP` statt `THEMIS_LLM_ENABLED`).

## Priorisierte Reihenfolge

1. **LC-MI-01** (Hoch) — Stabilität/Correctness
2. **LC-MI-02** (Mittel) — Performance/Regression-Schutz
3. **LC-MI-03** (Mittel) — API-Ausbau
4. **LC-MI-04** (Mittel) — Operative Robustheit
