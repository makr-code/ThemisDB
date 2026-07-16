# Test Architecture (`tests/`)

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Kontext

`tests/` enthält den verifizierenden Gegenpart zu den Produktionsmodulen in `src/` und den öffentlichen Header-Verträgen in `include/`.

## Architektur-Surfaces

| Surface | Ort |
|---|---|
| Aggregierter Test-Build | `tests/CMakeLists.txt` (`ALL_TEST_SOURCES` + `themis_tests`) |
| Fokus-/Spezialtests | dedizierte Targets in `tests/CMakeLists.txt` (bewusst aus Aggregat ausgenommen) |
| Modulspezifische Tests | z. B. `tests/graph/`, `tests/security/`, `tests/llm/`, `tests/rag/`, `tests/storage/` |
| Cross-modulare Regressionen | Root-nahe `tests/test_*.cpp` |
| Fixture-/Testdatenbasis | `tests/fixtures/`, `tests/data/` |

## Laufzeitmodell

1. CMake sammelt `test_*.cpp` rekursiv und bildet einen aggregierten Testlauf.
2. Nicht aggregierbare Fälle bleiben in dedizierten Focused-Targets.
3. Feature-Gates (z. B. LLM/GPU) steuern Excludes, damit Builds reproduzierbar bleiben.
4. `ctest`-Presets (`linux-release`, `linux-debug`, ...) steuern Ausführung.

## Beziehung zur strategischen Zielarchitektur

Die strategische Zielarchitektur aus `../FUTURE_PLAN.md` wird in `tests/` in Schichten validiert:

- ANN Frontdoor: Retrieval-/Index-/Search-nahe Tests
- Tensor Mid-Layer: Tensor-/Training-/Optimierungsnahe Tests
- Graph Truth Layer: Graph-/Query-/Provenance-nahe Tests
- LLM/LoRA Final Layer: LLM-/RAG-/Safety-nahe Tests

Aktuelle Testabdeckung ist breit, aber nicht als vollständig abgeschlossene Endzustandsvalidierung aller Zielschichten zu verstehen.

## Nicht-Ziele

- keine funktionale Spezifikation von Produktionslogik
- keine produktive Betriebsdokumentation
- keine stillen Flaky-Retries als Ersatz für deterministische Testfixes

## Sourcecode Verification (Scope: tests/<root>)

- Verifiziert:
  - `tests/CMakeLists.txt`
  - `tests/README.md`
  - Verzeichnisstruktur unter `tests/` (Modulordner, `fixtures/`, `data/`, `integration/`)
