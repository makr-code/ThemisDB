# Copilot Regression Guard

Der Guard standardisiert die Suche nach Copilot-typischen Build/Link-Fehlern in ThemisDB.

## Scope

- `tests/CMakeLists.txt`: Inventar von `add_executable(...)`-Targets mit nur Testquellen
- `src/**`: Heuristische Zuordnung fehlender Produktions-`.cpp`
- `include/themis_export.h`, `cmake/CMakeLists.txt`, `tests/CMakeLists.txt`: Export-/Import-Makro-Regeln
- Build-Logs: Muster `LNK2001`, `LNK2019`, `LNK1120`

## Regeln (Export/Import-Makros)

Der Guard validiert zentral:

1. `include/themis_export.h` enthält `THEMIS_BASE_EXPORTS`, `THEMIS_TEST_BUILD`, `THEMIS_BASE_API`
2. `tests/CMakeLists.txt` definiert `THEMIS_TEST_BUILD=1` für Test-Binaries
3. `cmake/CMakeLists.txt` setzt `target_compile_definitions(themis_core PRIVATE THEMIS_BASE_EXPORTS)`

## Häufige Copilot-Fehlermuster + empfohlene Fixes

| Fehlermuster | Typisches Symptom | Empfohlener Fix |
|---|---|---|
| Test-Target enthält nur `test_*.cpp` | Später Linker-Fehler im Full-Build | Entweder passende `src/**/<modul>.cpp` per `target_sources` ergänzen **oder** explizit gegen `themis_*`-Library linken |
| Header/API ergänzt, Implementierung fehlt | `LNK2019`/`LNK2001` auf neue Methoden | Implementierungsdatei erzeugen/registrieren und in CMake-Target aufnehmen |
| Export-Makro inkonsistent | Windows-only `dllimport/dllexport` Fehler | `THEMIS_BASE_API` nur über zentrale Makros steuern, Testbuilds mit `THEMIS_TEST_BUILD=1` bauen |
| Scheinbar valide Focused-Tests | Einzeltest baut, Full-Build bricht | Focused-Target um Produktionsquellen/Module ergänzen und Guard-Report prüfen |

## Aktuelles Inventar (automatisch erzeugbar)

```bash
python tools/ci/copilot_regression_guard.py \
  --repo-root . \
  --cmake-file tests/CMakeLists.txt \
  --output-json artifacts/copilot-regression-guard/report.json
```

Aktuell gemeldete, wahrscheinlich fehlende Zuordnungen (Stand Guard-Lauf):

- `test_flatfile_importer_focused` -> `src/importers/flatfile_importer.cpp`
- `test_schema_validator_importer_focused` -> `src/importers/schema_validator.cpp`
- `test_importer_conflict_resolver_focused` -> `src/importers/conflict_resolver.cpp`
- `test_mysql_importer_focused` -> `src/importers/mysql_importer.cpp`
- `test_mongo_importer_focused` -> `src/importers/mongo_importer.cpp`
- `test_sqlite_importer_focused` -> `src/importers/sqlite_importer.cpp`
- `test_oracle_importer_focused` -> `src/importers/oracle_importer.cpp`
- `test_s3_importer_focused` -> `src/importers/s3_importer.cpp`
- `test_metadata_snapshot_focused` -> `src/sharding/metadata_snapshot.cpp`

## CI-Integration

Workflow: `.github/workflows/copilot-regression-guard.yml`

- Führt Guard-Unit-Tests aus
- Erzeugt Inventar/Report als Artefakt
- Enthält reproduzierbare Negativ-Checks:
  - fehlende `.cpp`-Verlinkung (`--strict-missing-sources` muss fehlschlagen)
  - LNK-Muster (`LNK2019`-Fixture muss fehlschlagen)
