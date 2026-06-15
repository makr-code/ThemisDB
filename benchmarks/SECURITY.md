# SECURITY — benchmarks

**Scope:** `benchmarks/` — C++ benchmark sources, CMake registration, Python evaluation scripts, orchestration tooling.
**Last reviewed:** 2026-06-15
**Reviewer:** copilot-swe-agent

---

## Bedrohungsmodell

| Bedrohung | Relevanz | Bewertung |
|-----------|----------|-----------|
| Unvalidierte Eingaben (JSON/Pfade) | Mittel | Geprüft — Input-Validierung vorhanden |
| Externe Netzwerkzugriffe | Niedrig | Kein externer Netzwerkzugriff im Core |
| Leakage sensibler Daten in Reports | Niedrig | Reports enthalten nur Messdaten |
| Code-Injection über Benchmark-Input | Mittel | Geprüft — nur JSON-Parse, kein `eval` |
| Supply-Chain-Risiken (Deps) | Niedrig | Nur Python-Stdlib im Core |
| Fehlkonfiguration CMake | Niedrig | Audit-Script prüft Vollständigkeit |

---

## Mindestanforderungen

- [x] Eingaben strikt validieren und Fehler explizit behandeln
  - `scientific_evaluation_framework.py`: Alle Pflichtfelder validiert, NaN/Inf geblockt, n≥30 erzwungen.
  - `audit_benchmark_registration.py`: Dateipfade normalisiert, ungültige Token übersprungen.
- [x] Geheimnisse niemals im Klartext ablegen
  - Keine Secrets, Credentials oder API-Tokens in Benchmark-Quellen oder Reports.
  - CMake-Targets enthalten keine eingebetteten Credentials.
- [x] Security-relevante Änderungen mit Tests absichern
  - Benchmark-Registrierungsintegrität ist durch `audit_benchmark_registration.py` maschinell prüfbar.
- [x] Abhängigkeiten regelmäßig auf Schwachstellen prüfen
  - Core-Evaluator verwendet nur Python-Stdlib — kein PyPI-Risk im Kernpfad.
  - C++-Deps über vcpkg verwaltet; vcpkg-Baseline-Freeze im Preset verankert.

---

## Findings

| ID | Schwere | Befund | Status |
|----|---------|--------|--------|
| BENCH-S01 | INFO | `load_test_data.py` akzeptiert externe URLs als Eingabe — Redirect-Handling nicht auditiert. | Offen — Low-Risk da Testdaten-Script, kein Produktionspfad |
| BENCH-S02 | INFO | CMake-Build-Flags (`-march=native`) können bei Cross-Compile zu undefiniertem Verhalten führen — kein Security-Finding, aber Reproduzierbarkeitsrisiko. | Akzeptiert |

---

## Kein Security-Blocker gefunden

Der Benchmark-Stack ist für seinen Scope (lokale Performance-Messung und Auswertung) sicherheitstechnisch angemessen aufgestellt. Keine kritischen oder hohen Findings.

---

## Incident & Meldung

Sicherheitsfunde gemäß Root-[`SECURITY.md`](../SECURITY.md) melden und behandeln.
