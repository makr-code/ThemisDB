# Research Review: Error Awareness and Introspection in ThemisDB

**Datum:** 14. Mai 2026
**Status:** Review-fähig (gegen aktuellen Repository-Stand abgeglichen)
**Sprache:** Deutsch (mit konsistenten englischen Fachbegriffen)
**Scope:** Error Registry, HTTP Error API, MCP Error Tools, Integrations- und Testabdeckung

---

## Abstract / Zusammenfassung

Diese Review bewertet den tatsächlichen Stand von „Error Awareness and Introspection“ in ThemisDB anhand von Code, Tests und Doku-Artefakten. Der zentrale Befund ist, dass die ursprünglich als „fehlend“ beschriebenen Kernbausteine inzwischen weitgehend implementiert sind: Es existiert eine zentrale `ErrorRegistry` mit strukturierten `ErrorCode`-Bereichen und Metadaten, eine HTTP Error API (`/api/v1/errors`, Kategorien, Suche, Lookup per Code) sowie MCP-Tools (`get_error_info`, `search_errors`) für agentische Nutzung.

Die verbleibenden Lücken liegen nicht primär in fehlender Grundfunktionalität, sondern in Reife und Konsistenz über Integrationspfade hinweg: Einige Antworten bleiben fallback-orientiert, und für „Error Awareness Quality“ (z. B. Genauigkeit/Abdeckung/Latenz über alle Oberflächen) fehlt im vorliegenden Scope ein dedizierter End-to-End-Benchmarkbericht. Damit verschiebt sich der Fokus von „Neuimplementierung“ zu „Konsolidierung, Messbarkeit und Außenvertrag“.

---

## Introduction / Einleitung

### Problemstellung

Agentic-AI-Workflows benötigen verlässliche Antworten auf Fragen wie:

- Welche Fehlercodes existieren?
- Was bedeutet ein konkreter Fehlercode?
- Welche Recovery-Hinweise sind dokumentiert?
- Wie lassen sich Fehler maschinenlesbar über API/MCP abrufen?

### Ziel dieser Review

1. Alle zentralen Aussagen mit Code-/Test-/Doku-Belegen abgleichen.
2. Terminologie vereinheitlichen (AQL, Multi-Model, Konsistenzmodell, Komponentenbezeichnungen).
3. Unbelegte oder überholte Behauptungen entfernen.
4. Eine belastbare Argumentationskette bereitstellen: Problem -> Ansatz -> Evaluation -> Grenzen -> Fazit.

### Terminologie (vereinheitlicht)

- **AQL**: Query-Sprache und zugehörige Query-Engine-Pfade in ThemisDB.
- **Multi-Model**: Kombination mehrerer Datenmodelle in einem System (u. a. relational, graph, document, vector, time-series, geospatial).
- **Konsistenzmodell**: ACID/MVCC als zentrale Transaktionsgrundlage im Projektnarrativ.
- **Error Awareness**: Fähigkeit, Fehlerwissen (Code, Ursache, Lösung, Kategorie) strukturiert bereitzustellen.
- **Introspection**: Programmatischer Zugriff auf System- und Fehler-Metadaten (z. B. HTTP, MCP).
- **Außenvertrag (public API contract)**: Versionierbare, öffentlich dokumentierte Spezifikation von Endpunkten, Feldern und Semantik für externe Clients.

---

## Methodik / Ansatz

### M1. Artefaktbasierter Faktencheck

Diese Review stützt sich auf:

- Produktivcode in `include/` und `src/`
- vorhandene Tests in `tests/`
- projektexterne Normquellen (MCP, HTTP-Status-Codes, SemVer)

### M2. Bewertungslogik für Claims

Jeder Claim wird in eine von drei Klassen eingeordnet:

- **Verifiziert**: direkter Beleg in Code und/oder Tests.
- **Teilweise verifiziert**: Implementierung vorhanden, aber mit Abhängigkeiten/Fallbacks oder ohne E2E-Metrik.
- **Nicht verifiziert**: kein belastbarer Nachweis im aktuellen Stand.

### M3. Evidenzprinzip

- Architektur-Claims wurden nur übernommen, wenn ein konkreter Pfad (Datei/Funktion/Test) nachweisbar ist.
- Zukunfts- oder ROI-Behauptungen ohne Mess-/Artefaktbezug wurden entfernt.

---

## Evaluation / Experimente

### E1. Zentraler Error-Katalog (`ErrorRegistry`)

**Beobachtung:** Ein zentraler Registry-Mechanismus mit strukturierten Codes und Metadaten ist implementiert.

- `include/utils/error_registry.h` definiert `ErrorCode`, `ErrorMetadata`, `ErrorRegistry`.
- `src/utils/error_registry.cpp` registriert umfangreiche Default-Errors über mehrere Kategorien.
- `ErrorMetadata` enthält u. a. `category`, `severity`, `message_template`, `cause`, `solution`, `keywords`, `related_docs`.

**Bewertung:** **Verifiziert**.

### E2. HTTP Error API

**Beobachtung:** Öffentliche Endpunkte für Error-Introspection sind vorhanden und geroutet.

- `GET /api/v1/errors`
- `GET /api/v1/errors/{code}`
- `GET /api/v1/errors/categories`
- `GET /api/v1/errors/search?q=...`

Implementierung in `src/server/error_api_handler.cpp`, Routing in `src/server/http_server.cpp`.

**Bewertung:** **Verifiziert**.

### E3. MCP Error Tools für Agentic-Pfade

**Beobachtung:** MCP-Tools für Fehlerabfrage sind registriert und implementiert.

- Tool-Registrierung: `get_error_info`, `search_errors`, zusätzlich `introspect_database`
- Handler: `McpServer::toolGetErrorInfo`, `McpServer::toolSearchErrors`
- Sicherheitsklassifizierung: `AiOperationGuard` behandelt `get_error_info` und `search_errors` als `READ_ONLY`

**Bewertung:** **Verifiziert**.

### E4. Testabdeckung

**Beobachtung:** Relevante Testartefakte existieren.

- `tests/test_error_registry.cpp` (Registry-Verhalten)
- `tests/test_http_error_api.cpp` (HTTP-Endpunkte inkl. Suche/Kategorien/Code-Lookup)
- `tests/manual_test_error_api.sh` (manueller API-Smoke-Test)

**Bewertung:** **Verifiziert** (funktionale Abdeckung vorhanden; kein dedizierter wissenschaftlicher E2E-Benchmark für Quality-Metriken im Scope).

### E5. Claim-Matrix (kompakt)

| Claim | Ergebnis | Beleg |
|---|---|---|
| „Zentrale Fehler-Taxonomie fehlt“ | **Falsch** | `include/utils/error_registry.h`, `src/utils/error_registry.cpp` |
| „Error-Introspection-API fehlt“ | **Falsch** | `src/server/error_api_handler.cpp`, `src/server/http_server.cpp` |
| „MCP kann keine Fehler introspektieren“ | **Falsch** | `src/server/mcp_server.cpp`, `include/server/mcp_server.h` |
| „Error Awareness ist ungetestet“ | **Zu stark** | `tests/test_error_registry.cpp`, `tests/test_http_error_api.cpp` |
| „Messbare End-to-End-Qualität vollständig belegt“ | **Nicht vollständig belegt** | Kein dedizierter protokollübergreifender Qualitätsbenchmark im Scope |

---

## Limitations / Known Issues

1. **Kein einheitlicher Qualitätsbenchmark über alle Oberflächen:** Im aktuellen Repository-Stand liegt kein konsolidierter End-to-End-Artefaktbericht vor, der HTTP + MCP (und weitere Interfaces) gemeinsam entlang Abdeckung/Korrektheit/Latenz quantifiziert; es ist damit primär eine Artefakt-/Implementierungslücke, nicht nur eine Doku-Auslassung.
2. **Fallback-Verhalten bleibt relevant:** Introspektionspfade können je nach Komponentenverfügbarkeit variieren (z. B. reduzierte Antworten in bestimmten Laufzeitkonfigurationen).
3. **Außenvertrag nicht als einzelnes „Error-Awareness-Standarddokument“ konsolidiert:** Implementierungen sind vorhanden, aber Spezifikation/Versionierung als einheitlicher öffentlicher Vertrag ist ausbaufähig.
4. **Terminologie driftet in älteren Research-Drafts:** Begriffe wie „fehlt komplett“ sind in Teilen historisch überholt und sollten bei Folgearbeiten vermieden werden.

---

## Fazit

ThemisDB verfügt bereits über produktive Kernbausteine für Error Awareness und Introspection (Registry, HTTP API, MCP-Tools, Testartefakte). Die ursprüngliche Problemformulierung „Grundfunktionalität fehlt“ ist im aktuellen Stand nicht mehr haltbar. Der nächste sinnvolle Schritt ist keine Neuentwicklung von Basics, sondern die Konsolidierung in drei Richtungen: (1) einheitlicher externer Vertrag, (2) reproduzierbare Qualitätsmetriken, (3) konsistente Kommunikation über alle Interfaces.

---

## References / Quellen

### A) Externe Referenzen (auflösbare URLs)

1. Model Context Protocol (MCP): https://modelcontextprotocol.io/
2. HTTP Semantics (RFC 9110): https://www.rfc-editor.org/rfc/rfc9110
3. Semantic Versioning 2.0.0: https://semver.org/
4. spdlog (Structured Logging Library): https://github.com/gabime/spdlog
5. fmt Library: https://github.com/fmtlib/fmt
6. JSON for Modern C++: https://github.com/nlohmann/json

### B) Interne Artefakte (Code/Test-Belege)

- Error Registry API: `include/utils/error_registry.h`
- Error Registry Implementation: `src/utils/error_registry.cpp`
- HTTP Error API Handler: `include/server/error_api_handler.h`, `src/server/error_api_handler.cpp`
- HTTP Routing/Endpoint-Deklaration: `src/server/http_server.cpp`
- MCP Tool-Interface und Implementierung: `include/server/mcp_server.h`, `src/server/mcp_server.cpp`
- AI Safety Klassifizierung für Error-Tools: `src/security/ai_operation_guard.cpp`
- Tests: `tests/test_error_registry.cpp`, `tests/test_http_error_api.cpp`, `tests/manual_test_error_api.sh`
