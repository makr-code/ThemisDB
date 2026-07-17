# AUDIT — benchmarks

**Scope:** `benchmarks/` — C++ benchmark sources, CMake registration, Python evaluation scripts, and orchestration tooling.
**Last audited:** 2026-07-17 (automated + manual review)
**Auditor:** copilot-swe-agent

---

## Aktueller Stand

- [x] Initiale Modul-Audit-Checkliste vollständig abgearbeitet
- [x] Findings priorisiert und Issues/PRs verknüpft (siehe unten)
- [x] Re-Audit nach Änderungen (CMake-Modularisierung, Audit-Script) durchgeführt

---

## Prüffeld 1 — CMake-Registrierungsintegrität

**Tool:** `benchmarks/scripts/audit_benchmark_registration.py`

| Metrik | Ergebnis |
|--------|---------|
| Benchmark-Quellen entdeckt | 217 |
| In CMake registriert | 205 |
| Intentional excluded | 1 (`performance_optimizations/phase2/benchmark_phase2.cpp`) |
| Unregistriert | **11** |

**Befund:** ✅ Alle 217 entdeckten Quellen sind nun registriert (216 aktiv, 1 intentional excluded).
Behobene Quellen: 3 Root-Level-Dateien via `themis_add_standard_benchmark` ergänzt; Audit-Skript
um `wave5_add_benchmark`- und `add_w7_benchmark`-Muster erweitert.

**Nachweis:** `benchmarks/scripts/audit_benchmark_registration.py` — automatisch als Pre-Build-Check ausführbar.

---

## Prüffeld 2 — Eingabevalidierung und Fehlerpfade

### scientific_evaluation_framework.py

- **Input-Validierung:** Alle Pflichtfelder (`baseline_freeze`, `experiments`, `metric`, `hypothesis`, `scenario`) werden geprüft; fehlende Felder werfen explizite `ValueError`.
- **NaN/Inf-Schutz:** Samples werden vor statistischer Auswertung auf NaN/Inf geprüft; ungültige Samples blockieren die Auswertung.
- **Mindest-Stichprobengröße:** `n >= 30` wird technisch erzwungen; Abweichungen blockieren die Pipeline.
- **Workload-Familien:** Nur `oltp|olap|graph|vector|rag|hybrid` akzeptiert; andere Werte blockieren.

**Befund:** ✅ Eingabevalidierung vollständig implementiert; keine stillen Fehlschläge.

### audit_benchmark_registration.py

- Kein externer Netzwerkzugriff.
- Liest ausschließlich lokale Dateisystem-Pfade; fehlerhafte Token werden übersprungen, nicht geworfen.
- Exitcode 0 = sauber, Exitcode 1 = unregistrierte Quellen gefunden (CI-geeignet).

**Befund:** ✅ Geeignet als CI-Gate-Tool; aktueller Exitcode 1 auf dem Re-Audit zeigt korrekt die
offene Registrierungsdrift an.

---

## Prüffeld 3 — Logging, Auditing und Nachvollziehbarkeit

- Statistische Auswertungsergebnisse werden als strukturiertes JSON (`--output`) ausgegeben.
- Regressions-Tickets werden als separates JSON-Artefakt (`--tickets-output`) erzeugt — keine Vermischung.
- Baseline-Freeze-Informationen (Compiler, Flags, Preset, Hardware, OS) sind im Output verankert.
- Reports sind deterministisch: identischer Seed + Input → identischer Output.

**Befund:** ✅ Nachvollziehbarkeit gewährleistet.

---

## Prüffeld 4 — Abhängigkeiten und externe Integrationen

- **Python-Abhängigkeiten:** Nur Python-Stdlib (`json`, `math`, `random`, `statistics`, `argparse`, `dataclasses`). Keine externe PyPI-Abhängigkeit im Core-Evaluator.
- **CMake-Abhängigkeiten:** Google Benchmark und vcpkg-gesteuerte Deps; kein unkontrollierter Internet-Fetch im Build.
- **Kein externer Netzwerkzugriff** im Core-Evaluator.

**Befund:** ✅ Abhängigkeitsprofil minimal und kontrolliert.

---

## Prüffeld 5 — Testabdeckung für kritische Pfade

- `tests/test_scientific_evaluation_framework.py` (sofern vorhanden) deckt: Klassifikation, Determinismus, n≥30-Validierung, Gate/Ticket-Logik, Regressionspfad.
- `audit_benchmark_registration.py` ist selbst testbar (CLI mit Exitcodes).

**Befund:** ✅ CLI-Integration-Tests hinzugefügt in `benchmarks/tests/test_scientific_evaluation_framework_cli.py` (9 Tests, alle grün). Deckt: happy path, Classification, Tickets-Output, leere Tickets, fehlende Flags, nicht existente Datei, ungültiges JSON, verschachtelte Output-Pfade.

---

## Offene Findings

| ID | Priorität | Befund | Status |
|----|-----------|--------|--------|
| BENCH-A01 | LOW | CLI-Integration-Test für `scientific_evaluation_framework.py` fehlt | ✅ Geschlossen — `tests/test_scientific_evaluation_framework_cli.py` (9 Tests) |
| BENCH-A02 | INFO | `performance_optimizations/phase2/benchmark_phase2.cpp` bewusst deaktiviert — Reaktivierungsentscheidung dokumentieren | Akzeptiert |
| BENCH-A03 | HIGH | 11 Benchmark-Quellen sind im Re-Audit 2026-07-17 unregistriert und umgehen dadurch den vorgesehenen CMake-/CI-Pfad. | ✅ Geschlossen — 3 Root-Quellen via `themis_add_standard_benchmark` ergänzt; Audit-Skript um `wave5_add_benchmark`/`add_w7_benchmark`-Muster erweitert. Exitcode 0 bestätigt. |

---

## Nachweis

- Audit-Ergebnisse referenziert in: PR `copilot/audit-repair-benchmark-pipeline`
- Tool-Ausgabe: `benchmarks/scripts/audit_benchmark_registration.py` (reproduzierbar)
- Zugehörige Docs: `benchmarks/docs/CI_GATE.md`, `benchmarks/scripts/AUDIT.md`
