# Document-Modul

<!-- Status: current | validated: 2026-05-11 -->
<!-- Links: PRIMARY_SOURCES.md · ../../../include/document/README.md · ../../../src/document/README.md -->

**Stand:** 11. Mai 2026
**Version:** aktuell
**Kategorie:** Dokumentenverwaltung
**Status:** 🟢 Production-Ready

---

## Übersicht

Das `document`-Modul stellt öffentliche C++-Schnittstellen für Dokument-CRUD, Lifecycle-Hooks, Schema-Evolution, Diff/Merge sowie XDOMEA-Import/Export bereit.

## Öffentliche Header / Entry-Points

| Header | Zweck |
|--------|-------|
| `document_store.h` | Backend-CRUD (`IDocumentStore`, `InMemoryDocumentStore`) |
| `document_manager.h` | Orchestrierung (`IDocumentManager`, Hooks, verschlüsselte Handles) |
| `document_lifecycle.h` | Hook-Events für Create/Update/Delete |
| `document_schema_evolution.h` | Versionierte Schemata und Validierungsreports |
| `document_diff_merge.h` | Feldbasierte Diffs und 3-Wege-Merge |
| `xdomea_connector.h` | XDOMEA XML Import/Export und In-Memory-Repository |
| `encrypted_entities.h` | Beispielhafte verschlüsselte Entitäten |

## Laufzeitverhalten, Fehlerfälle und Grenzen

- Leere IDs werden abgewiesen (`ERR_DOC_INVALID_ID`).
- Doppelte Inserts/Create liefern `ERR_DOC_ALREADY_EXISTS`.
- Fehlende Reads liefern `std::nullopt` (kein Fehler), fehlende Updates `ERR_DOC_NOT_FOUND`.
- `remove()` ist idempotent.
- `InMemory*`-Implementierungen sind nicht persistent und für Test/Development gedacht.
- Der In-Memory-XDOMEA-Parser nutzt einfache Tag-Extraktion ohne XSD-Validierung.

## Konfigurationsoptionen

Das Modul hat aktuell keine zentrale Konfigurationsdatei; das Verhalten wird über API-Parameter gesteuert:

- Merge-Strategie: `MergeStrategy::{FAIL, OURS_WINS, THEIRS_WINS}`
- XDOMEA-Version: `XDOMEAVersion::{V2_1, V3_0}`
- XDOMEA-Nachrichtentyp: `XDOMEAMessageType` (0201/0202/0203/0401/0501/0601)

## Installation

Das Modul wird über den regulären ThemisDB-Build bereitgestellt:

```bash
cmake --preset linux-release
cmake --build build/linux-release
```

## Usage

```cpp
#include "document/document_manager.h"

themis::document::InMemoryDocumentManager manager;
manager.create("akten", "doc-1", nlohmann::json{{"titel", "Antrag"}});
auto value = manager.get("akten", "doc-1");
```

## Troubleshooting

- `ERR_DOC_SCHEMA_VERSION_NOT_FOUND`: gewünschte Version vorher mit `registerVersion()` registrieren.
- `ERR_DOC_MERGE_CONFLICT`: bei konfliktfreiem Merge bleiben oder alternativ `OURS_WINS`/`THEIRS_WINS` verwenden.
- Leerer XDOMEA-Import: XML auf geschlossene `<dokument>`/`<akte>`-Tags prüfen.

## Primäre Dokumentation

- Public API (`include/document`): [`../../../include/document/README.md`](../../../include/document/README.md)
- Implementierungsübersicht (`src/document`): [`../../../src/document/README.md`](../../../src/document/README.md)
- Primary Sources (DE): [`./PRIMARY_SOURCES.md`](./PRIMARY_SOURCES.md)
- Modulübergreifende Roadmap: [`../../../src/ROADMAP.md`](../../../src/ROADMAP.md)
- Modulübergreifende Future Enhancements: [`../../../src/FUTURE_ENHANCEMENTS.md`](../../../src/FUTURE_ENHANCEMENTS.md)
