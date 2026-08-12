# Performance Expectations Audit

**Zweck:** Automatisierte Verifikation der Top-10 Maßnahmen aus `PERFORMANCE_EXPECTATIONS.md` §1.4.

---

## Überblick

Das Script `tools/perf_expectations_audit.py` und der zugehörige CI-Workflow
`.github/workflows/perf-expectations-audit.yml` prüfen maschinell, ob die in
`PERFORMANCE_EXPECTATIONS.md` Abschnitt **1.4 Top-10 Maßnahmen zur Vollabdeckung**
dokumentierten Maßnahmen implementiert sind.

**Kernregel:**  
Ist eine Maßnahme im Dokument als **ERLEDIGT** markiert, aber die erwarteten
Artefakte/Targets/Registrierungen fehlen, bricht der CI-Job mit Exit-Code 1 ab
und gibt eine klare Fehlermeldung aus.

---

## Geprüfte Maßnahmen (§1.4)

| # | Maßnahme | ERLEDIGT | Geprüfte Evidence |
|---|----------|----------|-------------------|
| 1 | `bench_query.cpp` Pagination-Benchmarks registrieren/stabilisieren | Nein | `BENCHMARK(BM_Pagination_Offset)` + `BENCHMARK(BM_Pagination_Cursor)` in `benchmarks/bench_query.cpp`; `docs/de/search/pagination_benchmarks.md` vorhanden |
| 2 | `bench_olap_analytics.cpp` von Disabled-Stub umstellen | Nein | ≥4 produktive (non-`*_Disabled`) `BENCHMARK()`-Registrierungen in `bench_olap_analytics.cpp` |
| 3 | Security/Governance-Binaries + Runtime-DLL-Sync erzwingen | **Ja** | `bench_security.cpp`, `bench_compliance_security_governance.cpp`, `bench_governance_policy_latency.cpp` vorhanden + zugehörige `add_executable()`-Targets in `benchmarks/CMakeLists.txt` |
| 4 | Voice-Benchmark via `THEMIS_ENABLE_VOICE_ASSISTANT` aktivieren | Nein | `bench_voice_assistant.cpp` + CMake-Target `bench_voice_assistant` + `THEMIS_ENABLE_VOICE_ASSISTANT`-Guard |
| 5 | GPU-Benchmark-Matrix (CUDA/HIP/Vulkan) als separaten Runner | Nein | GPU-Bench-Sources vorhanden; `THEMIS_ENABLE_CUDA`/`THEMIS_ENABLE_HIP`/`THEMIS_ENABLE_GPU`-Guards in CMake; mind. 1 Workflow mit GPU-Runner |
| 6 | Modell-/Artefakt-Vorbereitung (LLM/LoRA/gguf) standardisieren | Nein | Model-Download-Script oder `llm_bench_config.json` vorhanden; `bench_llm*.cpp`-Quellen und `bench_lora_framework.cpp` vorhanden |
| 7 | Ziel-ID-zu-Benchmark-Mapping-Datei erzwingen | Nein | `benchmarks/benchmark_target_mapping.json` oder äquivalentes Dokument vorhanden |
| 8 | CI-Guard „source exists but binary missing" | Nein | Kein `bench_*.cpp` ohne zugehöriges `add_executable()`-Target in `benchmarks/CMakeLists.txt` (Guard ist im Audit-Script implementiert) |
| 9 | Disabled-Stub-Policy einführen | Nein | Alle `BENCHMARK(*_Disabled)`-Registrierungen enthalten Deadline-Kommentar + Issue-Referenz |
| 10 | Modulweise Benchmark-Sweeps (2..33) als Nightly-Presets | **Ja** | Nightly-Workflow `nightly-benchmark-sweep.yml` mit `schedule:`/`cron:`-Trigger (02:00 UTC), CMake-Preset `nightly-bench-sweep`, `tools/bench_coverage_report.py` vorhanden; Coverage-Docs: `docs/de/performance/nightly_bench_sweep.md` |

---

## Lokal ausführen

```bash
# Aus dem Repository-Root (Python 3.8+ erforderlich, keine externen Dependencies):
python3 tools/perf_expectations_audit.py

# Mit explizitem Repo-Root und Output-Verzeichnis:
python3 tools/perf_expectations_audit.py \
  --repo-root /path/to/ThemisDB \
  --output-dir /tmp/audit_output

# Strict-Modus: Exit-Code != 0 auch bei WARNings (nicht nur bei ERLEDIGT-FAILs):
python3 tools/perf_expectations_audit.py --strict

# Nur Zusammenfassung ausgeben (keine Einzel-Check-Details):
python3 tools/perf_expectations_audit.py --quiet

# Markdown-Report überspringen:
python3 tools/perf_expectations_audit.py --no-markdown
```

Das Script schreibt Reports nach `artifacts/perf_expectations_audit/` (relativ zum Repo-Root):
- `report.json` – maschinenlesbarer Report
- `report.md` – menschenlesbarer Markdown-Report

---

## Exit-Codes

| Code | Bedeutung |
|------|-----------|
| `0` | Alle ERLEDIGT-Maßnahmen bestehen. WARNings sind erlaubt. |
| `1` | Mindestens eine ERLEDIGT-Maßnahme hat die Evidence-Prüfung nicht bestanden — oder im `--strict`-Modus: mind. 1 Check fehlgeschlagen. |
| `2` | Interner Fehler / ungültige Argumente. |

---

## CI-Integration

Der Workflow `.github/workflows/perf-expectations-audit.yml` wird ausgelöst bei:

- `push` auf `develop`
- `pull_request` auf `develop` oder `community`
- manuell via `workflow_dispatch` (mit optionalem `strict`-Parameter)

Bei einem Fehler wird der JSON-Report in der Ausgabe zusammengefasst und als
Workflow-Artefakt (`perf-expectations-audit-report-<run_number>`) hochgeladen.

---

## Evidence-Regeln erweitern

Um eine neue Evidence-Regel hinzuzufügen:

1. Öffne `tools/perf_expectations_audit.py`.
2. Suche die Funktion `check_measure_<N>` für die betreffende Maßnahme.
3. Füge einen neuen Check zur `checks`-Liste hinzu:
   ```python
   ok_new = (root / "path/to/expected/file").is_file()
   checks.append({
       "id": "Nc",
       "description": "Expected file exists",
       "result": STATUS_PASS if ok_new else STATUS_FAIL,
   })
   if ok_new:
       evidence.append("path/to/expected/file")
   ```
4. Passe ggf. die `status`-Berechnung am Ende der Funktion an.
5. Führe `python3 tools/perf_expectations_audit.py` lokal aus und verifiziere die Ausgabe.

**ERLEDIGT-Flag setzen:**  
Wenn eine Maßnahme im Dokument als ERLEDIGT markiert wird, setze in der
entsprechenden `check_measure_<N>`-Funktion `"erledigt": True`. Damit löst ein
fehlgeschlagener Check einen CI-Fehler (Exit 1) aus.

---

## Report-Format (JSON)

```json
{
  "meta": {
    "tool": "perf_expectations_audit.py",
    "version": "1.0.0",
    "generated_at": "2026-04-13T06:56:30Z",
    "repo_root": "/path/to/ThemisDB",
    "source_document": "PERFORMANCE_EXPECTATIONS.md §1.4"
  },
  "summary": {
    "total": 10,
    "pass": 4,
    "warn": 6,
    "fail": 0,
    "erledigt_failed": []
  },
  "measures": [
    {
      "id": 3,
      "title": "Security/Governance-Binaries inkl. Runtime-DLL-Sync erzwingen (ERLEDIGT)",
      "erledigt": true,
      "status": "pass",
      "checks": [
        { "id": "3a", "description": "...", "result": "pass" }
      ],
      "evidence": ["benchmarks/bench_security.cpp"],
      "notes": "..."
    }
  ]
}
```

---

## Bekannte Einschränkungen

- **Maßnahme #8 (CI-Guard):** Der Guard ist direkt in diesem Audit-Script implementiert.
  Ein separates externes Guard-Script ist noch nicht vorhanden. Bei sehr vielen
  Bench-Quellen ohne CMake-Target (aktuell ~80 Dateien) erscheint eine WARN-Ausgabe
  mit der vollständigen Liste.

- **Maßnahme #9 (Policy):** Die Prüfung auf Deadline-Kommentare und Issue-Referenzen
  erfolgt im unmittelbaren Kontext der `BENCHMARK(*_Disabled)`-Registrierung (±200
  Zeichen). Kommentare, die weiter oben im File stehen, werden ggf. nicht erkannt.

- **Keine externen Dependencies:** Das Script verwendet ausschließlich die Python
  Standard Library. Es läuft auf Linux- und Windows-Runnern ohne zusätzliche
  Installation.
