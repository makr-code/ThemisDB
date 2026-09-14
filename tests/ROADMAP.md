# ROADMAP (`tests/`)

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

- [x] Root-Testdokumente auf konkrete `tests/`-Struktur und CMake-Realität ausgerichtet (Target: 2026-Q2)
- [x] Veraltete Preset-Referenzen (`linux-ninja-release`) auf `linux-release` aktualisiert (Target: 2026-Q2)
- [x] Source-validierte Testdichte-Matrix fuer 69 code-bearing `src/`-Module plus 4 kanonische Release-Flows veroeffentlicht (`tests/TEST_DENSITY_MATRIX.md`) (Target: 2026-Q3)

## In Progress

- [ ] Root-Dokumente für `tests/config/` und `tests/data/` ergänzen (Target: 2026-Q2)
- [~] Schichtbezogene Abdeckungszuordnung (ANN/Tensor/Graph/LLM) aus bestehenden Test-Suites konsolidieren und an die kanonischen Release-Flows anbinden (Target: 2026-Q3)
- [~] Risk-basierte Closure-Wave 1 fuer `query`, `index`, `rag`, `transaction`, `llm_wiki`, `search` ueber direkte Test-/Benchmark-/Gate-Evidenz verfolgen (Target: 2026-Q4)

## Planned Features

- [x] Dokumentierte Matrix: Produktionsmodul `src/<module>` → verantwortliche Test-Suites (Target: 2026-Q3)
- [ ] Fokus-Targets aus `tests/CMakeLists.txt` als kuratierte Regression-Sets dokumentieren (Target: 2026-Q3)
- [ ] CI-fähige, reproduzierbare Test-Kommandos pro Prioritätsblock dokumentieren (Target: 2026-Q4)
- [ ] Indirekt abgesicherte Module (`distributed_tensor`, `execution`, `llama_cpp`, `stable_diffusion`) auf dedizierte Owner-Suites oder explizite Ausnahme-Regeln umstellen (Target: 2026-Q4)

## Implementation Phases

### Phase 1: Dokumentationskonsolidierung
- [x] Root-Dokumente (`README/ARCHITECTURE/ROADMAP/FUTURE_ENHANCEMENTS`) inhaltlich konkretisieren (Target: 2026-Q2)
- [ ] Fehlende Unterbereichs-Readmes (`tests/config`, `tests/data`) ergänzen (Target: 2026-Q2)

### Phase 2: Abdeckungs-Transparenz
- [x] Modul-zu-Test-Mapping für kritische Runtime-Bereiche veröffentlichen (`tests/TEST_DENSITY_MATRIX.md`) (Target: 2026-Q3)
- [ ] Fokus-Suites mit klaren Akzeptanzkriterien und Ziel-Cadence versehen (Target: 2026-Q3)
- [ ] Flow-Matrix fuer `server->query->storage->transaction`, `sharding<->transaction`, `search->index->tensor->graph->llm`, `server<->llm` als Gate-Backlog pflegen (Target: 2026-Q4)

### Phase 3: Rollout-Härtung
- [ ] Doku-Änderungen regelmäßig gegen CMake-Presets/Test-Presets verifizieren (Target: 2026-Q4)
- [ ] Historische Testreports aus Closure-Entscheidungen entfernen und nur noch als Kontext verlinken (Target: 2026-Q4)

## Production Readiness Checklist

- [x] Root-Dokumente sind source-verifizierbar und nicht nur Template-Platzhalter
- [x] Test-Kommandos referenzieren vorhandene Presets
- [x] Kritische modulübergreifende Regression-Sets explizit dokumentiert (`tests/TEST_DENSITY_MATRIX.md`, Abschnitt "Critical flow coverage matrix")
- [x] Schichtbezogene Abdeckungs-Gaps als priorisierter Backlog gepflegt (`tests/TEST_DENSITY_MATRIX.md`, Abschnitt "Closure waves")
