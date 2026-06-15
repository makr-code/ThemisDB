# SECURITY — benchmarks/scripts

**Scope:** `benchmarks/scripts/` — Python evaluation and audit scripts.
**Last reviewed:** 2026-06-15
**Reviewer:** copilot-swe-agent

---

## Bedrohungsmodell

| Bedrohung | Relevanz | Bewertung |
|-----------|----------|-----------|
| Unvalidierte JSON-Eingaben | Mittel | ✅ Geprüft — alle Pflichtfelder validiert |
| Code-Injection via `eval`/`exec` | Hoch | ✅ Kein `eval`/`exec` in Evaluator-Kern |
| Externes Netzwerk im Core | Niedrig | ✅ Kein externer Zugriff in `scientific_evaluation_framework.py` |
| URL-Injection in `load_test_data.py` | Niedrig | ⚠️ Externe URLs ohne Allowlist (Testdaten-Script) |
| Secrets in Benchmark-Reports | Niedrig | ✅ Reports enthalten nur Messdaten/Statistiken |
| Pfad-Traversal in CMake-Audit | Niedrig | ✅ Pfade werden relativ zum BENCHMARKS_DIR normalisiert |

---

## Mindestanforderungen

- [x] Eingaben strikt validieren — NaN/Inf, fehlende Felder, ungültige Workload-Familien blockiert
- [x] Geheimnisse niemals im Klartext ablegen — keine Credentials in Scripts oder Outputs
- [x] Abhängigkeiten: nur Python-Stdlib in Kernpfaden — kein PyPI-Supply-Chain-Risk
- [x] Fehler explizit melden — kein stiller Fehlschlag im Evaluator
- [ ] `load_test_data.py` URL-Allowlist ergänzen (BENCH-S01, Low-Risk, offen)

---

## Findings

| ID | Schwere | Befund | Status |
|----|---------|--------|--------|
| BENCH-S01 | LOW | `load_test_data.py` akzeptiert externe URLs ohne Allowlist | Offen — kein Produktionspfad |
| BENCH-S02 | INFO | Historisch: C-Block-Header in `scientific_evaluation_framework.py` verursachte Python-SyntaxError — behoben in PR `copilot/audit-repair-benchmark-pipeline` | Geschlossen |

---

## Keine kritischen oder hohen Security-Findings

---

## Incident & Meldung

Sicherheitsfunde gemäß Root-[`SECURITY.md`](../../SECURITY.md) melden und behandeln.
