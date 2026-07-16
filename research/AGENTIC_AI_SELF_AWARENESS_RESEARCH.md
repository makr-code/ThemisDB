# Research Review: Agentic AI Self-Awareness in ThemisDB

**Datum:** 13. Mai 2026
**Status:** Review-fähig (technisch gegen Codebasis abgeglichen)
**Sprache:** Deutsch
**Scope:** Self-Awareness-Fähigkeiten über MCP, HTTP/API, GraphQL, PostgreSQL-Wire, AQL/EXPLAIN, LLM/LoRA

---

## Abstract / Zusammenfassung

Diese überarbeitete Fassung bewertet den Stand von „Agentic AI Self-Awareness“ in ThemisDB auf Basis des aktuellen Repository-Zustands.

Kernaussage: ThemisDB besitzt bereits wesentliche Bausteine für Self-Awareness (u. a. MCP-Tools/Resources, Schema- und Information-Schema-Endpunkte, GraphQL-Introspection, AQL-/Graph-EXPLAIN-Bausteine sowie LoRA-Management mit Multi-GPU-Strategien). Gleichzeitig bestehen Integrationslücken: einige Pfade sind abhängig von optionalen Komponenten (`SchemaManager`, QueryEngine, Build-Flags), und es gibt Inkonsistenzen zwischen dokumentierten und gerouteten Endpunkten (z. B. `/api/v1/capabilities` vs. `/api/capabilities`).

Damit ist die Forschungsfrage nicht mit „alles fehlt“ zu beantworten, sondern mit „teilweise produktiv vorhanden, aber nicht in allen Pfaden konsistent orchestriert“.

---

## Introduction / Einleitung

### Problemstellung

Für Agentic-AI-Szenarien muss ThemisDB Fragen wie „Was kannst du?“, „Welche Datenstrukturen kennst du?“ oder „Wie erkläre ich einen Query-Plan?“ belastbar beantworten können.

### Ziel dieser Review

1. Technische Claims gegen Code, Tests und aktuelle Doku prüfen.
2. Terminologie konsolidieren (AQL, Multi-Model, Konsistenzmodell, Komponenten).
3. Überholte oder unbelegte Aussagen entfernen.
4. Eine nachvollziehbare Argumentationskette bereitstellen: Problem -> Ansatz -> Evaluation -> Grenzen -> Fazit.

### Terminologie (vereinheitlicht)

- **AQL**: ThemisDB Query-Layer und zugehörige Plan-/Explain-Funktionen.
- **Multi-Model**: Kombination mehrerer Datenmodelle im selben System (u. a. relational, graph, vector, document, geospatial, time-series gemäß Root-Dokumentation).
- **Konsistenzmodell**: ACID/MVCC als zentrale Transaktionsgrundlage laut Root-Dokumentation.
- **Self-Awareness**: Fähigkeit, eigene Struktur/Fähigkeiten über APIs/Protokolle maschinenlesbar bereitzustellen (MCP, HTTP, GraphQL, PostgreSQL-Introspection).

---

## Methodik / Ansatz

### M1. Artefaktbasierter Faktencheck

Die Analyse basiert auf:

- **Produktivcode** (insbesondere `src/server`, `src/api`, `src/query`, `src/llm`, `src/metadata`)
- **öffentlichen Headern** in `include/`
- **Tests** in `tests/`
- **aktueller Doku** unter `docs/`

### M2. Bewertungsraster für Claims

Jeder zentrale Claim wird als eine der Kategorien markiert:

- **Verifiziert**: durch konkreten Code-/Test-/Doku-Beleg gestützt.
- **Teilweise verifiziert**: Implementierung existiert, ist aber optional/abhängig/inkonsistent.
- **Nicht verifiziert**: kein belastbarer Beleg im aktuellen Stand.

### M3. Ausschlusskriterium

Unbelegte Zukunftsbehauptungen ohne Code-/Test-/Referenzbezug wurden entfernt.

---

## Evaluation / Experimente

### E1. MCP-basierte Self-Awareness

**Beobachtung:** MCP-Tools und Resources für Schema/Stats sind registriert und implementiert, inkl. Full-/Minimal-Pfad je nach `SchemaManager`-Verfügbarkeit.

- Tool-Registrierung (`get_schema`, `get_stats`, `list_indexes`) ist vorhanden.
- `toolGetSchema` liefert bei fehlendem `SchemaManager` „minimal/error“, sonst `schema_mgr_->toJSON()`.
- `toolGetStats` liefert bei vorhandenem `SchemaManager` strukturierte Metadaten, sonst Minimalantwort.
- Resources `schema://database`, `stats://database`, `metadata://database`, `examples://queries` sind registriert.

**Bewertung:** **Teilweise verifiziert** (funktional vorhanden, aber Integrationsgrad ist umgebungsabhängig).

### E2. HTTP-Schema- und Introspection-Endpunkte

**Beobachtung:** Die ursprüngliche Aussage „/schema fehlt“ ist im aktuellen Stand nicht korrekt.

- Routen für `/api/v1/schema`, `/api/v1/schema/tables`, `/api/v1/schema/tables/{name}` sind vorhanden.
- INFORMATION_SCHEMA-Endpunkte unter `/api/v1/information_schema...` sind vorhanden.
- Zusätzlich existiert `/api/capabilities` als gerouteter Endpunkt.
- Header-Dokumentation nennt `/api/v1/capabilities`; das Routing zeigt jedoch `/api/capabilities`.

**Bewertung:** **Teilweise verifiziert** (breite Funktionalität vorhanden; konsistente Endpunkt-Dokumentation bleibt offen).

### E3. PostgreSQL-Wire und Introspection

**Beobachtung:** PostgreSQL-Schemaqueries (`pg_catalog`, `information_schema`) werden explizit behandelt.

- `PostgresSession::isSchemaQuery()` erkennt relevante Katalogabfragen.
- `handleSchemaQuery()` beantwortet typische BI-Queries und nutzt teils `QueryEngine`-Daten.

**Bewertung:** **Verifiziert** (mit Hinweis: Teile sind „best effort“/Fallback, nicht immer vollständige Live-Metadaten).

### E4. GraphQL-Introspection

**Beobachtung:** GraphQL-Introspection ist implementiert und policy-gesteuert.

- Introspection-Felder `__schema`, `__type`, `__typename` sind explizit behandelt.
- Produktionsgrenzen können Introspection deaktivieren (`QueryLimits::production()`).
- Es existieren dedizierte Tests für Introspection-Verhalten.

**Bewertung:** **Verifiziert**.

### E5. Explain-/Transparenzpfade (AQL/Graph)

**Beobachtung:** Die pauschale Aussage „kein EXPLAIN vorhanden“ ist nicht mehr haltbar.

- AQL-Explain-Schnittstellen (`explainAql`, Text/DOT-Ausgabe) sind vorhanden.
- Graph-API enthält dedizierten EXPLAIN-Endpunkt.
- Query-Plan-Visualisierung ist mit Tests hinterlegt.

**Bewertung:** **Verifiziert** (mindestens auf API-/Komponentenebene).

### E6. LoRA-/Multi-GPU-Awareness

**Beobachtung:** Die LoRA-Infrastruktur ist umfangreich vorhanden.

- `MultiLoRAManager` existiert inkl. Multi-GPU-Strategien (`ROUND_ROBIN`, `DATA_PARALLEL`, `MODEL_PARALLEL`).
- `LoRAMetadataCache` ist implementiert.
- Interne Introspection-nahe Funktionen (`listLoRAs`, `getLoRAInfo`, Memory/Cache-Stats, GPU-Placement-Funktionen) sind vorhanden.

**Bewertung:** **Teilweise verifiziert** für „Self-Awareness nach außen“: intern stark vorhanden, aber ein klar abgegrenztes, konsolidiertes externes Introspection-API für Agentic-Q&A ist nicht als einheitlicher öffentlicher Vertrag nachgewiesen.

### E7. Zusammenfassende Claim-Matrix

| Claim | Ergebnis | Belegtyp |
|---|---|---|
| MCP hat nur Stub-Schema/Stats | **Überholt** -> Teilweise/teils voll integriert je nach `SchemaManager` | Code + Tests |
| `/api/v1/schema` fehlt | **Falsch** | Code-Routing |
| GraphQL-Introspection unklar | **Falsch** | Code + Tests |
| `EXPLAIN` fehlt | **Falsch** | Code + Tests |
| PostgreSQL `information_schema` unklar | **Teilweise** (implementiert, teils fallback-orientiert) | Code + Architekturdoku |
| LoRA-Awareness fehlt vollständig | **Zu stark** (intern viel vorhanden; externe Vereinheitlichung offen) | Code |

---

## Limitations / Known Issues

1. **Abhängigkeitsabhängige Pfade:** Mehrere Self-Awareness-Antworten hängen von optionalen Komponenten/Build-Flags ab (z. B. `THEMIS_ENABLE_LLM`, `SchemaManager`, QueryEngine).
2. **Endpoint-Inkonsistenz:** Dokumentierte Pfade (`/api/v1/capabilities`) und tatsächlich geroutete Pfade (`/api/capabilities`) sind nicht durchgängig harmonisiert.
3. **Uneinheitlicher Integrationsgrad:** Einige Protokollpfade liefern produktive Daten, andere nutzen fallback-orientierte Antworten.
4. **Benchmark-Lage:** Es liegt im aktuellen Scope kein dedizierter, reproduzierbarer End-to-End-Benchmark vor, der „Self-Awareness-Qualität“ (Korrektheit + Latenz + Abdeckung) über alle Protokolle quantitativ ausweist.
5. **Außenvertrag für LoRA-Introspection:** Interne Funktionen sind vorhanden, ein klarer einheitlicher externer API-Vertrag für Agentic-Q&A ist nicht als abgeschlossenes, konsolidiertes Interface dokumentiert.

---

## Fazit

ThemisDB verfügt im aktuellen Stand über eine signifikante technische Basis für Agentic AI Self-Awareness. Frühere Aussagen über „fehlende Grundfunktionalität“ treffen in dieser Form nicht mehr zu. Der aktuelle Schwerpunkt sollte nicht auf der Neuerfindung grundlegender Mechanismen liegen, sondern auf **Konsolidierung**:

- Endpunkt-/Dokumentationsharmonisierung,
- klare Definition von Full-vs-Minimal-Integrationspfaden,
- ein einheitlicher externer Self-Awareness-Vertrag (insbesondere für LoRA-/GPU-Metadaten),
- ergänzende, reproduzierbare Evaluation für Self-Awareness-Qualität.

---

## References / Quellen

### A) Externe Referenzen (auflösbare URLs)

1. Anthropic / Model Context Protocol (MCP): https://modelcontextprotocol.io/
2. GraphQL Specification: https://spec.graphql.org/
3. PostgreSQL Information Schema: https://www.postgresql.org/docs/current/information-schema.html
4. PostgreSQL Frontend/Backend Protocol: https://www.postgresql.org/docs/current/protocol.html
5. llama.cpp (Projekt-Repository): https://github.com/ggml-org/llama.cpp
6. Semantic Versioning 2.0.0: https://semver.org/

### B) Interne Artefakte (Code/Test/Doku-Belege)

- MCP-Implementierung: `src/server/mcp_server.cpp`, `include/server/mcp_server.h`
- MCP-Dokumentation: `docs/de/apis/MCP_PROTOCOL_SUPPORT.md`
- HTTP-Routing/Endpunkte: `src/server/http_server.cpp`, `src/server/schema_api_handler.cpp`, `include/server/schema_api_handler.h`
- HTTP-API-Referenz: `docs/de/apis/HTTP_API_REFERENCE.md`
- PostgreSQL Wire + Schemaqueries: `src/server/postgres_session.cpp`, `docs/architecture/POSTGRESQL_WIRE_PROTOCOL.md`
- GraphQL Introspection: `include/api/graphql.h`, `src/api/graphql.cpp`, `tests/test_graphql_introspection.cpp`
- AQL/Explain-Pfade: `include/query/aql_runner.h`, `src/server/graph_api_handler.cpp`, `tests/test_query_plan_visualizer.cpp`
- LoRA/Multi-GPU: `include/llm/multi_lora_manager.h`, `src/llm/multi_lora_manager.cpp`, `include/llm/lora_metadata_cache.h`, `src/llm/lora_metadata_cache.cpp`
- Zusatztests: `tests/test_mcp_integration.cpp`, `tests/test_information_schema.cpp`
