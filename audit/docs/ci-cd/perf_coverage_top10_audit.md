# Performance Coverage Top-10 Measures Audit (§1.4)

## Zweck

Dieses Dokument beschreibt das automatisierte Audit-System für Abschnitt **1.4
„Top-10 Maßnahmen zur Vollabdeckung (alle Module)"** in
`PERFORMANCE_EXPECTATIONS.md`.

Das Audit-Script prüft bei jedem PR und Push automatisch, ob die **10 Maßnahmen
aus §1.4** im aktuellen Sourcecode, Build-System und CI-Konfiguration
implementiert sind oder noch offen liegen.

---

## Komponenten

| Datei | Beschreibung |
|-------|-------------|
| `tools/perf_coverage_top10_audit.py` | Python-Audit-Script (stdlib only) |
| `.github/workflows/perf-coverage-top10-audit.yml` | CI-Workflow |
| `artifacts/perf_coverage_top10_audit/report.json` | JSON-Report (erzeugt) |
| `artifacts/perf_coverage_top10_audit/report.md`  | Markdown-Report (erzeugt) |

---

## Geprüfte Maßnahmen (§1.4)

| ID | Maßnahme | Prüfmethode | FAIL-Bedingung |
|----|---------|-------------|----------------|
| M01 | `bench_query.cpp` Pagination-Benchmarks registrieren | `BENCHMARK(BM_Pagination_Offset)` und `BM_Pagination_Cursor` in `bench_query.cpp` vorhanden | Keiner der beiden registriert |
| M02 | `bench_olap_analytics.cpp` auf echte Cases umstellen | >= 4 echte BENCHMARK-Cases jenseits von `BM_OLAP_Disabled` | Nur `BM_OLAP_Disabled` vorhanden |
| M03 | Security/Governance-Binaries (ERLEDIGT) | CMake-Targets `bench_security`, `bench_governance_policy_latency`, `bench_compliance_security_governance` vorhanden | Mind. ein Target fehlt |
| M04 | Voice-Benchmark-CI-Job | Workflow mit `THEMIS_ENABLE_VOICE_ASSISTANT=ON` + `bench_voice_assistant` | Kein Workflow gefunden |
| M05 | GPU-Benchmark-Matrix | Workflow mit GPU-Runner und `THEMIS_ENABLE_CUDA=ON` oder `THEMIS_ENABLE_HIP=ON` | Kein Workflow gefunden |
| M06 | LLM/LoRA/gguf Modell-Vorbereitung | Setup-Script oder CI-Step für Modell-Download | Kein Script und keine Dokumentation |
| M07 | Ziel-ID-zu-Benchmark-Mapping-Datei | `benchmarks/goal_benchmark_mapping.json` oder äquivalent | Keine Mapping-Datei gefunden |
| M08 | CI-Guard „source exists but binary missing" | Workflow prüft bench_*.cpp vs CMake-Targets | Kein Guard gefunden |
| M09 | Disabled-Stub-Policy | Jede `*_Disabled` Registrierung hat Deadline + Issue-Referenz | Mind. ein Stub ohne Policy |
| M10 | Nightly Benchmark-Sweeps (Module 2..33) | Nightly-Workflow mit per-Modul Coverage-Report | Kein Nightly-Sweep gefunden |

---

## Exit-Codes

| Code | Bedeutung |
|------|-----------|
| `0` | **PASS** oder **WARN-only** – alle FAIL-Klassen-Checks bestehen |
| `1` | **FAIL** – mindestens eine Maßnahme, die implementiert sein sollte, ist es nicht |
| `2` | Interner Fehler / falsche Argumente |

---

## Lokale Ausführung

```bash
# Ausführen (Outputs in artifacts/perf_coverage_top10_audit/)
python3 tools/perf_coverage_top10_audit.py

# Mit explizitem Repo-Root
python3 tools/perf_coverage_top10_audit.py --repo-root /path/to/ThemisDB

# Nur Summary
python3 tools/perf_coverage_top10_audit.py --quiet

# Nur JSON-Output
python3 tools/perf_coverage_top10_audit.py --format json --no-color
```

---

## Aktuelle Gaps (Stand 2026-04-13)

| ID | Status | Begründung |
|----|--------|-----------|
| M01 | ⚠️ WARN | Benchmarks registriert, aber veralteter Kommentar „not currently registered" in Zeile ~144 noch vorhanden |
| M02 | ❌ FAIL | `bench_olap_analytics.cpp` enthält nur `BM_OLAP_Disabled`, keine produktiven Cases |
| M03 | ✅ OK | ERLEDIGT – alle drei CMake-Targets vorhanden |
| M04 | ❌ FAIL | CMake-Target vorhanden, aber kein CI-Job mit `THEMIS_ENABLE_VOICE_ASSISTANT=ON` gefunden |
| M05 | ✅ OK | GPU-Workflow `02-feature-modules_llm_llm-cuda-gpu-ci.yml` mit GPU-Runner und CUDA=ON vorhanden |
| M06 | ⚠️ WARN | Dokumentation vorhanden (`gguf_loader_README.md`, `BENCHMARK_RUNBOOK.md`), aber kein dediziertes Setup-Script |
| M07 | ❌ FAIL | Keine maschinenlesbare `goal_benchmark_mapping.json` vorhanden |
| M08 | ⚠️ WARN | `cmake-source-coverage-audit.yml` vorhanden, prüft aber nicht explizit bench_*.cpp vs Targets (80/169 ohne Target) |
| M09 | ❌ FAIL | 11 Dateien mit `*_Disabled`-Stubs, davon mind. eine ohne Issue-Referenz/Deadline |
| M10 | ⚠️ WARN | Cross-module regression workflow vorhanden, aber kein dedizierter per-Modul Coverage-Sweep mit Nightly-Preset |

---

## Was triggered ein FAIL?

### M01 – Pagination nicht registriert
- `BENCHMARK(BM_Pagination_Offset)` oder `BENCHMARK(BM_Pagination_Cursor)` fehlt in `bench_query.cpp`.

### M02 – OLAP nur Disabled-Stub
- `bench_olap_analytics.cpp` hat ausschließlich `BM_OLAP_Disabled` registriert.
- Fix: Mindestens 4 echte OLAP-Analytics-Cases (z. B. `BM_OLAP_Count`, `BM_OLAP_GroupBy`,
  `BM_OLAP_Aggregation`, `BM_OLAP_Window`) hinzufügen.

### M04 – Kein Voice-CI-Job
- Kein Workflow setzt `THEMIS_ENABLE_VOICE_ASSISTANT=ON` und baut/runnt `bench_voice_assistant`.
- Fix: Optionalen CI-Job hinzufügen (z. B. `02-feature-modules_voice_voice-benchmark-ci.yml`).

### M07 – Keine Mapping-Datei
- Keine maschinenlesbare Datei, die Ziel-IDs (Q-1, AN-10, …) auf Benchmark-Cases mappt.
- Fix: `benchmarks/goal_benchmark_mapping.json` anlegen (Format: `{"Q-1": "BM_SimpleWhere", …}`).

### M09 – Disabled-Stub ohne Policy
- Eine `*_Disabled`-Registrierung hat weder eine Deadline-Angabe noch eine Issue-Referenz.
- Fix: Kommentar hinzufügen, z. B.:
  ```cpp
  // DISABLED: issue #1234, deadline v1.9.0 – convert to real cases by 2026-06-30
  BENCHMARK(BM_OLAP_Disabled);
  ```

---

## Was nur ein WARN erzeugt

- **M01 Stale Comment**: Kommentar „not currently registered" noch im Code.
- **M06 Nur Doku**: Setup nur in README/Runbook, kein ausführbares Script.
- **M08 Partial Guard**: `cmake-source-coverage-audit.yml` vorhanden, aber deckt bench_*.cpp nicht vollständig ab.
- **M10 Partial Sweep**: Cross-module regression workflow vorhanden, aber kein per-Modul Nightly-Coverage-Report.

---

## Neue Maßnahmen hinzufügen

Wenn eine neue Maßnahme zu §1.4 hinzugefügt wird, muss das Script erweitert werden:

1. Füge eine neue Funktion `_mNN_<beschreibung>(repo_root, ...)` in
   `tools/perf_coverage_top10_audit.py` hinzu.
2. Rufe sie in `main()` auf (nach den bestehenden Checks).
3. Dokumentiere Prüfmethode und FAIL-Bedingung in dieser Datei.

---

## Zusammenhang mit anderen Audit-Tools

| Tool | Fokus |
|------|-------|
| `tools/perf_coverage_top10_audit.py` | §1.4 Maßnahmen-Status vs. Code/CI |
| `tools/perf_expectations_rootcause_audit.py` | §1.5 Gap-Aussagen vs. Code/Artefakte |
| `tools/error_handling_audit.py` | Error-Handling-Konventionen |

---

*Letzte Aktualisierung: 2026-04-13*
