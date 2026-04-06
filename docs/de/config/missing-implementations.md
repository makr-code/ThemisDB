# Config-Modul — Fehlende Implementierungen
<!-- status: current | validated: 2026-04-06 | primary: src/config/ -->

Dieser Report dokumentiert Punkte, bei denen der Reality-Check (Doku ↔ Sourcecode) Abweichungen
ergeben hat. Er wird beim nächsten Validierungslauf aktualisiert.

**Erstellungsdatum:** 2026-03-09  
**Geprüfte Quellen:** `src/config/`, `benchmarks/`, `tests/`, `tools/`  
**Gesamtbefund:** 2 kritische Pfadfehler behoben; CFG-005, CFG-006, CFG-007 und CFG-008 implementiert; alle Findings geschlossen

---

## Übersicht

| ID | Quelle | Erwartet | Beobachtet | Schwere |
|----|--------|----------|------------|---------|
| CFG-001 | `src/config/FUTURE_ENHANCEMENTS.md` → Performance Targets | `benchmarks/config_bench.cpp` | Datei existiert nicht; tatsächlich: `benchmarks/bench_config_path_resolver.cpp` | ⚠️ Pfadfehler (behoben) |
| CFG-002 | `src/config/FUTURE_ENHANCEMENTS.md` → Test Strategy | `tests/config/config_path_resolver_test.cpp` | Datei existiert nicht; tatsächlich: `tests/test_config_path_resolver.cpp` | ⚠️ Pfadfehler (behoben) |
| CFG-003 | `src/config/FUTURE_ENHANCEMENTS.md` → Performance Targets | `benchmarks/bench_config_migration_scanner.cpp` | Datei existiert | ✅ Behoben |
| CFG-004 | `src/config/FUTURE_ENHANCEMENTS.md` → Performance Targets | `tests/config/metrics_scrape_test.cpp` | Datei existiert nicht | ℹ️ Geplant (offen) |
| CFG-005 | `src/config/FUTURE_ENHANCEMENTS.md` → ConfigSchemaValidator Extended Keywords | `allOf`, `anyOf`, `oneOf` implementiert | In `config_schema_validator.cpp` vorhanden (seit v1.7.0) | ✅ Implementiert |
| CFG-006 | `src/config/FUTURE_ENHANCEMENTS.md` § ConfigSchemaValidator Extended Keywords | `$ref` mit `$defs`-Auflösung | In `config_schema_validator.cpp` vorhanden (seit v2.0.0) | ✅ Implementiert |
| CFG-007 | `src/config/FUTURE_ENHANCEMENTS.md` § ConfigSchemaValidator Extended Keywords | `format`, `uniqueItems` | In `config_schema_validator.cpp` vorhanden (seit v2.0.0) | ✅ Implementiert |
| CFG-008 | `src/config/FUTURE_ENHANCEMENTS.md` § ConfigSchemaValidator Extended Keywords | `loadAsJson()` für In-Memory-YAML-String | Beide Überladungen in `config_schema_validator.h` L132 + L147 vorhanden | ✅ Implementiert (Doku war veraltet) |

---

## Detailbeschreibung

### CFG-001 — Falscher Benchmark-Dateiname in FUTURE_ENHANCEMENTS.md ✅ Behoben

**Claim-Quelle:** `src/config/FUTURE_ENHANCEMENTS.md`, Abschnitt „Performance Targets" und „Test Strategy"  
**Erwartet:** `benchmarks/config_bench.cpp` (drei Nennungen)  
**Beobachtet:** Datei existiert nicht  
**Evidence:** `ls benchmarks/config_bench.cpp` → `No such file or directory`  
**Tatsächliche Datei:** `benchmarks/bench_config_path_resolver.cpp` (401 Zeilen, Commit `90c733a50`)  
**Status:** ✅ Behoben — FUTURE_ENHANCEMENTS.md korrigiert auf `benchmarks/bench_config_path_resolver.cpp`  
**Issue-Titelvorschlag:** n/a (behoben)

---

### CFG-002 — Falscher Test-Dateipfad in FUTURE_ENHANCEMENTS.md ✅ Behoben

**Claim-Quelle:** `src/config/FUTURE_ENHANCEMENTS.md`, Abschnitt „Test Strategy"  
**Erwartet:** `tests/config/config_path_resolver_test.cpp`  
**Beobachtet:** Datei existiert nicht  
**Evidence:** `ls tests/config/config_path_resolver_test.cpp` → `No such file or directory`  
**Tatsächliche Datei:** `tests/test_config_path_resolver.cpp` (1 339 Zeilen)  
**Status:** ✅ Behoben — FUTURE_ENHANCEMENTS.md korrigiert  
**Issue-Titelvorschlag:** n/a (behoben)

---

### CFG-003 — Fehlende Scanner-Benchmark-Datei ✅ Behoben

**Claim-Quelle:** `src/config/FUTURE_ENHANCEMENTS.md`, Abschnitt „Performance Targets"  
**Erwartet:** `benchmarks/bench_config_migration_scanner.cpp` (oder gleichwertige Benchmark-Datei)  
**Beobachtet:** Datei existiert  
**Evidence:** `benchmarks/bench_config_migration_scanner.cpp` enthält BM_ScanTree_10K (10K Dateien < 5 s)  
**Ist-Stand:** Die Performance-Anforderung (CLI scanner 10K Dateien < 5 s) ist dokumentiert und durch BM_ScanTree_10K verifiziert.  
**Status:** ✅ Behoben — `benchmarks/bench_config_migration_scanner.cpp` implementiert; CMakeLists.txt aktualisiert

---

### CFG-004 — Fehlende Metrics-Scrape-Testdatei ✅ Behoben

**Claim-Quelle:** `src/config/FUTURE_ENHANCEMENTS.md`, Abschnitt „Performance Targets"  
**Erwartet:** `tests/config/metrics_scrape_test.cpp`  
**Beobachtet:** `tests/test_config_metrics_scrape.cpp` (9 Tests: Latenz-Gate cold/warm/repeated, Prometheus-Formatprüfung, Counter-Genauigkeit)  
**Evidence:** `find tests/ -name "*metrics_scrape*"` → `tests/test_config_metrics_scrape.cpp`  
**Ist-Stand:** `ConfigMetricsExporter::collect()` wird in drei Latenztests (< 1 000 µs) und sechs Format-/Korrektheitstests abgedeckt. CMake-Target `MetricsScrapeFocusedTests` registriert.  
**Status:** ✅ Behoben — `tests/test_config_metrics_scrape.cpp` implementiert; `tests/CMakeLists.txt` aktualisiert  
**Issue-Titelvorschlag:** n/a (behoben)

---

### CFG-005 — ConfigSchemaValidator: allOf / anyOf / oneOf ✅ Implementiert

**Claim-Quelle:** `src/config/FUTURE_ENHANCEMENTS.md`, Abschnitt „ConfigSchemaValidator: Extended JSON Schema Keyword Support"  
**Erwartet:** `allOf`, `anyOf`, `oneOf` implementiert in `config_schema_validator.cpp`  
**Status:** ✅ Implementiert — `validateAllOf`, `validateAnyOf`, `validateOneOf` in `src/config/config_schema_validator.cpp`; FUTURE_ENHANCEMENTS.md zeigt `[x]`

---

### CFG-006 — ConfigSchemaValidator: $ref mit $defs ✅ Implementiert

**Claim-Quelle:** `src/config/FUTURE_ENHANCEMENTS.md`  
**Erwartet:** `$ref` mit lokalem `$defs`/`definitions`-Lookup in `config_schema_validator.cpp`  
**Status:** ✅ Implementiert — `resolveRef()` + `validateValueImpl()` in `src/config/config_schema_validator.cpp`; RFC 6901 JSON-Pointer-Walk, Zyklus-Erkennung, SSRF-Schutz (externe URIs abgelehnt); 10 neue Tests in `tests/test_config_schema_validator.cpp`; FUTURE_ENHANCEMENTS.md zeigt `[x]`

---

### CFG-007 — ConfigSchemaValidator: format / uniqueItems ✅ Implementiert

**Claim-Quelle:** `src/config/FUTURE_ENHANCEMENTS.md`  
**Erwartet:** `format` und `uniqueItems` Keywords in `config_schema_validator.cpp`  
**Status:** ✅ Implementiert — `validateString()` prüft `format` (date, date-time, email, uri, ipv4, ipv6); `validateArray()` prüft `uniqueItems`; 20 neue Tests in `tests/test_config_schema_validator.cpp`; `FUTURE_ENHANCEMENTS.md` zeigt `[x]`

---

### CFG-008 — ConfigSchemaValidator::loadAsJson() für In-Memory-String ✅ BEHOBEN

**Claim-Quelle:** `src/config/FUTURE_ENHANCEMENTS.md`  
**Erwartet:** `loadAsJson(const std::string& yaml_content, bool is_yaml)` Überladung  
**Beobachtet (Stand 2026-03-11):** Beide Überladungen sind vorhanden:
- `loadAsJson(const std::string& file_path)` — lädt aus Datei (Zeile 132 in `config_schema_validator.h`)
- `loadAsJson(const std::string& content, bool is_yaml)` — parst In-Memory-String (Zeile 147 in `config_schema_validator.h`)

**Evidence:** `config_schema_validator.h` Zeile 147; `src/config/config_schema_validator.cpp` Zeile 134  
**Status:** ✅ Vollständig implementiert — Dokumentation war veraltet

---

## Behobene Findings dieser Review-Runde

| ID | Datei | Änderung |
|----|-------|----------|
| CFG-001 | `src/config/FUTURE_ENHANCEMENTS.md` | `benchmarks/config_bench.cpp` → `benchmarks/bench_config_path_resolver.cpp` |
| CFG-002 | `src/config/FUTURE_ENHANCEMENTS.md` | `tests/config/config_path_resolver_test.cpp` → `tests/test_config_path_resolver.cpp` |
| CFG-004 | `tests/test_config_metrics_scrape.cpp` | Neue Testdatei erstellt (9 Tests: Latenz-Gate < 1 ms, Prometheus-Format, Counter-Genauigkeit); `MetricsScrapeFocusedTests` in `tests/CMakeLists.txt` registriert |
| ROADMAP | `src/config/ROADMAP.md` | Performance-Benchmark-Status `[I]` → `[x]` (bench_config_path_resolver.cpp nachgewiesen) |
| ROADMAP | `src/config/ROADMAP.md` | Unit-Test-Coverage-Status `[~]` → `[x]` (4 Testdateien, 3 363 Zeilen nachgewiesen) |
| CFG-005 | `src/config/config_schema_validator.cpp` | `allOf`/`anyOf`/`oneOf` implementiert; `[x]` in FUTURE_ENHANCEMENTS.md |
| CFG-006 | `src/config/config_schema_validator.cpp` | `$ref`/`$defs`-Auflösung implementiert (RFC 6901, Zyklus-Schutz, SSRF-Schutz); `[x]` in FUTURE_ENHANCEMENTS.md |
| CFG-007 | `src/config/config_schema_validator.cpp` | `format` (date, date-time, email, uri, ipv4, ipv6) und `uniqueItems` implementiert; 20 neue Tests; `[x]` in FUTURE_ENHANCEMENTS.md |
| CFG-008 | `src/config/config_schema_validator.h` / `.cpp` | `loadAsJson(const std::string& content, bool is_yaml)` Überladung nachgewiesen (h:147, cpp:134); Doku-Status von ℹ️ auf ✅ korrigiert |
| DOC | `src/config/README.md` | Nutzungsbeispiele für `format` und `uniqueItems` ergänzt |
