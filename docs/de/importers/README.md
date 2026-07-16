# Importers-Modul

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: PRIMARY_SOURCES.md · ../../../src/importers/README.md -->

**Stand:** 6. April 2026  
**Version:** aktuell  
**Kategorie:** Datenimport / Integration  
**Status:** 🟢 Production-Ready

---

## Übersicht

Das Importers-Modul bietet eine umfassende Datenimport-Pipeline für ThemisDB: adaptiver Import, Entity-Linking, Konfliktauflösung, CRDT-basierte Zusammenführung und E-Government-spezifische Konnektoren.

**Primäre Quelle:** [`src/importers/`](../../../src/importers/) · [`include/importers/`](../../../include/importers/)

---

## Kernkomponenten (Auswahl)

| Komponente | Header | Beschreibung |
|------------|--------|--------------|
| ImporterInterface | `importer_interface.h` | Basis-Importer-Schnittstelle |
| AdaptiveImport | `adaptive_import.h` | Adaptiver Import mit Schema-Evolution |
| EntityLinker | `entity_linker.h` | Entity-Linking und Disambiguation |
| EntityMatcher | `entity_matcher.h` | Entity-Matching (fuzzy, deterministisch) |
| ConflictResolver | `conflict_resolver.h` | Konfliktauflösung bei Import-Konflikten |
| CRDTImporter | `crdt_importer.h` | CRDT-basierter konfliktfreier Import |
| DataQuality | `data_quality.h` | Datenqualitätsprüfung und -scoring |
| BlockchainIntegrity | `blockchain_integrity.h` | Blockchain-Integritätsprüfung |
| CanonicalResolver | `canonical_resolver.h` | Kanonische Entitäts-Auflösung |
| FlatfileImporter | `flatfile_importer.h` | CSV/TSV/JSON-Flat-File-Import |
| OZGServiceRegistry | `ozg_service_registry.h` | OZG-Serviceregister (E-Government, 30 Tests) |
| XOEVImporter | `xoev_importer.h` | XÖV-Standard-Importer (XOEVStandard, 30 Tests) |

---

## Deutsche E-Government-Konnektoren

| Connector | Standard | Beschreibung |
|-----------|---------|--------------|
| OZGServiceRegistry | OZG | Onlinezugangsgesetz-Serviceregister (Bund + Länder) |
| XOEVImporter | XÖV | XÖV-Standard-Daten (XDOMEA, XMeld, etc.) |

---

## Primäre Dokumentation

| Dokument | Beschreibung |
|----------|--------------|
| [`src/importers/README.md`](../../../src/importers/README.md) | Modulübersicht |
