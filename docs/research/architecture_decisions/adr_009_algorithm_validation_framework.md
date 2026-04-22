# ADR-009: Systematisches Algorithm-Validation-Framework für ThemisDB

**Status:** Accepted  
**Date:** 2026-04-22  
**Deciders:** @themisdb-core-team  
**Modules Affected:** alle `src/<modul>/` (cross-cutting)  
**Related Research:**
- [Algorithm Validation Process](../ALGORITHM_VALIDATION_PROCESS.md)
- [Prompting Templates](../PROMPTING_TEMPLATES.md)
- [Performance Expectations](../../../PERFORMANCE_EXPECTATIONS.md)
- Vorbild-Implementierung: mimalloc (src/performance/ROADMAP.md Phase 1)

---

## Context

ThemisDB umfasst >50 Module mit über 215 offenen Roadmap-Items (Stand: 2026-04).
Algorithmische Verbesserungen wurden bisher ad hoc eingeführt — ohne standardisierten
Prozess zur Baseline-Erfassung, Kandidaten-Evaluation und CI-Absicherung.

Das Einzige bislang vollständig durchlaufene Beispiel ist die Einführung von **mimalloc**
im Performance-Modul:
- Baseline eingefroren (tcmalloc / glibc malloc)
- Mehrere Kandidaten verglichen (jemalloc, mimalloc, SnakeMalloc)
- Benchmark reproduzierbar (Google Benchmark, JSON-Output)
- CI-Gate aktiv (`THEMIS_ENABLE_MIMALLOC=ON` im nightly sweep)
- Research-Dokumentation vollständig

Ohne einen standardisierten Prozess drohen folgende Risiken:
1. **Nicht-reproduzierbare Experimente**: Benchmarks ohne fixierte Hardware, Warmup und Statistik
2. **Unsichtbare Regressionen**: Optimierungen ohne CI-Gate können beim nächsten Refactor verloren gehen
3. **Fehlende Dokumentation**: Entscheidungen (Adopt / Reject) sind nicht nachvollziehbar
4. **Duplikate**: Verschiedene Teams evaluieren dieselben Kandidaten unabhängig voneinander
5. **"Gefühlte" Optimierungen**: Verbesserungen ohne SLO-Anker können echte Ziele verfehlen

## Decision Drivers

- **Reproduzierbarkeit:** Jeder Benchmark muss auf einem definierten Hardware-Profil reproduzierbar sein.
- **Messbarkeit:** Jede Optimierung muss an eine konkrete Ziel-ID aus `PERFORMANCE_EXPECTATIONS.md` gebunden sein.
- **CI-Absicherung:** Keine Verbesserung gilt als "done" ohne aktiven Regression-Gate in CI.
- **Dokumentation:** Entscheidungen werden als ADR dauerhaft festgehalten (Adopt oder Reject).
- **Modularität:** Der Prozess muss für alle >50 Module anwendbar sein, ohne Module zu koppeln.
- **LLM-Unterstützung:** Der Prozess muss mit bestehenden Prompt-Engineering-Praktiken des Teams kompatibel sein (`@ollama` + Copilot Cloud).

## Considered Options

| Option | Pros | Cons |
|--------|------|------|
| **A: 6-Schritte-Framework (gewählt)** | Vollständig strukturiert; jeder Schritt hat konkrete Artefakt-Anforderungen; integriert in bestehende ThemisDB-Infrastruktur; LLM-Prompts standardisiert | Overhead pro Experiment (~2–4 h Dokumentation) |
| **B: Freie Experimente mit Nachfassung** | Niedriger Overhead pro Experiment | Kein CI-Gate-Standard; Regressionen nicht blockiert; Dokumentation entsteht erst post-hoc |
| **C: Externes Benchmarking-Tool (z. B. Conbench, Benchdiff)** | Automatisches Tracking ohne manuelle Dokumentation | Zusätzliche externe Abhängigkeit; nicht auf ThemisDB-ADR/Research-System abgestimmt; kein Paper-Link |

## Decision

**Chosen: Option A — 6-Schritte-Framework**

Das Framework formalisiert den mimalloc-Erfolgsweg als wiederholbares Verfahren:

1. **Ziel-ID + SLO fixieren** — aus `PERFORMANCE_EXPECTATIONS.md §1.2`
2. **Baseline einfrieren** — JSON-Output + HW-Profil in `benchmarks/baselines/<modul>/`
3. **Kandidaten sammeln** — ≥5, aus Literatur 2023–heute, mit Research-Steckbrief
4. **Experiment standardisieren** — Welch's t-Test, ≥5 Runs, P50/P95/P99 + Throughput + RSS
5. **CI-Gate** — Workflow-Datei + Mapping in `benchmark_target_mapping.json`
6. **ADR + Research-Doku** — dauerhaft in `docs/research/`

Beide Prompt-Templates (modul-spezifisch + cross-module) sind in `PROMPTING_TEMPLATES.md`
kodifiziert und in das bestehende Ollama-Routing-Framework des Teams integriert.

## Consequences

### Positive

- **Reproduzierbarkeit sichergestellt:** Baseline-JSONs + HW-Profile machen Experimente wiederholbar.
- **Regressionen blockiert:** CI-Gates stellen sicher, dass spätere Code-Änderungen keine Verbesserungen rückgängig machen können.
- **Entscheidungen nachvollziehbar:** ADRs dokumentieren Adopt/Reject-Entscheidungen mit quantitativer Begründung.
- **LLM-Effizienz:** Standardisierte Prompt-Templates vermeiden doppelte Recherche und produzieren konsistente Ausgaben.
- **Kandidaten-Pool skaliert:** Das Research-System (`docs/research/papers/`, `docs/research/best_practices/`) aggregiert Kandidaten modulartig — spätere Module profitieren von frühen Evaluierungen.

### Negative / Trade-offs

- **Dokumentations-Overhead:** Jedes vollständig durchlaufene Experiment erfordert ~2–4 Stunden Dokumentation. *Mitigation: LLM-Prompts übernehmen die Struktur-Erstellung; Entwickler füllen nur Metriken und Bewertungen aus.*
- **Experiment-Overhead:** ≥5 Runs + Statistik erhöhen Benchmark-Zeit. *Akzeptiert: ohne Statistik sind Ergebnisse nicht belastbar.*
- **Kein Nutzen für triviale Optimierungen:** Konstante-Faktor-Verbesserungen (< 5 %) müssen trotzdem die Checkliste durchlaufen. *Mitigation: "Minor"-Verbesserungen können mit vereinfachtem ADR-Stub dokumentiert werden.*

### Neutral

- Das Framework ändert keine bestehenden Interfaces oder APIs.
- Bestehende Benchmarks und CI-Workflows bleiben unverändert; das Framework erweitert sie.
- Module, die das Framework noch nicht durchlaufen haben (Mehrzahl der >50 Module), sind nicht blockiert — das Framework gilt für neue Experiment-Starts.

## Validation

- [x] Framework-Dokument erstellt: `docs/research/ALGORITHM_VALIDATION_PROCESS.md`
- [x] Prompt-Templates erstellt: `docs/research/PROMPTING_TEMPLATES.md`
- [x] Kompatibilität mit bestehendem Research-System geprüft (`_template_paper.md`, `_template_decision.md`)
- [x] Kompatibilität mit bestehendem CI-System geprüft (`07-quality_nightly-benchmark-sweep.yml`, `performance_regression_detector.py`, `verify_benchmark_mapping.py`)
- [x] Kompatibilität mit Ollama-Routing-Framework geprüft (`tools/copilot-ollama-router/`)
- [ ] Pilotanwendung auf ein zweites Modul (neben mimalloc) — geplant Q3 2026
- [ ] Framework-Review nach erstem Piloten — geplant 2026-09-30

## Follow-up Actions

- [ ] Pilotanwendung: Template A auf `src/index/` für Ziel-ID `I-L2Distance` anwenden (Target: Q3 2026)
- [ ] Pilotanwendung: Template B für Module `src/index/`, `src/query/`, `src/cache/` (Target: Q3 2026)
- [ ] Experiment-Verzeichnis anlegen: `docs/research/experiments/` mit README (Target: Q3 2026)
- [ ] CI-Integration: Experiment-Protokoll-Validierung in nightly sweep (Target: Q4 2026)
- [ ] Quarterly Update: Framework-Erfahrungen in `docs/research/stand_der_technik/2026_q2_landscape.md` (Target: 2026-06-30)

## Related Decisions

- [ADR-001: HNSW over FAISS for ANN Vector Index](adr_001_hnsw_over_faiss_vector_index.md)
- [ADR-002: RocksDB as Primary Storage Backend](adr_002_rocksdb_storage_backend.md)

---
**Last Updated:** 2026-04-22
