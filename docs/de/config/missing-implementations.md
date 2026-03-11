# Config-Modul — Fehlende Implementierungen
<!-- status: current | validated: 2026-03-09 | primary: src/config/ -->

Dieser Report dokumentiert Punkte, bei denen der Reality-Check (Doku ↔ Sourcecode) Abweichungen
ergeben hat. Er wird beim nächsten Validierungslauf aktualisiert.

**Erstellungsdatum:** 2026-03-09  
**Geprüfte Quellen:** `src/config/`, `benchmarks/`, `tests/`, `tools/`  
**Gesamtbefund:** 2 kritische Pfadfehler behoben; 5 geplante Features noch nicht implementiert (korrekt als `[ ]` markiert)

---

## Übersicht

| ID | Quelle | Erwartet | Beobachtet | Schwere |
|----|--------|----------|------------|---------|
| CFG-001 | `src/config/FUTURE_ENHANCEMENTS.md` → Performance Targets | `benchmarks/config_bench.cpp` | Datei existiert nicht; tatsächlich: `benchmarks/bench_config_path_resolver.cpp` | ⚠️ Pfadfehler (behoben) |
| CFG-002 | `src/config/FUTURE_ENHANCEMENTS.md` → Test Strategy | `tests/config/config_path_resolver_test.cpp` | Datei existiert nicht; tatsächlich: `tests/test_config_path_resolver.cpp` | ⚠️ Pfadfehler (behoben) |
| CFG-003 | `src/config/FUTURE_ENHANCEMENTS.md` → Performance Targets | `benchmarks/bench_config_migration_scanner.cpp` | Datei existiert | ✅ Behoben |
| CFG-004 | `src/config/FUTURE_ENHANCEMENTS.md` → Performance Targets | `tests/config/metrics_scrape_test.cpp` | Datei existiert nicht | ℹ️ Geplant (offen) |
| CFG-005 | `src/config/FUTURE_ENHANCEMENTS.md` § ConfigSchemaValidator Extended Keywords | `allOf`, `anyOf`, `oneOf` implementiert | Nicht in `config_schema_validator.cpp` vorhanden | ℹ️ Geplant für v2.0.0 |
| CFG-006 | `src/config/FUTURE_ENHANCEMENTS.md` § ConfigSchemaValidator Extended Keywords | `$ref` mit `$defs`-Auflösung | Nicht in `config_schema_validator.cpp` vorhanden | ℹ️ Geplant für v2.0.0 |
| CFG-007 | `src/config/FUTURE_ENHANCEMENTS.md` § ConfigSchemaValidator Extended Keywords | `format`, `uniqueItems` | Nicht in `config_schema_validator.cpp` vorhanden | ℹ️ Geplant für v2.0.0 |
| CFG-008 | `src/config/FUTURE_ENHANCEMENTS.md` § ConfigSchemaValidator Extended Keywords | `loadAsJson()` für In-Memory-YAML-String | Nicht in `config_schema_validator.h` vorhanden | ℹ️ Geplant für v2.0.0 |

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

### CFG-004 — Fehlende Metrics-Scrape-Testdatei ℹ️ Geplant

**Claim-Quelle:** `src/config/FUTURE_ENHANCEMENTS.md`, Abschnitt „Performance Targets"  
**Erwartet:** `tests/config/metrics_scrape_test.cpp`  
**Beobachtet:** Datei existiert nicht  
**Evidence:** `find tests/ -name "*metrics_scrape*"` → keine Treffer  
**Ist-Stand:** Die Metrics-Scrape-Latenz (< 1 ms) ist dokumentiert; `benchmarks/bench_config_path_resolver.cpp` enthält einen Prometheus-Scrape-Abschnitt, aber kein dediziertes Testlauf-Gate.  
**Status:** ℹ️ Offen — in FUTURE_ENHANCEMENTS.md aktualisiert (auf bench_config_path_resolver.cpp verwiesen)  
**Issue-Titelvorschlag:** `feat(config): add dedicated metrics scrape latency test`  
**Labels:** `testing`, `observability`, `config`

---

### CFG-005 — ConfigSchemaValidator: allOf / anyOf / oneOf nicht implementiert ℹ️ Geplant für v2.0.0

**Claim-Quelle:** `src/config/FUTURE_ENHANCEMENTS.md`, Abschnitt „ConfigSchemaValidator: Extended JSON Schema Keyword Support"  
**Erwartet:** `allOf`, `anyOf`, `oneOf` implementiert in `config_schema_validator.cpp`  
**Beobachtet:** `grep "allOf\|anyOf\|oneOf" config_schema_validator.cpp` → 0 Treffer  
**Evidence:** Implementierungs-Checkbox `[ ]` (offen) in FUTURE_ENHANCEMENTS.md — kein Code-Evidence vorhanden  
**Status:** ℹ️ Korrekt als `[ ]` (geplant) markiert, zielversion v2.0.0  
**Issue-Titelvorschlag:** `feat(config): implement allOf/anyOf/oneOf JSON Schema keywords in ConfigSchemaValidator`  
**Labels:** `enhancement`, `config`, `schema-validation`

---

### CFG-006 — ConfigSchemaValidator: $ref mit $defs nicht implementiert ℹ️ Geplant für v2.0.0

**Claim-Quelle:** `src/config/FUTURE_ENHANCEMENTS.md`  
**Erwartet:** `$ref` mit lokalem `$defs`/`definitions`-Lookup in `config_schema_validator.cpp`  
**Beobachtet:** `grep '\$ref' config_schema_validator.cpp` → 0 Treffer  
**Evidence:** Implementierungs-Checkbox `[ ]` (offen)  
**Status:** ℹ️ Korrekt als geplant markiert  
**Issue-Titelvorschlag:** `feat(config): implement $ref/$defs JSON Schema keyword in ConfigSchemaValidator`  
**Labels:** `enhancement`, `config`, `schema-validation`

---

### CFG-007 — ConfigSchemaValidator: format / uniqueItems nicht implementiert ℹ️ Geplant für v2.0.0

**Claim-Quelle:** `src/config/FUTURE_ENHANCEMENTS.md`  
**Erwartet:** `format` und `uniqueItems` Keywords in `config_schema_validator.cpp`  
**Beobachtet:** Keine entsprechenden Code-Stellen  
**Status:** ℹ️ Korrekt als geplant markiert  
**Issue-Titelvorschlag:** `feat(config): add format and uniqueItems JSON Schema keywords`  
**Labels:** `enhancement`, `config`, `schema-validation`

---

### CFG-008 — ConfigSchemaValidator::loadAsJson() für In-Memory-String nicht implementiert ℹ️ Geplant für v2.0.0

**Claim-Quelle:** `src/config/FUTURE_ENHANCEMENTS.md`  
**Erwartet:** `loadAsJson(const std::string& yaml_content, bool is_yaml)` Überladung  
**Beobachtet:** Nur `loadAsJson(const std::string& file_path)` vorhanden  
**Evidence:** `config_schema_validator.h` Zeile 130: nur `file_path`-Variante  
**Status:** ℹ️ Korrekt als geplant markiert  
**Issue-Titelvorschlag:** `feat(config): add loadAsJson(string_content) overload to ConfigSchemaValidator`  
**Labels:** `enhancement`, `config`, `schema-validation`

---

## Behobene Findings dieser Review-Runde

| ID | Datei | Änderung |
|----|-------|----------|
| CFG-001 | `src/config/FUTURE_ENHANCEMENTS.md` | `benchmarks/config_bench.cpp` → `benchmarks/bench_config_path_resolver.cpp` |
| CFG-002 | `src/config/FUTURE_ENHANCEMENTS.md` | `tests/config/config_path_resolver_test.cpp` → `tests/test_config_path_resolver.cpp` |
| ROADMAP | `src/config/ROADMAP.md` | Performance-Benchmark-Status `[I]` → `[x]` (bench_config_path_resolver.cpp nachgewiesen) |
| ROADMAP | `src/config/ROADMAP.md` | Unit-Test-Coverage-Status `[~]` → `[x]` (4 Testdateien, 3 363 Zeilen nachgewiesen) |
