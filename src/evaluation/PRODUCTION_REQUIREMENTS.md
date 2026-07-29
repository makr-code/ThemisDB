> **Status:** 2026-07-29 – mit aktuellem Evaluation-Code und Statusdokumenten fuer Issue #5643 abgeglichen.

# ThemisDB Evaluation Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt fuer produktive Mindestanforderungen**
des Evaluation-Moduls. Es definiert verbindliche Betriebs-, Sicherheits- und
Freigabeanforderungen fuer EPIC-2-Evaluationspfade und ergaenzt die Phasenplanung aus
`ROADMAP.md`.

## Dokumentabgrenzung (Canonical Split)

- **`src/evaluation/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/evaluation/README.md`:** Funktionsuebersicht, Modulgrenzen, aktuelle Lieferstufe.
- **`src/evaluation/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/evaluation/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Integrationsziele.
- **`src/evaluation/MODULE_EVIDENCE.md`:** Build-/Test-/Benchmark-Nachweise und validierte Evidence-Gaps.

## Verbindliche Produktionsanforderungen

- **MUST:** Planner-, Approximation-, und Artefakt-Fallbacks muessen maschinenlesbar und erklaerbar bleiben.
- **MUST:** Fehlende oder veraltete Tensor-/Shard-Manifeste fuehren zu fail-closed Verhalten oder exaktem Fallback; keine truth-bearing Degradation.
- **MUST:** Advisory-only Tensor-Semantik und CPU-only Graph-Truth-Finalisierung bleiben erhalten.
- **MUST:** Produktionsfreigaben duerfen nur auf benchmark- und testgestuetzter Evidence basieren.
- **MUST NOT:** Default-Workflow-Integration aktivieren, solange Phase 3 bis 6 nicht nachweisbar gruen sind.
- **MUST NOT:** Summary-only Wahrheitsresultate ohne Graph-Verifikation einfuehren.

## Verbindliche Sicherheits- und Zuverlaessigkeitsanforderungen

- Policy-Entscheidungen und Downgrades muessen operator-sichtbar sein.
- Fehler in sicherheits- oder korrektheitskritischen Evaluationspfaden werden explizit propagiert; kein Silent-Permit.
- Hardware-/Profil-Annahmen muessen validiert werden; unpassende Voraussetzungen duerfen keinen stillen Best-Effort-Truth-Pfad erzeugen.
- Build-/Test-/Benchmark-Blocker muessen in `MODULE_EVIDENCE.md` dokumentiert werden, wenn aktuelle Evidence nicht erzeugt werden kann.

## Betriebsgrenzen

- Bewertungs- und Routingpfade bleiben heuristisch/policy-basiert; learned cost models sind aktuell nicht Bestandteil der Produktionsfreigabe.
- Verteilte Summary-First-Pfade sind nur zulaessig, wenn Manifest-Freshness und Exact-on-Demand-Vertraege eingehalten werden.
- Benchmark-Claims gelten erst nach dokumentierter Messung im passenden Preset/Umfeld.

## Minimaler Produktions-Check (Audit-faehig)

- [ ] Phase-3 Fehler- und Fallback-Semantik fuer die produktiven EPIC-2-Surfaces ist dokumentiert und verifiziert
- [ ] Fokus-Tests wurden erfolgreich gebaut und ausgefuehrt oder eine justified gap ist dokumentiert
- [ ] Relevante Benchmarks wurden ausgefuehrt und Guardrails sind dokumentiert
- [ ] Advisory-only Tensor-Semantik und Graph-Truth-Finalisierung sind unveraendert
- [ ] Default-Workflow-Integration bleibt deaktiviert, bis alle Gates bestanden sind

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review

- `src/evaluation/PRODUCTION_REQUIREMENTS.md`
- `src/evaluation/README.md`
- `src/evaluation/ROADMAP.md`
- `src/evaluation/FUTURE_ENHANCEMENTS.md`
- `src/evaluation/MODULE_EVIDENCE.md`
- `src/evaluation/src/query_planner.cc`
- `src/evaluation/src/approximation_rules.cc`
- `src/evaluation/src/artifact_lifecycle.cc`
- `tests/epic2_evaluation/CMakeLists.txt`
- `benchmarks/epic2_evaluation/CMakeLists.txt`
