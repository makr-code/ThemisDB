# Performance-Expectations Root-Cause Audit (§1.5)

## Zweck

Dieses Dokument beschreibt das automatisierte Audit-System für Abschnitt **1.5
„Ursachenanalyse: warum Erwartungswerte nicht erreicht werden"** in
`PERFORMANCE_EXPECTATIONS.md`.

Das Audit-Script prüft bei jedem PR und Push automatisch, ob die **Gap-Aussagen
in §1.5** noch mit dem aktuellen Sourcecode und den vorhandenen
Benchmark-Artefakten übereinstimmen.

---

## Komponenten

| Datei | Beschreibung |
|-------|-------------|
| `tools/perf_expectations_rootcause_audit.py` | Python-Audit-Script (stdlib only) |
| `.github/workflows/perf-expectations-rootcause-audit.yml` | CI-Workflow |
| `artifacts/perf_expectations_rootcause_audit/report.json` | JSON-Report (erzeugt) |
| `artifacts/perf_expectations_rootcause_audit/report.md`  | Markdown-Report (erzeugt) |

---

## Geprüfte KPIs (§1.5-Zeilen)

| KPI-ID | Bereich | Art der Prüfung |
|--------|---------|-----------------|
| `QUERY_THROUGHPUT` | Query Engine Throughput (900 M/s vs 796 M/s) | Benchmark-Registrierungen vorhanden; Artefaktpfad |
| `VECTOR_INSERT` | Vector Insert (600 k/s vs 548 k/s) | `BM_VectorInsert_Batch100` registriert; CMake-Target; Artefakt |
| `SECONDARY_INDEX_INSERT` | Secondary Index Insert (1 M/s vs 254 k/s) | Benchmark-Registrierungen (SecondaryIndex_Write, Insert_AllIndexes) |
| `STORAGE_SUSTAINED_WRITE` | Storage Sustained Write (Proxy-Claim) | Proxy-Check: existiert inzwischen ein dediziertes `BM_SustainedWrite`? |
| `ANALYTICS_AN10` | Analytics AN-10 ARM NEON (plattformblockiert) | Blocked-Check: sind ARM-Benchmarks registriert + CMake-Target vorhanden? |
| `TPCC_YCSB` | System-Level TPC-C / YCSB (Lite-Profile) | Benchmark-Registrierungen vorhanden; Artefaktpfade |
| `META_CAUSE_2` | bench_olap_analytics.cpp Disabled-Stub (§1.5.1) | Nur `BM_OLAP_Disabled` oder echte Cases? |
| `META_CAUSE_3` | ARM SIMD Benchmark-Datei (§1.5.1) | Datei bench_arm_simd.cpp vorhanden? |

---

## Exit-Codes

| Code | Bedeutung |
|------|-----------|
| `0` | **PASS** oder **WARN-only** – alle Hard-Claims konsistent; ggf. Artefakte fehlen |
| `1` | **FAIL** – mindestens eine harte Behauptung in §1.5 ist durch den Code widerlegt (Dokument ist veraltet) |
| `2` | Interner Fehler / falsche Argumente |

### Was triggert ein FAIL?

Ein **FAIL** wird ausgelöst, wenn eine der folgenden Bedingungen zutrifft:

1. **Stale Proxy-Claim** (`stale_proxy_claim`): Das Dokument behauptet, es gibt
   nur eine Proxy-Messung (kein 1:1 SLO-Benchmark), aber im Code ist bereits ein
   dedizierter Benchmark registriert (z. B. `BM_SustainedWrite`).

2. **Stale Blocked/n/v-Claim** (`stale_blocked_claim`): Das Dokument behauptet,
   ein KPI ist „blockiert" / „n/v", aber der Benchmark ist registriert **und** ein
   CMake-Target existiert (d. h. er ist buildbar).

3. **OLAP Disabled-Stub überholt** (`olap_disabled_stub_stale`): In
   `bench_olap_analytics.cpp` gibt es inzwischen echte BENCHMARK-Fälle über
   `BM_OLAP_Disabled` hinaus.

### Was erzeugt nur ein WARN (kein FAIL)?

- **Fehlende Artefakte** (`missing_artifact`): Das JSON-Artefakt ist nicht im
  Repo versioniert. Bedeutet: Benchmark muss laufen und Output committed werden.
- **Fehlende Benchmark-Datei** (`missing_benchmark_file`): Quelldatei nicht
  gefunden (ggf. umbenannt).
- **Fehlende CMake-Target** (`missing_cmake_target`): Benchmark-Source vorhanden,
  aber kein `add_executable` im CMakeLists.txt.
- **Proxy-Claim noch gültig** (`proxy_no_dedicated_case`): Kein dedizierter 1:1
  Benchmark gefunden; Proxy-Claim im Dokument ist noch korrekt.
- **Blocked, aber nur Source, kein CMake** (`blocked_bench_registered_no_cmake`):
  Benchmark-Cases sind im Source registriert, aber das CMake-Target fehlt → noch
  nicht buildbar, daher nur WARN.

---

## CI-Workflow

```yaml
# Trigger: PR/Push auf develop/community wenn relevante Dateien geändert + Nightly
# Läuft NUR das Audit-Script (keine schweren Benchmarks)
# Artefakte: artifacts/perf_expectations_rootcause_audit/ (30 Tage Retention)
```

**Workflow-Datei:** `.github/workflows/perf-expectations-rootcause-audit.yml`

Der Workflow läuft auf `ubuntu-latest` und benötigt nur Python 3.11 (keine
externen Dependencies).

---

## Lokale Ausführung

```bash
# Einfach ausführen (Outputs in artifacts/perf_expectations_rootcause_audit/)
python3 tools/perf_expectations_rootcause_audit.py

# Mit explizitem Repo-Root
python3 tools/perf_expectations_rootcause_audit.py --repo-root /path/to/ThemisDB

# Nur JSON-Output, kein Farb-ANSI
python3 tools/perf_expectations_rootcause_audit.py --format json --no-color

# Nur Summary anzeigen (keine Detail-Findings)
python3 tools/perf_expectations_rootcause_audit.py --quiet
```

---

## Neue §1.5-Aussagen hinzufügen

Wenn in `PERFORMANCE_EXPECTATIONS.md` eine neue Zeile zu §1.5 hinzugefügt wird,
muss das Audit-Script ebenfalls aktualisiert werden:

1. Öffne `tools/perf_expectations_rootcause_audit.py`.
2. Füge einen neuen Eintrag in der `KPI_DEFINITIONS`-Liste hinzu.
3. Fülle folgende Felder aus:

```python
{
    "kpi_id": "MEIN_KPI",               # Eindeutige ID
    "label": "Mein KPI (§1.5 Label)",   # Beschreibung (für Report)
    "hard_claims": [],                   # Schlüsselwörter aus Dokumenttext
    "benchmark_files": [                 # Benchmark-Quelldateien (relativ zu Repo-Root)
        "benchmarks/bench_mein_kpi.cpp",
    ],
    "required_benchmark_cases": [        # Regex-Muster für BENCHMARK()-Zeilen
        r"BENCHMARK\s*\(\s*BM_MeinKPI\b",
    ],
    "cmake_targets": ["bench_mein_kpi"], # CMake add_executable-Targets
    "artefact_paths": [                  # Erwartete JSON-Artefaktpfade
        "artifacts/perf_nv/targeted_validation/bench_mein_kpi_targeted.json",
    ],
    "proxy_claim": False,   # True = Dokument behauptet "nur Proxy"
    "blocked_claim": False, # True = Dokument behauptet "blockiert/n/v"
}
```

**Proxy-Claim (`proxy_claim: True`):**  
Zusätzlich müssen gesetzt werden:
- `proxy_check_files`: Liste der Dateien, in denen nach dediziertem Case gesucht wird.
- `proxy_check_pattern`: Regex, das einen echten 1:1 SLO-Benchmark matcht.

**Blocked-Claim (`blocked_claim: True`):**  
Zusätzlich müssen gesetzt werden:
- `blocked_check_files`: Liste der Dateien.
- `blocked_check_pattern`: Regex für BENCHMARK-Zeilen des geblockt-gemeldeten KPIs.
- `blocked_cmake_target`: Name des CMake-Targets für diesen Benchmark.

---

## Optionaler Benchmark-Helper: `run_rootcause_validation`

Wenn du die 1.5-KPI-Benchmarks lokal ausführen möchtest, um Artefakte zu
erzeugen, kannst du die entsprechenden Binaries direkt aufrufen. Ein CMake
Custom-Target `run_rootcause_validation` kann lokal hinzugefügt werden
(nicht in CI):

```bash
# Beispiel: Query-Benchmarks für QUERY_THROUGHPUT
./build/benchmarks/bench_query \
  --benchmark_filter="BM_SimpleWhere|BM_ComplexWhere|BM_JoinUsersPosts" \
  --benchmark_out=artifacts/perf_nv/targeted_validation/bench_query_targeted.json \
  --benchmark_out_format=json

# Beispiel: Vector Insert für VECTOR_INSERT
./build/benchmarks/bench_vector_search \
  --benchmark_filter="BM_VectorInsert_Batch100" \
  --benchmark_out=artifacts/perf_nv/targeted_validation/bench_vector_search_targeted.json \
  --benchmark_out_format=json

# Beispiel: Sustained Write für STORAGE_SUSTAINED_WRITE
./build/benchmarks/bench_hotspots_micro \
  --benchmark_filter="BM_SustainedWrite" \
  --benchmark_out=artifacts/perf_nv/targeted_validation/bench_storage_sustained_targeted.json \
  --benchmark_out_format=json

# Beispiel: TPC-C / YCSB für TPCC_YCSB
./build/benchmarks/bench_tpcc \
  --benchmark_out=artifacts/perf_nv/targeted_validation/bench_tpcc_targeted_v2.json \
  --benchmark_out_format=json
./build/benchmarks/bench_ycsb \
  --benchmark_out=artifacts/perf_nv/targeted_validation/bench_ycsb_targeted_v2.json \
  --benchmark_out_format=json
```

Nach Ausführung die JSON-Outputs mit `git add artifacts/perf_nv/...` committen,
damit das Audit die Artefakte findet und WARN → OK wechselt.

---

## Zusammenhang mit anderen Audit-Tools

| Tool | Fokus |
|------|-------|
| `tools/perf_expectations_rootcause_audit.py` | §1.5 Gap-Aussagen vs. Code/Artefakte |
| `tools/error_handling_audit.py` | Error-Handling-Konventionen |
| `cmake-source-coverage-audit.yml` | CMake-Source-Coverage |

---

*Letzte Aktualisierung: 2026-04-13*
