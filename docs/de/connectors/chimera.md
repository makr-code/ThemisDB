# Chimera — Datenbank-Adapter-Framework

**Stand:** 6. April 2026  
**Version:** 1.1.0  
**Kategorie:** Connectors / Adapter  
**Status:** 🟡 Alpha (Simulation Mode — alle Adapter implementiert)

---

## Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Architektur](#architektur)
- [Source-Code-Referenz](#source-code-referenz)
- [Implementierte Adapter](#implementierte-adapter)
- [Capability-Matrix](#capability-matrix)
- [Nutzung (Kurzbeispiel)](#nutzung-kurzbeispiel)
- [Konfiguration](#konfiguration)
- [Bekannte Einschränkungen](#bekannte-einschränkungen)
- [Verwandte Dokumentation](#verwandte-dokumentation)

---

## Übersicht

Das **Chimera**-Modul (Comprehensive Hybrid Inferencing & Multi-model Evaluation Resource
Assessment) ist das herstellerneutrale Datenbank-Adapter-Framework von ThemisDB. Es
ermöglicht ein faires, reproduzierbares Benchmarking verschiedener Datenbanksysteme
(relational, dokumentenorientiert, graphbasiert, vektorbasiert, hybrid) über eine einheitliche
Schnittstelle.

Benannt nach dem mythologischen Wesen mit mehreren Formen, abstrahiert Chimera die
native API jedes Datenbanksystems und stellt konsistente Abfrage-, Transaktions- und
Metriken-Interfaces für Benchmark- und Integrations-Konsumenten bereit.

**Kernprinzipien:**

- **Herstellerneutralität** — Keine system-spezifischen Typen oder Konzepte im Interface.
- **Interface Segregation** — `IDatabaseAdapter` aufgeteilt in fünf Sub-Interfaces
  (`IRelationalOps`, `IVectorOps`, `IGraphOps`, `IDocumentOps`, `ITransactionOps`).
- **Factory Pattern** — Adapter werden zur Laufzeit per Name registriert und instanziiert.
- **Result-basierte Fehlerbehandlung** — Alle Operationen liefern `AdapterResult<T>`
  (kein Exception-Throwing im Benchmark-Hot-Path).

---

## Architektur

```
Benchmark Harness
       │
       ▼
AdapterFactory::create("ThemisDB")
       │
       ▼
┌─────────────────────────────────────────────────────────┐
│                  AdapterFactory (Singleton)             │
│  register_adapter(name, fn) · create(name) · list()     │
└──────┬──────┬──────┬──────┬──────┬──────┬──────┬───────┘
       │      │      │      │      │      │      │
  ThemisDB  MongoDB  PG   ES  Pinecone  Qdrant  Weaviate  Neo4j
```

Detaillierte Architekturinformationen: [src/chimera/ARCHITECTURE.md](../../../src/chimera/ARCHITECTURE.md)

---

## Source-Code-Referenz

| Komponente | Header | Source | Beschreibung |
|-----------|--------|--------|--------------|
| AdapterFactory | `include/chimera/database_adapter.hpp` | `src/chimera/adapter_factory.cpp` | Thread-sicheres Singleton-Registry |
| ThemisDBAdapter | `include/chimera/themisdb_adapter.hpp` | `src/chimera/themisdb_adapter.cpp` | ThemisDB-Referenz-Implementierung |
| MongoDBAdapter | `include/chimera/mongodb_adapter.hpp` | `src/chimera/mongodb_adapter.cpp` | MongoDB (Dokument + Atlas Vector Search) |
| PostgreSQLAdapter | `include/chimera/postgresql_adapter.hpp` | `src/chimera/postgresql_adapter.cpp` | PostgreSQL (Relational + pgvector) |
| ElasticsearchAdapter | `include/chimera/elasticsearch_adapter.hpp` | `src/chimera/elasticsearch_adapter.cpp` | Elasticsearch (Volltext + Vektor) |
| PineconeAdapter | `include/chimera/pinecone_adapter.hpp` | `src/chimera/pinecone_adapter.cpp` | Pinecone (verwaltete Vektorsuche) |
| QdrantAdapter | `include/chimera/qdrant_adapter.hpp` | `src/chimera/qdrant_adapter.cpp` | Qdrant (native Vektordatenbank) |
| WeaviateAdapter | `include/chimera/weaviate_adapter.hpp` | `src/chimera/weaviate_adapter.cpp` | Weaviate (native Vektordatenbank) |
| Neo4jAdapter | `include/chimera/neo4j_adapter.hpp` | `src/chimera/neo4j_adapter.cpp` | Neo4j (native Graphdatenbank) |

---

## Implementierte Adapter

| System | Typ | Status | Besonderheiten |
|--------|-----|--------|----------------|
| **ThemisDB** | Multi-model (relational, vektor, graph, dokument, TX) | ✅ Referenz-Impl. | Alle 5 Sub-Interfaces |
| **MongoDB** | Dokumentenorientiert + Atlas Vector Search | ✅ Simulation | Cosine Similarity |
| **PostgreSQL** | Relational + pgvector | ✅ Simulation | JSONB-Dokumente, TX |
| **Elasticsearch** | Volltext + k-NN Vektorsuche | ✅ Simulation | HTTP-basiert |
| **Pinecone** | Verwaltete Vektorsuche | ✅ Simulation | Upsert, Query, Delete |
| **Qdrant** | Native Vektordatenbank | ✅ Simulation | Collections, Payload-Filter |
| **Weaviate** | Native Vektordatenbank | ✅ Simulation | Objekte, semantische Suche |
| **Neo4j** | Native Graphdatenbank | ✅ Simulation | Cypher, Graph-Traversal |

> **Simulation Mode:** Alle Adapter verwenden im Test-Modus `std::unordered_map`-internen
> Speicher — kein laufender Datenbankserver erforderlich. Produktiv-Deployments erfordern
> die jeweilige native Client-Bibliothek (z. B. `libmongocxx`, `libpqxx`,
> `cpp-httplib`/`cpr` für HTTP-basierte Adapter).

---

## Capability-Matrix

| Operation | ThemisDB | MongoDB | PostgreSQL | Elasticsearch | Pinecone | Qdrant | Weaviate | Neo4j |
|-----------|----------|---------|------------|---------------|----------|--------|----------|-------|
| Relational CRUD | ✅ | ❌ | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ |
| Dokument CRUD | ✅ | ✅ | ✅ | ✅ | ❌ | ❌ | ✅ | ❌ |
| Vektor-Suche | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ❌ |
| Graph-Traversal | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ |
| Transaktionen | ✅ | ✅ | ✅ | ❌ | ❌ | ❌ | ❌ | ✅ |

---

## Nutzung (Kurzbeispiel)

```cpp
#include "chimera/database_adapter.hpp"
#include "chimera/themisdb_adapter.hpp"

// Adapter per Factory erstellen
auto adapter = chimera::AdapterFactory::create("ThemisDB");
if (!adapter) {
    // Adapter nicht registriert
    return;
}

// Verbindung herstellen
chimera::AdapterConfig config;
config.connection_string = "themisdb://localhost:7777/mydb";
auto connect_result = adapter->connect(config);

// Dokument einfügen
nlohmann::json doc = {{"name", "Alice"}, {"score", 42}};
auto insert_result = adapter->insertDocument("users", doc);

// Vektor-Suche
std::vector<float> query_vec = {0.1f, 0.2f, 0.3f};
auto search_result = adapter->vectorSearch("embeddings", query_vec, /*k=*/10);

// Verbindung trennen
adapter->disconnect();
```

Alle verfügbaren Systeme abfragen:

```cpp
auto systems = chimera::AdapterFactory::get_supported_systems();
// Ergebnis (alphabetisch):
// ["Elasticsearch", "MongoDB", "Neo4j", "Pinecone", "PostgreSQL",
//  "Qdrant", "ThemisDB", "Weaviate"]
```

---

## Konfiguration

| Parameter | Standard | Beschreibung |
|-----------|----------|--------------|
| `chimera.default_adapter` | `"ThemisDB"` | Standard-Adapter wenn keiner angegeben |
| `chimera.connection_timeout_ms` | `5000` | Verbindungs-Timeout in Millisekunden |
| `chimera.query_timeout_ms` | `30000` | Abfrage-Timeout in Millisekunden |

---

## Bekannte Einschränkungen

- **Simulation Mode:** Alle Adapter erfordern für den Produktivbetrieb native
  Client-Bibliotheken (siehe Tabelle oben).
- **Kein Connection Pooling:** Jeder `create()`-Aufruf erzeugt eine neue Verbindung.
- **Nicht implementierte Operationen** (z. B. relationale Abfragen auf Qdrant) liefern
  `AdapterResult` mit `ErrorCode::NOT_IMPLEMENTED`.

---

## Verwandte Dokumentation

| Dokument | Pfad | Beschreibung |
|----------|------|--------------|
| Source README | [src/chimera/README.md](../../../src/chimera/README.md) | Implementierungsdetails aller Adapter |
| Architektur | [src/chimera/ARCHITECTURE.md](../../../src/chimera/ARCHITECTURE.md) | Designprinzipien, Datenfluss, Threading |
| Roadmap | [src/chimera/ROADMAP.md](../../../src/chimera/ROADMAP.md) | Status, geplante Features, DoD-Checkliste |
| Future Enhancements | [src/chimera/FUTURE_ENHANCEMENTS.md](../../../src/chimera/FUTURE_ENHANCEMENTS.md) | Langfristige Erweiterungen |
| Interface-Spezifikation | [include/chimera/README.md](../../../include/chimera/README.md) | Header-Interfaces und Verträge |
