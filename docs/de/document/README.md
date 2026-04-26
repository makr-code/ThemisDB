# Document-Modul

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: PRIMARY_SOURCES.md · ../../../include/document/ -->

**Stand:** 6. April 2026  
**Version:** aktuell  
**Kategorie:** Dokumentenverwaltung  
**Status:** 🟢 Production-Ready

---

## Übersicht

Das Document-Modul stellt Dokumentenverwaltungs-Konnektoren für ThemisDB bereit, insbesondere XDOMEA-Integration für den deutschen E-Government-Bereich.

**Primäre Quelle:** [`include/document/`](../../../include/document/)

---

## Kernkomponenten

| Komponente | Header | Beschreibung |
|------------|--------|--------------|
| XDOMEAConnector | `xdomea_connector.h` | XDOMEA-Konnektor (IXDOMEAConnector, InMemoryXDOMEAConnector, msg-types 0201..0601, 30 Tests) |
| EncryptedEntities | `encrypted_entities.h` | Verschlüsselte Dokumententitäten |

---

## Deutsche E-Government-Integration

Der `XDOMEAConnector` implementiert den XDOMEA-Standard für den elektronischen Aktenversand:
- Nachrichtentypen: 0201 (Abgabe), 0401 (Aussonderung), 0501 (Anbietung), 0601 (Bewertung)
- 30 Tests in `tests/test_xdomea_connector.cpp`

---

## Primäre Dokumentation

| Dokument | Beschreibung |
|----------|--------------|
| [`include/document/ARCHITECTURE.md`](../../../include/document/ARCHITECTURE.md) | Architektur |
