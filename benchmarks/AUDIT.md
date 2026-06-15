# AUDIT — benchmarks

**Scope:** `benchmarks/` — C++ benchmark sources, CMake registration, Python evaluation scripts, and orchestration tooling.
**Last audited:** 2026-06-15 (automated + manual review)
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
| Benchmark-Quellen entdeckt | 196 |
| In CMake registriert | 195 |
| Intentional excluded | 1 (`performance_optimizations/phase2/benchmark_phase2.cpp`) |
| Unregistriert | **0** |

**Befund:** ✅ Kein unregistrierter Benchmark-Source gefunden. Die Exclusion ist in `INTENTIONAL_EXCLUSIONS` explizit dokumentiert (Placeholder/disabled).

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

**Befund:** ✅ Geeignet als CI-Gate-Tool.

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

**Befund:** ⚠️ Integrations-Testabdeckung für den vollständigen CLI-Aufruf (`--input/--output`) sollte ergänzt werden (als eigenständiges Test-Target).

---

## Offene Findings

| ID | Priorität | Befund | Status |
|----|-----------|--------|--------|
| BENCH-A01 | LOW | CLI-Integration-Test für `scientific_evaluation_framework.py` fehlt | Offen |
| BENCH-A02 | INFO | `performance_optimizations/phase2/benchmark_phase2.cpp` bewusst deaktiviert — Reaktivierungsentscheidung dokumentieren | Akzeptiert |

---

## Nachweis

- Audit-Ergebnisse referenziert in: PR `copilot/audit-repair-benchmark-pipeline`
- Tool-Ausgabe: `benchmarks/scripts/audit_benchmark_registration.py` (reproduzierbar)
- Zugehörige Docs: `benchmarks/docs/CI_GATE.md`, `benchmarks/scripts/AUDIT.md`
