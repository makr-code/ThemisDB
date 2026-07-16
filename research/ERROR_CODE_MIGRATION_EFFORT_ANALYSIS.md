# Error Code Migration Effort Analysis (Research Review)

**Datum:** 2026-05-14
**Status:** Review-fähige, codebasierte Neubewertung

## Abstract / Zusammenfassung

Dieses Review bewertet den aktuellen Migrationsaufwand für ein konsistentes Error-Code-System in ThemisDB auf Basis der heutigen Codebasis (Stand: 2026-05-14). Die frühere Einschätzung in diesem Dokument (227 Error/Warn-Statements) ist nicht mehr aktuell. Eine reproduzierbare statische Auswertung zeigt deutlich höhere Bestände: 2.333 `spdlog::error()`/`spdlog::warn()`-Aufrufe in 249 C/C++-Dateien. Gleichzeitig existiert bereits produktive Infrastruktur (`ErrorCode`, `ErrorRegistry`, HTTP- und MCP-Fehlerabfrage), sodass keine Greenfield-Einführung nötig ist. Der realistische Schwerpunkt liegt daher auf priorisierter Restmigration, Governance (Code-Style/Checks) und risikobasierter Einführung statt kompletter Big-Bang-Umstellung. [R1][R2][R3][R4]

## Introduction / Einleitung

### Problem

ThemisDB ist als **Multi-Model**-Datenbank positioniert (relational, graph, vector, document, geospatial, time-series) und nutzt im Query-Layer **AQL**. Das System beansprucht starke Transaktionsgarantien (u. a. MVCC/SSI/2PC in den Projektunterlagen). Für Betrieb, Debugging und API-Transparenz ist ein konsistentes Fehlerbild über Module hinweg erforderlich. [R5]

### Ziel dieses Reviews

1. Faktischer Abgleich der Aussagen gegen den aktuellen Repository-Zustand.
2. Vereinheitlichung der Terminologie (AQL, Multi-Model, ErrorRegistry, Konsistenzmodell).
3. Bewertung, welche Teile der Migration bereits umgesetzt sind.
4. Aktualisierte Aufwandseinschätzung mit klaren Annahmen, Risiken und Grenzen.

## Methodik / Ansatz

### M1: Statische Codeanalyse (reproduzierbar)

Für dieses Review wurden C/C++-Dateien (`*.c,*.cc,*.cpp,*.cxx,*.h,*.hh,*.hpp,*.hxx,*.ipp,*.tpp`) außerhalb von Build-/Release-Artefakten analysiert.

Verwendete Metriken:

- Anzahl `spdlog::error()`
- Anzahl `spdlog::warn()`
- Anzahl Dateien mit mindestens einem dieser Aufrufe
- Anzahl `errors::logError()`
- Anzahl `ErrorCode`-Einträge und `registerError(...)`-Registrierungen

### M2: Architektureller Faktencheck

Verifiziert wurden folgende Artefakte:

- `include/utils/error_registry.h` (ErrorCode-System)
- `src/utils/error_registry.cpp` (Registry-Befüllung)
- `src/server/error_api_handler.cpp` + `src/server/http_server.cpp` (REST-Error-API)
- `src/server/mcp_server.cpp` (`get_error_info`)
- `tests/test_error_registry.cpp` (Tests auf Registry-Verhalten)
- `docs/migration/ERROR_HANDLING_MIGRATION.md` (Migrationsleitfaden)

### M3: Terminologie-Regeln (dieses Dokument)

- **AQL**: Query-Layer-Terminologie in ThemisDB.
- **Multi-Model**: Systemebene, nicht einzelne Module.
- **Konsistenzmodell**: entlang der Projektclaims in README (MVCC/SSI/2PC) benannt.
- **Error-Code-System**: `ErrorCode` + `ErrorRegistry` + konsumierende APIs/Tools.

## Evaluation / Experimente

## E1: Aktueller Umfang der klassischen spdlog-Fehlerpfade

Messung (2026-05-14):

- `spdlog::error()`: **1.260**
- `spdlog::warn()`: **1.073**
- Summe: **2.333** Aufrufe
- Betroffene Dateien: **249**

Interpretation: Eine Vollmigration aller klassischen Log-Pfade ist deutlich größer als in der früheren Version angenommen.

## E2: Bereits vorhandene Error-Code-Infrastruktur

Messung (2026-05-14):

- `errors::logError()`: **33** Aufrufe in **7** Dateien
- `ErrorCode`-Enum-Einträge (`include/utils/error_registry.h`): **184**
- `registerError(...)`-Aufrufe in `src/utils/error_registry.cpp`: **119**

Interpretation: Das Zielsystem ist bereits produktiv nutzbar, aber die Nutzung ist noch nicht flächendeckend.

## E3: API- und Tool-Integration

Vorhandene Integrationspunkte:

- REST-Endpunkte: `GET /api/v1/errors`, `/api/v1/errors/{code}`, `/api/v1/errors/categories`, `/api/v1/errors/search` (Routing/Dispatch in `src/server/http_server.cpp`; Handler in `src/server/error_api_handler.cpp`).
- MCP-Tool: `get_error_info` in `src/server/mcp_server.cpp`.

Interpretation: Fehlerwissen ist nicht nur intern, sondern bereits extern (REST/MCP) konsumierbar.

## E4: Was an der alten Fassung fachlich nicht mehr trägt

Die frühere Kernannahme „~227 Error/Warn-Statements in ~26 Dateien“ ist durch aktuelle Messung widerlegt. Daraus folgen:

- Zeit-/Kostenannahmen aus der alten Fassung sind als historische Schätzung zu behandeln.
- Eine einmalige Komplettmigration über alle Module ist kurzfristig nur mit hohem Koordinationsaufwand realistisch.
- Priorisierte Migration (kritische Pfade zuerst) ist technisch angemessener.

## Aktualisierte Aufwandseinordnung

Unter Annahme einer priorisierten Migration (kritische Laufzeitpfade + externe API-Sichtbarkeit zuerst):

1. **Phase A – Governance & Tooling (1-2 Sprints)**
   Migrationskriterien, Mapping-Regeln, CI-Checks für neue Fehlerpfade.
2. **Phase B – High-Impact-Module (3-5 Sprints)**
   Fokus auf Module mit hoher Betriebsrelevanz (z. B. Storage, Query, Network, API).
3. **Phase C – Breitenmigration & Konsolidierung (fortlaufend)**
   Kontinuierliche Migration bei Feature-/Bugfix-Arbeit, plus Regressionstests.

Diese Einschätzung ersetzt frühere absolute Zahlen (Personentage/ROI) durch ein belastbareres, inkrementelles Vorgehen auf Basis des Ist-Zustands.

## Limitations / Known Issues

1. **Nur statische Auswertung:** Die Messung bewertet Aufrufhäufigkeit, nicht Fehlerkritikalität oder Laufzeitimpact.
2. **Makro-/Wrapper-Effekte:** Nicht jeder relevante Fehlerpfad ist zwingend über direkte `spdlog::error/warn`-Treffer erfassbar.
3. **Historische Artefakte im Repo:** Archivdokumente können frühere Zustände enthalten und dürfen nicht unkritisch als aktueller Stand gelesen werden.
4. **Keine dedizierte Error-Migrations-Benchmarkserie identifiziert:** Performanceeffekte der Umstellung sind separat zu messen.
5. **Registry-Abdeckung vs. Nutzung:** Viele Codes sind definiert, aber noch nicht überall im operativen Pfad verankert.

## Fazit

Die zentrale Aussage nach Review lautet:

- ThemisDB besitzt bereits ein substantielles Error-Code-Fundament.
- Der Restaufwand liegt primär in der **systematischen Adoption** über die Breite der Codebasis.
- Ein **risikobasiertes, inkrementelles Migrationsmodell** ist dem Big-Bang-Ansatz klar überlegen.

Damit ist die Migration nicht „klein“, aber klar planbar, wenn Priorisierung, Terminologie und Governance strikt umgesetzt werden.

## References / Quellen

- [R1] ThemisDB Repository, `include/utils/error_registry.h` (ErrorCode-Definitionen).
  [https://github.com/makr-code/ThemisDB/blob/develop/include/utils/error_registry.h](https://github.com/makr-code/ThemisDB/blob/develop/include/utils/error_registry.h)
- [R2] ThemisDB Repository, `src/utils/error_registry.cpp` (Registry-Registrierungen).
  [https://github.com/makr-code/ThemisDB/blob/develop/src/utils/error_registry.cpp](https://github.com/makr-code/ThemisDB/blob/develop/src/utils/error_registry.cpp)
- [R3] ThemisDB Repository, `src/server/error_api_handler.cpp` (Error-API Handler).
  [https://github.com/makr-code/ThemisDB/blob/develop/src/server/error_api_handler.cpp](https://github.com/makr-code/ThemisDB/blob/develop/src/server/error_api_handler.cpp)
- [R4] ThemisDB Repository, `src/server/http_server.cpp` (Error-API Routing).
  [https://github.com/makr-code/ThemisDB/blob/develop/src/server/http_server.cpp](https://github.com/makr-code/ThemisDB/blob/develop/src/server/http_server.cpp)
- [R5] ThemisDB Repository, `README.md` (Multi-Model-, AQL- und Konsistenz-Claims).
  [https://github.com/makr-code/ThemisDB/blob/develop/README.md](https://github.com/makr-code/ThemisDB/blob/develop/README.md)
- [R6] ThemisDB Repository, `src/server/mcp_server.cpp` (`get_error_info` Tool).
  [https://github.com/makr-code/ThemisDB/blob/develop/src/server/mcp_server.cpp](https://github.com/makr-code/ThemisDB/blob/develop/src/server/mcp_server.cpp)
- [R7] ThemisDB Repository, `tests/test_error_registry.cpp` (Registry-Tests).
  [https://github.com/makr-code/ThemisDB/blob/develop/tests/test_error_registry.cpp](https://github.com/makr-code/ThemisDB/blob/develop/tests/test_error_registry.cpp)
- [R8] ThemisDB Repository, `docs/migration/ERROR_HANDLING_MIGRATION.md` (Migrationsleitfaden).
  [https://github.com/makr-code/ThemisDB/blob/develop/docs/migration/ERROR_HANDLING_MIGRATION.md](https://github.com/makr-code/ThemisDB/blob/develop/docs/migration/ERROR_HANDLING_MIGRATION.md)
- [R9] Meyer, B. (1992). Applying "Design by Contract". *Computer*, 25(10), 40-51. DOI: [https://doi.org/10.1109/2.161279](https://doi.org/10.1109/2.161279)
- [R10] P0709R4: Herb Sutter (2019). Zero-overhead deterministic exceptions: Throwing values.
  [https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0709r4.pdf](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0709r4.pdf)
