# 🌐 HTTP API Referenz

> **Kategorie:** Core API  
> **Seit Version:** 1.3.0  
> **Status:** ✅ Stable  
> **Aktualisiert:** 22. Dezember 2025

---

## Übersicht

Dieses Dokument bietet eine vollständige Referenz aller HTTP-Endpunkte der ThemisDB REST API. Für maschinell lesbare API-Spezifikationen siehe die [OpenAPI-Spezifikation](../openapi.yaml).

### Basis-URL

```
http://localhost:8765
```

In Produktionsumgebungen sollte HTTPS verwendet werden.

### Authentifizierung

Die meisten Endpunkte erfordern eine Bearer-Token-Authentifizierung:

```
Authorization: Bearer <api_key>
```

API-Keys können über die [Key Management API](#api-key-management) verwaltet werden.

---

## Inhaltsverzeichnis

1. [System & Monitoring](#system--monitoring)
2. [Entities (CRUD)](#entities-crud)
3. [Query & AQL](#query--aql)
4. [Index Management](#index-management)
5. [Graph Operations](#graph-operations)
6. [Vector Search](#vector-search)
7. [Content Management](#content-management)
8. [Cache (Semantic Cache)](#cache-semantic-cache)
9. [LLM Integration](#llm-integration)
10. [Change Data Capture (CDC)](#change-data-capture-cdc)
11. [Transaction Management](#transaction-management)
12. [API Key Management](#api-key-management)
13. [PII Operations](#pii-operations)
14. [Audit & Compliance](#audit--compliance)
15. [Classification & Reports](#classification--reports)
16. [Error Handling](#error-handling)

---

## System & Monitoring

### GET /health

Health Check des Servers.

**Query-Parameter:** Keine

**Antwort (200 OK):**
```json
{
  "status": "healthy",
  "version": "1.0.1",
  "database": "rocksdb",
  "uptime_seconds": 3600
}
```

**Fehlerbehandlung:**
- Bei Ausfall der Datenbank wird Status 503 zurückgegeben

---

### GET /stats

Detaillierte Server- und Datenbankstatistiken.

**Query-Parameter:** Keine

**Antwort (200 OK):**
```json
{
  "server": {
    "uptime_seconds": 3600,
    "total_requests": 12345,
    "total_errors": 42,
    "queries_per_second": 123.45,
    "threads": 8
  },
  "storage": {
    "rocksdb": {
      "block_cache_usage_bytes": 1048576,
      "block_cache_capacity_bytes": 8388608,
      "estimate_num_keys": 100000,
      "estimate_live_data_size_bytes": 52428800,
      "cache_hit_rate_percent": 95.5,
      "bytes_written": 104857600,
      "bytes_read": 209715200
    },
    "raw_stats": "..."
  }
}
```

**Fehlerbehandlung:**
- `500 Internal Server Error`: Bei Fehler beim Abrufen der Statistiken

---

### GET /metrics

Prometheus-Metriken im Text Exposition Format.

**Query-Parameter:** Keine

**Antwort (200 OK):**
```
Content-Type: text/plain

# HELP process_uptime_seconds Process uptime in seconds
# TYPE process_uptime_seconds gauge
process_uptime_seconds 3600

# HELP vccdb_requests_total Total HTTP requests handled
# TYPE vccdb_requests_total counter
vccdb_requests_total 12345

# HELP vccdb_errors_total Total HTTP errors
# TYPE vccdb_errors_total counter
vccdb_errors_total 42
```

**Fehlerbehandlung:**
- `500 Internal Server Error`: Bei Fehler beim Generieren der Metriken

---

## Entities (CRUD)

### GET /entities/{key}

Liest eine Entity anhand des Primärschlüssels.

**Path-Parameter:**
- `key` (string, required): Primärschlüssel im Format `table:pk` (z.B. `users:123`)

**Query-Parameter:** Keine

**Antwort (200 OK):**
```json
{
  "key": "users:123",
  "blob": "{\"name\":\"Alice\",\"age\":30,\"email\":\"alice@example.com\"}"
}
```

**Fehlerbehandlung:**
- `404 Not Found`: Entity existiert nicht
- `400 Bad Request`: Ungültiger Key-Format
- `500 Internal Server Error`: Datenbankfehler

---

### PUT /entities/{key}

Erstellt oder aktualisiert eine Entity (Upsert).

**Path-Parameter:**
- `key` (string, required): Primärschlüssel im Format `table:pk`

**Request Body:**
```json
{
  "blob": "{\"name\":\"Alice\",\"age\":30,\"email\":\"alice@example.com\"}"
}
```

**Antwort (201 Created):**
```json
{
  "success": true,
  "key": "users:123",
  "blob_size": 58
}
```

**Fehlerbehandlung:**
- `400 Bad Request`: Ungültiges Request-Format oder fehlender Blob
- `500 Internal Server Error`: Datenbankfehler

---

### DELETE /entities/{key}

Löscht eine Entity.

**Path-Parameter:**
- `key` (string, required): Primärschlüssel im Format `table:pk`

**Query-Parameter:** Keine

**Antwort (200 OK):**
```json
{
  "success": true,
  "key": "users:123"
}
```

**Fehlerbehandlung:**
- `400 Bad Request`: Ungültiger Key-Format
- `500 Internal Server Error`: Datenbankfehler

---

### POST /entities

Erstellt eine neue Entity (mit Key im Body oder auto-generiert).

**Query-Parameter:** Keine

**Request Body:**
```json
{
  "key": "users:124",
  "blob": "{\"name\":\"Bob\",\"age\":25}"
}
```

Falls `key` fehlt, wird automatisch ein UUID generiert.

**Antwort (201 Created):**
```json
{
  "success": true,
  "key": "users:124",
  "blob_size": 28
}
```

**Fehlerbehandlung:**
- `400 Bad Request`: Ungültiges Request-Format
- `500 Internal Server Error`: Datenbankfehler

---

## Query & AQL

### POST /query

Führt eine Query mit Gleichheits- und Range-Prädikaten aus.

**Query-Parameter:** Keine

**Request Body:**
```json
{
  "table": "users",
  "predicates": [
    { "column": "city", "value": "Berlin" }
  ],
  "range": [
    {
      "column": "age",
      "gte": "25",
      "lte": "35",
      "includeLower": true,
      "includeUpper": true
    }
  ],
  "order_by": {
    "column": "age",
    "desc": false,
    "limit": 10
  },
  "return": "entities",
  "optimize": true,
  "allow_full_scan": false,
  "explain": false
}
```

**Antwort (200 OK) - bei `return: "keys"`:**
```json
{
  "table": "users",
  "count": 42,
  "keys": ["users:123", "users:456", "..."],
  "plan": {
    "mode": "index_optimized",
    "order": [
      { "column": "city", "value": "Berlin" }
    ],
    "estimates": [
      {
        "column": "city",
        "value": "Berlin",
        "estimatedCount": 1000,
        "capped": false
      }
    ]
  }
}
```

**Antwort (200 OK) - bei `return: "entities"`:**
```json
{
  "table": "users",
  "count": 42,
  "entities": [
    "{\"name\":\"Alice\",\"age\":30,\"city\":\"Berlin\"}",
    "{\"name\":\"Bob\",\"age\":28,\"city\":\"Berlin\"}",
    "..."
  ],
  "plan": { "..." }
}
```

**Fehlerbehandlung:**
- `400 Bad Request`: Ungültige Query-Syntax oder fehlende erforderliche Felder
- `501 Not Implemented`: Feature noch nicht implementiert (z.B. Graph/Vector-Platzhalter)
- `500 Internal Server Error`: Query-Ausführungsfehler

---

### POST /query/aql

Führt eine AQL-Query aus (Advanced Query Language).

**Query-Parameter:** Keine

**Request Body:**
```json
{
  "query": "FOR user IN users FILTER user.city == \"Berlin\" SORT user.age ASC LIMIT 10 RETURN user",
  "optimize": true,
  "allow_full_scan": false,
  "explain": false,
  "use_cursor": false,
  "cursor": null
}
```

**Antwort (200 OK) - Standard:**
```json
{
  "table": "users",
  "count": 10,
  "entities": [
    "{\"name\":\"Alice\",\"age\":25,\"city\":\"Berlin\"}",
    "..."
  ],
  "plan": {
    "mode": "index_optimized",
    "let_pre_extracted": false
  },
  "query": "FOR user IN users FILTER user.city == \"Berlin\" SORT user.age ASC LIMIT 10 RETURN user"
}
```

**Antwort (200 OK) - mit Cursor:**
```json
{
  "items": ["...", "..."],
  "batch_size": 10,
  "has_more": true,
  "next_cursor": "eyJvZmZzZXQiOjEwfQ==",
  "plan": { "..." }
}
```

**Fehlerbehandlung:**
- `400 Bad Request`: Ungültige AQL-Syntax
- `500 Internal Server Error`: Query-Ausführungsfehler

**AQL-Features:**
- `FOR ... IN collection`: Iteration über Collection
- `FILTER condition`: Filterung
- `SORT field ASC|DESC`: Sortierung
- `LIMIT n`: Limitierung
- `RETURN expression`: Rückgabe
- `LET var = expression`: Variable definieren
- Einfache Equality-Joins über zwei `FOR`-Klauseln

---

## Index Management

### POST /index/create

Erstellt einen Sekundärindex.

**Query-Parameter:** Keine

**Request Body:**
```json
{
  "table": "users",
  "column": "email",
  "type": "equality",
  "unique": true
}
```

**Index-Typen:**
- `equality`: Gleichheitsfilter (Standard)
- `range`: Range-Prädikate und ORDER BY
- `fulltext`: Volltextsuche mit BM25

**Fulltext-Konfiguration (optional):**
```json
{
  "table": "docs",
  "column": "text",
  "type": "fulltext",
  "config": {
    "stemming_enabled": true,
    "language": "de",
    "stopwords_enabled": true,
    "normalize_umlauts": true,
    "stopwords": ["foo", "bar"]
  }
}
```

**Antwort (200 OK):**
```json
{
  "success": true,
  "table": "users",
  "column": "email"
}
```

**Fehlerbehandlung:**
- `400 Bad Request`: Ungültige Parameter oder Index existiert bereits
- `500 Internal Server Error`: Index-Erstellungsfehler

---

### POST /index/drop

Löscht einen Sekundärindex.

**Query-Parameter:** Keine

**Request Body:**
```json
{
  "table": "users",
  "column": "email",
  "type": "equality"
}
```

**Antwort (200 OK):**
```json
{
  "success": true,
  "table": "users",
  "column": "email"
}
```

**Fehlerbehandlung:**
- `400 Bad Request`: Index existiert nicht
- `500 Internal Server Error`: Fehler beim Löschen

---

## Graph Operations

### POST /graph/traverse

Führt eine BFS-Traversierung im Graph aus.

**Query-Parameter:** Keine

**Request Body:**
```json
{
  "start_vertex": "user1",
  "max_depth": 3
}
```

**Antwort (200 OK):**
```json
{
  "start_vertex": "user1",
  "max_depth": 3,
  "visited_count": 15,
  "visited": ["user1", "user2", "user3", "..."]
}
```

**Fehlerbehandlung:**
- `400 Bad Request`: Ungültige Parameter
- `500 Internal Server Error`: Traversierungsfehler

**Hinweis:** Graph-Kanten müssen als Entities mit Feldern `id`, `_from`, `_to` gespeichert sein.

---

## Vector Search

### POST /vector/search

Führt eine k-NN Vektorsuche aus.

**Query-Parameter:** Keine

**Request Body:**
```json
{
  "vector": [0.1, 0.2, 0.3, 0.4],
  "k": 10,
  "use_cursor": false,
  "cursor": null
}
```

**Antwort (200 OK) - Legacy-Format:**
```json
{
  "results": [
    { "pk": "doc1", "distance": 0.234 },
    { "pk": "doc2", "distance": 0.456 }
  ],
  "k": 10,
  "count": 10
}
```

**Antwort (200 OK) - mit Cursor:**
```json
{
  "items": [
    { "pk": "doc1", "distance": 0.234 }
  ],
  "batch_size": 10,
  "has_more": true,
  "next_cursor": "eyJvZmZzZXQiOjEwfQ=="
}
```

**Fehlerbehandlung:**
- `400 Bad Request`: Ungültige Vektor-Dimension oder Parameter
- `500 Internal Server Error`: Such-Fehler

---

### POST /vector/batch_insert

Fügt mehrere Vektoren im Batch ein.

**Query-Parameter:** Keine

**Request Body:**
```json
{
  "vector_field": "embedding",
  "items": [
    {
      "pk": "doc1",
      "vector": [0.1, 0.2, 0.3],
      "fields": { "title": "Document 1" }
    },
    {
      "pk": "doc2",
      "vector": [0.4, 0.5, 0.6],
      "fields": { "title": "Document 2" }
    }
  ]
}
```

**Antwort (200 OK):**
```json
{
  "inserted": 2,
  "errors": 0,
  "objectName": "vector_index",
  "dimension": 3
}
```

**Fehlerbehandlung:**
- `400 Bad Request`: Ungültige Vektor-Dimension
- `500 Internal Server Error`: Einfügefehler

---

### DELETE /vector/by-filter

Löscht Vektoren basierend auf Filter (PKs oder Prefix).

**Query-Parameter:** Keine

**Request Body (per PKs):**
```json
{
  "pks": ["doc1", "doc2", "doc3"]
}
```

**Request Body (per Prefix):**
```json
{
  "prefix": "doc:"
}
```

**Antwort (200 OK):**
```json
{
  "deleted": 42,
  "method": "pks",
  "prefix": null
}
```

**Fehlerbehandlung:**
- `400 Bad Request`: Weder `pks` noch `prefix` angegeben
- `500 Internal Server Error`: Löschfehler

---

### GET /vector/index/config

Gibt die aktuelle Vektorindex-Konfiguration zurück.

**Query-Parameter:** Keine

**Antwort (200 OK):**
```json
{
  "objectName": "vector_index",
  "dimension": 768,
  "metric": "COSINE",
  "efSearch": 64,
  "M": 16,
  "efConstruction": 200,
  "hnswEnabled": true
}
```

**Fehlerbehandlung:**
- `500 Internal Server Error`: Fehler beim Abrufen der Konfiguration

---

### PUT /vector/index/config

Aktualisiert die Vektorindex-Konfiguration.

**Query-Parameter:** Keine

**Request Body:**
```json
{
  "efSearch": 128
}
```

**Antwort (200 OK):**
```json
{
  "message": "Configuration updated",
  "updated_fields": {
    "efSearch": 128
  }
}
```

**Fehlerbehandlung:**
- `400 Bad Request`: Ungültige Werte (efSearch muss 1-10000 sein)
- `500 Internal Server Error`: Aktualisierungsfehler

---

### GET /vector/index/stats

Gibt Statistiken zum Vektorindex zurück.

**Query-Parameter:** Keine

**Antwort (200 OK):**
```json
{
  "objectName": "vector_index",
  "dimension": 768,
  "metric": "COSINE",
  "efSearch": 64,
  "M": 16,
  "efConstruction": 200,
  "hnswEnabled": true,
  "vectorCount": 100000
}
```

**Fehlerbehandlung:**
- `500 Internal Server Error`: Fehler beim Abrufen der Statistiken

---

## Content Management

### POST /content/import

Importiert Content (v0 Bulk).

**Query-Parameter:** Keine

**Request Body:**
```json
{
  "content": {
    "id": "doc123",
    "mime_type": "application/pdf",
    "category": 0,
    "original_filename": "document.pdf",
    "size_bytes": 1024000
  },
  "chunks": [
    {
      "id": "chunk1",
      "content_id": "doc123",
      "seq_num": 0,
      "text": "Sample text...",
      "embedding": [0.1, 0.2, 0.3]
    }
  ],
  "blob": "base64_encoded_content"
}
```

**Antwort (200 OK):**
```json
{
  "status": "ok",
  "content_id": "doc123"
}
```

**Fehlerbehandlung:**
- `400 Bad Request`: Ungültiges Request-Format
- `500 Internal Server Error`: Import-Fehler

---

### GET /content/{id}

Liest Content-Metadaten.

**Path-Parameter:**
- `id` (string, required): Content-ID

**Query-Parameter:** Keine

**Antwort (200 OK):**
```json
{
  "id": "doc123",
  "mime_type": "application/pdf",
  "category": 0,
  "original_filename": "document.pdf",
  "size_bytes": 1024000,
  "created_at": 1702400000000,
  "hash_sha256": "abc123...",
  "chunk_count": 10
}
```

**Fehlerbehandlung:**
- `404 Not Found`: Content existiert nicht
- `500 Internal Server Error`: Fehler beim Abrufen

---

### GET /content/{id}/chunks

Listet alle Chunks eines Contents auf.

**Path-Parameter:**
- `id` (string, required): Content-ID

**Query-Parameter:** Keine

**Antwort (200 OK):**
```json
{
  "count": 10,
  "chunks": [
    {
      "id": "chunk1",
      "content_id": "doc123",
      "seq_num": 0,
      "text": "Sample text...",
      "embedding": [0.1, 0.2, 0.3]
    }
  ]
}
```

**Fehlerbehandlung:**
- `404 Not Found`: Content existiert nicht
- `500 Internal Server Error`: Fehler beim Abrufen

---

### GET /content/{id}/blob

Gibt den Original-Blob eines Contents zurück.

**Path-Parameter:**
- `id` (string, required): Content-ID

**Query-Parameter:** Keine

**Antwort (200 OK):**
```
Content-Type: application/pdf (aus Metadaten)

[Binary Content]
```

**Fehlerbehandlung:**
- `404 Not Found`: Content existiert nicht
- `500 Internal Server Error`: Fehler beim Abrufen

---

## Cache (Semantic Cache)

### POST /cache/query

Führt eine semantische Cache-Abfrage aus.

**Query-Parameter:** Keine

**Request Body:**
```json
{
  "key": "rag:question:hash123",
  "top_k": 1
}
```

**Oder mit Embedding:**
```json
{
  "prompt": "Wie skaliere ich HNSW efSearch?",
  "embedding": [0.01, 0.12, 0.0],
  "top_k": 3
}
```

**Antwort (200 OK) - Treffer:**
```json
{
  "hit": true,
  "hits": [
    {
      "key": "rag:question:hash123",
      "value": { "answer": "Nutze ef_search 64–128 für Latenz/Qualität." },
      "score": 0.95,
      "metadata": { "model": "gpt-4o" }
    }
  ]
}
```

**Antwort (200 OK) - Kein Treffer:**
```json
{
  "hit": false,
  "hits": []
}
```

**Fehlerbehandlung:**
- `400 Bad Request`: Ungültige Parameter
- `500 Internal Server Error`: Cache-Fehler

---

### POST /cache/put

Speichert einen Eintrag im Semantic Cache.

**Query-Parameter:** Keine

**Request Body:**
```json
{
  "key": "rag:question:hash123",
  "value": { "answer": "Nutze ef_search 64–128 für Latenz/Qualität." },
  "embedding": [0.01, 0.12, 0.0],
  "ttl_sec": 3600,
  "metadata": { "model": "gpt-4o", "topic": "vecsearch" }
}
```

**Antwort (200 OK):**
```json
{
  "success": true,
  "key": "rag:question:hash123",
  "expires_at": "2025-12-10T19:00:00Z"
}
```

**Fehlerbehandlung:**
- `400 Bad Request`: Fehlende erforderliche Felder
- `500 Internal Server Error`: Speicherfehler

---

### GET /cache/stats

Gibt Cache-Statistiken zurück.

**Query-Parameter:** Keine

**Antwort (200 OK):**
```json
{
  "entries": 10000,
  "hit_ratio_1m": 0.85,
  "hit_ratio_1h": 0.78,
  "evictions_total": 42,
  "bytes": 1048576
}
```

**Fehlerbehandlung:**
- `500 Internal Server Error`: Fehler beim Abrufen der Statistiken

---

## LLM Integration

### POST /llm/interaction

Speichert eine LLM-Interaktion.

**Query-Parameter:** Keine

**Request Body:**
```json
{
  "model": "gpt-4o",
  "messages": [
    { "role": "user", "content": "Erkläre MVCC kurz" },
    { "role": "assistant", "content": "MVCC erlaubt paralleles Lesen/Schreiben..." }
  ],
  "reasoning_steps": [
    {
      "type": "chain_of_thought",
      "content": ["MVCC erlaubt paralleles Lesen/Schreiben", "Versionen pro Tx"]
    }
  ],
  "metadata": { "session_id": "abc" }
}
```

**Antwort (201 Created):**
```json
{
  "id": "interaction123",
  "success": true
}
```

**Fehlerbehandlung:**
- `400 Bad Request`: Ungültiges Request-Format
- `500 Internal Server Error`: Speicherfehler

---

### GET /llm/interaction

Listet LLM-Interaktionen auf.

**Query-Parameter:**
- `limit` (integer, optional): Max. Anzahl (Standard: 50)
- `after` (string, optional): Cursor/ID für Pagination

**Antwort (200 OK):**
```json
{
  "items": [
    {
      "id": "interaction123",
      "created_at": "2025-12-10T18:00:00Z",
      "model": "gpt-4o",
      "messages": [...],
      "reasoning_steps": [...],
      "metadata": {}
    }
  ],
  "next": "interaction124"
}
```

**Fehlerbehandlung:**
- `500 Internal Server Error`: Fehler beim Abrufen

---

### GET /llm/interaction/{id}

Liest eine spezifische LLM-Interaktion.

**Path-Parameter:**
- `id` (string, required): Interaktions-ID

**Query-Parameter:** Keine

**Antwort (200 OK):**
```json
{
  "id": "interaction123",
  "created_at": "2025-12-10T18:00:00Z",
  "model": "gpt-4o",
  "messages": [...],
  "reasoning_steps": [...],
  "metadata": {}
}
```

**Fehlerbehandlung:**
- `404 Not Found`: Interaktion existiert nicht
- `500 Internal Server Error`: Fehler beim Abrufen

---

## Change Data Capture (CDC)

### GET /changefeed

Liefert Änderungen ab einer Sequenznummer (Long-Polling).

**Query-Parameter:**
- `from_seq` (integer, required): Start-Sequenznummer (exklusiv)
- `limit` (integer, optional): Max. Anzahl (Standard: 100)
- `long_poll_ms` (integer, optional): Long-Polling-Timeout in ms (Standard: 0, max: 60000)

**Antwort (200 OK):**
```json
{
  "from_seq": 100,
  "next_seq": 150,
  "entries": [
    {
      "seq": 101,
      "ts": "2025-12-10T18:00:00Z",
      "op": "put",
      "key": "users:123",
      "table": "users",
      "value": { "name": "Alice" },
      "prev": null,
      "txn_id": "tx123"
    }
  ]
}
```

**Fehlerbehandlung:**
- `400 Bad Request`: Ungültige Parameter
- `500 Internal Server Error`: CDC-Fehler

---

### GET /changefeed/stream

Streamt Änderungen in Server-Sent Events (SSE) Format.

**Query-Parameter:**
- `from_seq` (integer, optional): Start-Sequenznummer (Standard: 0)
- `key_prefix` (string, optional): Filtert Events nach Key-Präfix
- `keep_alive` (boolean, optional): Verbindung offen halten (Standard: true)
- `max_seconds` (integer, optional): Max. Stream-Dauer 1-60s (Standard: 30)
- `heartbeat_ms` (integer, optional): Heartbeat-Intervall (min: 100ms, nur für Tests)
- `consumer_id` (string, optional): Opaque Consumer-Identifier für At-least-once-Delivery. Wird ein Consumer-ID übergeben, werden unquittierte Events aus früheren Requests **vor** neuen Events wiederholt zugestellt. Maximallänge: 128 Zeichen.
- `ack_timeout_ms` (integer, optional): Override für den ACK-Timeout dieses Requests in Millisekunden (Standard: 30000 ms). Werte ≥ 0 sind gültig. Nützlich für Tests mit kurzen Timeouts.

**Antwort (200 OK):**
```
Content-Type: text/event-stream

id: 42
data: {"operation":"PUT","key":"doc:1","sequence":42,"old_val":null,"new_val":"{\"x\":1}"}

id: 43
data: {"operation":"DELETE","key":"doc:2","sequence":43,"old_val":"{\"y\":2}","new_val":null}

: heartbeat
```

**At-least-once Delivery (Reconnect-Protokoll):**
```
# 1. Client verbindet sich mit consumer_id
GET /changefeed/stream?from_seq=0&consumer_id=my-service-uuid

# 2. Server liefert Events mit id-Zeilen
# Client verarbeitet Events und quittiert via POST /changefeed/stream/ack

# 3. Bei Verbindungsabbruch: Reconnect mit Last-Event-ID + consumer_id
GET /changefeed/stream?from_seq=0&consumer_id=my-service-uuid
Last-Event-ID: 42

# Server liefert unquittierte Events erneut (redelivery) vor neuen Events
```

**Fehlerbehandlung:**
- `400 Bad Request`: Ungültige Parameter
- `500 Internal Server Error`: Streaming-Fehler

**Hinweis:** Dieser Endpoint ist nicht Teil der OpenAPI-Spezifikation, da er `text/event-stream` zurückliefert.

---

### POST /changefeed/stream/ack

Quittiert den Empfang von SSE-Events für einen Consumer im At-least-once-Delivery-Protokoll.
Entfernt alle Events mit `sequence ≤ up_to_sequence` aus dem In-flight-Tracking des Consumers.

**Request Body:**
```json
{
  "consumer_id": "my-service-uuid",
  "up_to_sequence": 42
}
```

| Feld | Typ | Pflicht | Beschreibung |
|------|-----|---------|--------------|
| `consumer_id` | string | ✅ | Derselbe Identifier wie beim GET-Request |
| `up_to_sequence` | uint64 | ✅ | Höchste Sequenznummer, die quittiert werden soll (inklusiv, kumulativ) |

**Antwort (200 OK):**
```json
{
  "consumer_id": "my-service-uuid",
  "up_to_sequence": 42,
  "acknowledged": 3
}
```

| Feld | Beschreibung |
|------|--------------|
| `acknowledged` | Anzahl der Events, die aus dem In-flight-Tracking entfernt wurden |

**Fehlerbehandlung:**
- `400 Bad Request`: Fehlendes / leeres `consumer_id` oder `up_to_sequence`; ungültiges JSON
- `404 Not Found`: Feature `cdc` deaktiviert
- `500 Internal Server Error`: Interner Fehler

---

## Transaction Management

### POST /transaction/begin

Startet eine neue Transaktion.

**Query-Parameter:** Keine

**Request Body:**
```json
{
  "isolation": "snapshot"
}
```

**Isolation Levels:**
- `read_committed` (Standard)
- `snapshot`

**Antwort (200 OK):**
```json
{
  "transaction_id": 42,
  "isolation": "snapshot",
  "status": "begun"
}
```

**Fehlerbehandlung:**
- `500 Internal Server Error`: Fehler beim Starten

---

### POST /transaction/commit

Committet eine Transaktion.

**Query-Parameter:** Keine

**Request Body:**
```json
{
  "transaction_id": 42
}
```

**Antwort (200 OK):**
```json
{
  "transaction_id": 42,
  "status": "committed",
  "message": "Transaction committed successfully"
}
```

**Fehlerbehandlung:**
- `400 Bad Request`: Transaktion nicht gefunden oder bereits abgeschlossen
- `500 Internal Server Error`: Commit fehlgeschlagen (automatisch rollback)

---

### POST /transaction/rollback

Rollt eine Transaktion zurück.

**Query-Parameter:** Keine

**Request Body:**
```json
{
  "transaction_id": 42
}
```

**Antwort (200 OK):**
```json
{
  "transaction_id": 42,
  "status": "rolled_back",
  "message": "Transaction rolled back successfully"
}
```

**Fehlerbehandlung:**
- `400 Bad Request`: Transaktion nicht gefunden
- `500 Internal Server Error`: Rollback-Fehler

---

### GET /transaction/stats

Gibt Transaktionsstatistiken zurück.

**Query-Parameter:** Keine

**Antwort (200 OK):**
```json
{
  "total_begun": 1523,
  "total_committed": 1401,
  "total_aborted": 122,
  "active_count": 3,
  "avg_duration_ms": 45,
  "max_duration_ms": 523,
  "success_rate": 0.92
}
```

**Fehlerbehandlung:**
- `500 Internal Server Error`: Fehler beim Abrufen der Statistiken

---

## API Key Management

### POST /api/keys

Erstellt einen neuen API-Key.

**Query-Parameter:** Keine

**Authentifizierung:** Erforderlich (admin-Berechtigung)

**Request Body:**
```json
{
  "name": "Analytics Service Key",
  "permissions": ["audit:read", "stats:read"],
  "expires_in_days": 90,
  "metadata": {
    "service": "internal-admin",
    "environment": "production"
  }
}
```

**Antwort (201 Created):**
```json
{
  "keyId": "key123",
  "secret": "sk_live_abc123...",
  "name": "Analytics Service Key",
  "permissions": ["audit:read", "stats:read"],
  "created_at": "2025-12-10T18:00:00Z",
  "expires_at": "2026-03-10T18:00:00Z"
}
```

**Hinweis:** Der `secret` wird nur einmal zurückgegeben!

**Fehlerbehandlung:**
- `400 Bad Request`: Ungültige Parameter
- `401 Unauthorized`: Nicht authentifiziert
- `403 Forbidden`: Keine admin-Berechtigung
- `500 Internal Server Error`: Fehler beim Erstellen

---

### GET /api/keys

Listet alle API-Keys auf.

**Query-Parameter:**
- `include_revoked` (boolean, optional): Auch widerrufene Keys anzeigen (Standard: false)
- `page` (integer, optional): Seitennummer (Standard: 1)
- `page_size` (integer, optional): Einträge pro Seite (Standard: 50, max: 100)

**Authentifizierung:** Erforderlich (admin-Berechtigung)

**Antwort (200 OK):**
```json
{
  "keys": [
    {
      "keyId": "key123",
      "name": "Analytics Service Key",
      "permissions": ["audit:read", "stats:read"],
      "created_at": "2025-12-10T18:00:00Z",
      "expires_at": "2026-03-10T18:00:00Z",
      "last_used_at": "2025-12-10T19:00:00Z",
      "revoked": false,
      "revoked_at": null
    }
  ],
  "total": 42,
  "page": 1,
  "page_size": 50
}
```

**Fehlerbehandlung:**
- `401 Unauthorized`: Nicht authentifiziert
- `403 Forbidden`: Keine admin-Berechtigung
- `500 Internal Server Error`: Fehler beim Abrufen

---

### GET /api/keys/{keyId}

Gibt Details zu einem spezifischen API-Key zurück.

**Path-Parameter:**
- `keyId` (string, required): Key-ID

**Query-Parameter:** Keine

**Authentifizierung:** Erforderlich (admin-Berechtigung)

**Antwort (200 OK):**
```json
{
  "keyId": "key123",
  "name": "Analytics Service Key",
  "permissions": ["audit:read", "stats:read"],
  "created_at": "2025-12-10T18:00:00Z",
  "expires_at": "2026-03-10T18:00:00Z",
  "last_used_at": "2025-12-10T19:00:00Z",
  "revoked": false,
  "revoked_at": null,
  "metadata": {
    "service": "internal-admin"
  }
}
```

**Fehlerbehandlung:**
- `404 Not Found`: Key existiert nicht
- `401 Unauthorized`: Nicht authentifiziert
- `403 Forbidden`: Keine admin-Berechtigung
- `500 Internal Server Error`: Fehler beim Abrufen

---

### PUT /api/keys/{keyId}

Aktualisiert einen API-Key (Name, Permissions, Metadata).

**Path-Parameter:**
- `keyId` (string, required): Key-ID

**Query-Parameter:** Keine

**Authentifizierung:** Erforderlich (admin-Berechtigung)

**Request Body:**
```json
{
  "name": "Updated Analytics Key",
  "permissions": ["audit:read", "stats:read", "reports:read"],
  "metadata": {
    "service": "internal-admin",
    "updated": true
  }
}
```

**Antwort (200 OK):**
```json
{
  "keyId": "key123",
  "name": "Updated Analytics Key",
  "permissions": ["audit:read", "stats:read", "reports:read"],
  "created_at": "2025-12-10T18:00:00Z",
  "expires_at": "2026-03-10T18:00:00Z",
  "revoked": false,
  "metadata": {
    "service": "internal-admin",
    "updated": true
  }
}
```

**Fehlerbehandlung:**
- `400 Bad Request`: Ungültige Parameter
- `404 Not Found`: Key existiert nicht
- `401 Unauthorized`: Nicht authentifiziert
- `403 Forbidden`: Keine admin-Berechtigung
- `500 Internal Server Error`: Fehler beim Aktualisieren

---

### DELETE /api/keys/{keyId}

Widerruft einen API-Key permanent.

**Path-Parameter:**
- `keyId` (string, required): Key-ID

**Query-Parameter:** Keine

**Authentifizierung:** Erforderlich (admin-Berechtigung)

**Antwort (200 OK):**
```json
{
  "success": true,
  "keyId": "key123",
  "revoked_at": "2025-12-10T20:00:00Z"
}
```

**Fehlerbehandlung:**
- `404 Not Found`: Key existiert nicht
- `401 Unauthorized`: Nicht authentifiziert
- `403 Forbidden`: Keine admin-Berechtigung
- `500 Internal Server Error`: Fehler beim Widerrufen

---

## PII Operations

### GET /pii/{uuid}

Gibt den Originalwert zu einer pseudonymisierten UUID zurück.

**Path-Parameter:**
- `uuid` (string, required): PII-UUID

**Query-Parameter:** Keine

**Authentifizierung:** Erforderlich

**Antwort (200 OK):**
```json
{
  "uuid": "550e8400-e29b-41d4-a716-446655440000",
  "value": "max.mustermann@example.com"
}
```

**Fehlerbehandlung:**
- `404 Not Found`: UUID existiert nicht
- `401 Unauthorized`: Nicht authentifiziert
- `403 Forbidden`: Keine Berechtigung
- `500 Internal Server Error`: Fehler beim Abrufen

---

### DELETE /pii/{uuid}

Löscht PII-Daten (soft oder hard delete).

**Path-Parameter:**
- `uuid` (string, required): PII-UUID

**Query-Parameter:**
- `mode` (string, optional): `soft` oder `hard` (Standard: `soft`)

**Authentifizierung:** Erforderlich

**Antwort (200 OK) - Soft Delete:**
```json
{
  "status": "ok",
  "mode": "soft",
  "uuid": "550e8400-e29b-41d4-a716-446655440000",
  "updated": true
}
```

**Antwort (200 OK) - Hard Delete:**
```json
{
  "status": "ok",
  "mode": "hard",
  "uuid": "550e8400-e29b-41d4-a716-446655440000",
  "deleted": true
}
```

**Fehlerbehandlung:**
- `401 Unauthorized`: Nicht authentifiziert
- `403 Forbidden`: Keine Berechtigung
- `500 Internal Server Error`: Fehler beim Löschen

---

## Audit & Compliance

### GET /api/audit

Fragt Audit-Logs ab.

**Query-Parameter:**
- `start` (string, optional): Start-Zeitpunkt (ms seit Epoch oder ISO 8601)
- `end` (string, optional): End-Zeitpunkt (ms seit Epoch oder ISO 8601)
- `user` (string, optional): Filter nach Benutzer
- `action` (string, optional): Filter nach Aktion
- `entity_type` (string, optional): Filter nach Entity-Typ
- `entity_id` (string, optional): Filter nach Entity-ID
- `success` (boolean, optional): Nur erfolgreiche Einträge (Standard: false)
- `page` (integer, optional): Seitennummer (Standard: 1)
- `page_size` (integer, optional): Einträge pro Seite (Standard: 100, max: 1000)

**Authentifizierung:** Erforderlich (audit:read)

**Antwort (200 OK):**
```json
{
  "entries": [
    {
      "id": 1,
      "timestamp": "2025-12-10T18:00:00Z",
      "user": "admin",
      "action": "CREATE_KEY",
      "entityType": "api_key",
      "entityId": "key123",
      "oldValue": null,
      "newValue": "{\"name\":\"Analytics Key\"}",
      "success": true,
      "ipAddress": "192.168.1.1",
      "sessionId": "session123",
      "errorMessage": null
    }
  ],
  "totalCount": 1000,
  "page": 1,
  "pageSize": 100,
  "hasMore": true
}
```

**Fehlerbehandlung:**
- `429 Too Many Requests`: Rate-Limit erreicht (Header: `Retry-After`, `X-RateLimit-Limit`, `X-RateLimit-Remaining`)
- `401 Unauthorized`: Nicht authentifiziert
- `403 Forbidden`: Keine audit:read-Berechtigung
- `500 Internal Server Error`: Fehler beim Abrufen

---

### GET /api/audit/export/csv

Exportiert Audit-Logs als CSV.

**Query-Parameter:**
- Gleiche wie `/api/audit`
- `page_size` max: 10000 (für Export)

**Authentifizierung:** Erforderlich (audit:read)

**Antwort (200 OK):**
```
Content-Type: text/csv

id,timestamp,user,action,entityType,entityId,success
1,2025-12-10T18:00:00Z,admin,CREATE_KEY,api_key,key123,true
...
```

**Fehlerbehandlung:**
- Gleich wie `/api/audit`

---

## Classification & Reports

### POST /api/classify

Klassifiziert Text auf personenbezogene Daten (PII).

**Query-Parameter:** Keine

**Authentifizierung:** Erforderlich (classify:read)

**Request Body:**
```json
{
  "text": "Kontakt: Max Mustermann, max@example.com, +49 123 456789",
  "include_positions": true
}
```

**Antwort (200 OK):**
```json
{
  "contains_pii": true,
  "matches": [
    {
      "category": "name",
      "value": "Max Mustermann",
      "confidence": 0.95,
      "start_pos": 9,
      "end_pos": 23
    },
    {
      "category": "email",
      "value": "max@example.com",
      "confidence": 1.0,
      "start_pos": 25,
      "end_pos": 40
    },
    {
      "category": "phone",
      "value": "+49 123 456789",
      "confidence": 0.98,
      "start_pos": 42,
      "end_pos": 56
    }
  ],
  "summary": {
    "total_matches": 3,
    "categories_found": ["name", "email", "phone"],
    "risk_level": "high"
  }
}
```

**Erkannte Kategorien:**
- `email`: E-Mail-Adressen
- `phone`: Telefonnummern
- `iban`: IBAN
- `credit_card`: Kreditkartennummern
- `name`: Namen (DE/EN)
- `address`: Adressen (DE/EN)

**Fehlerbehandlung:**
- `400 Bad Request`: Ungültiges Request-Format
- `401 Unauthorized`: Nicht authentifiziert
- `403 Forbidden`: Keine classify:read-Berechtigung
- `500 Internal Server Error`: Klassifizierungsfehler

---

### GET /api/classify

Gibt die aktuellen Klassifizierungs-Regeln zurück.

**Query-Parameter:** Keine

**Authentifizierung:** Erforderlich (classify:read)

**Antwort (200 OK):**
```json
{
  "rules": [
    {
      "category": "email",
      "pattern": "[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}",
      "enabled": true,
      "confidence": 1.0
    }
  ],
  "nlp_enabled": true,
  "nlp_model": "de_core_news_sm",
  "supported_languages": ["de", "en"]
}
```

**Fehlerbehandlung:**
- `401 Unauthorized`: Nicht authentifiziert
- `403 Forbidden`: Keine classify:read-Berechtigung
- `500 Internal Server Error`: Fehler beim Abrufen

---

### GET /api/reports/audit

Generiert einen aggregierten Audit-Report.

**Query-Parameter:**
- `start` (string, optional): Start-Zeitpunkt (ISO 8601 oder Epoch ms)
- `end` (string, optional): End-Zeitpunkt (ISO 8601 oder Epoch ms)
- `format` (string, optional): `json`, `csv` oder `pdf` (Standard: `json`)
- `group_by` (string, optional): `user`, `action`, `entity_type`, `hour`, `day` (Standard: `action`)

**Authentifizierung:** Erforderlich (audit:read, reports:read)

**Antwort (200 OK):**
```json
{
  "period": {
    "start": "2025-12-01T00:00:00Z",
    "end": "2025-12-10T23:59:59Z"
  },
  "summary": {
    "total_events": 10000,
    "successful_events": 9500,
    "failed_events": 500,
    "success_rate": 0.95,
    "unique_users": 42,
    "unique_entities": 1234
  },
  "top_actions": [
    {
      "action": "QUERY",
      "count": 5000,
      "success_rate": 0.98
    }
  ],
  "top_users": [
    {
      "user": "admin",
      "action_count": 1000,
      "failed_count": 10
    }
  ],
  "timeline": [
    {
      "timestamp": "2025-12-01T00:00:00Z",
      "event_count": 100
    }
  ]
}
```

**Fehlerbehandlung:**
- `400 Bad Request`: Ungültige Parameter
- `401 Unauthorized`: Nicht authentifiziert
- `403 Forbidden`: Fehlende Berechtigungen
- `500 Internal Server Error`: Report-Generierungsfehler

---

### GET /api/reports/compliance

Generiert einen Compliance-Report (DSGVO/Datenschutz).

**Query-Parameter:**
- `start` (string, optional): Start-Zeitpunkt
- `end` (string, optional): End-Zeitpunkt
- `format` (string, optional): `json` oder `pdf` (Standard: `json`)

**Authentifizierung:** Erforderlich (reports:read, audit:read)

**Antwort (200 OK):**
```json
{
  "period": {
    "start": "2025-12-01T00:00:00Z",
    "end": "2025-12-10T23:59:59Z"
  },
  "pii_processing": {
    "total_pii_records": 50000,
    "pii_access_events": 1000,
    "pii_deletion_events": 42,
    "pseudonymization_active": true,
    "encryption_enabled": true
  },
  "retention": {
    "policies_active": 5,
    "auto_deletion_enabled": true,
    "records_deleted_in_period": 500
  },
  "security": {
    "failed_auth_attempts": 10,
    "unauthorized_access_attempts": 5,
    "data_breaches_detected": 0
  },
  "data_subject_rights": {
    "access_requests": 15,
    "deletion_requests": 8,
    "rectification_requests": 3,
    "portability_requests": 2
  },
  "compliance_score": 92.5
}
```

**Fehlerbehandlung:**
- `400 Bad Request`: Ungültige Parameter
- `401 Unauthorized`: Nicht authentifiziert
- `403 Forbidden`: Fehlende Berechtigungen
- `500 Internal Server Error`: Report-Generierungsfehler

---

### GET /api/reports/retention

Gibt den Status der Retention Policies zurück.

**Query-Parameter:**
- `include_stats` (boolean, optional): Detaillierte Statistiken (Standard: true)

**Authentifizierung:** Erforderlich (reports:read)

**Antwort (200 OK):**
```json
{
  "policies": [
    {
      "collection": "audit_logs",
      "retention_days": 365,
      "auto_deletion_enabled": true,
      "last_cleanup": "2025-12-10T02:00:00Z",
      "records_total": 1000000,
      "records_expired": 5000,
      "oldest_record_age_days": 400
    }
  ],
  "summary": {
    "total_collections": 10,
    "collections_with_policy": 5,
    "total_expired_records": 10000,
    "auto_deletion_active": true,
    "last_global_cleanup": "2025-12-10T02:00:00Z"
  }
}
```

**Fehlerbehandlung:**
- `401 Unauthorized`: Nicht authentifiziert
- `403 Forbidden`: Keine reports:read-Berechtigung
- `500 Internal Server Error`: Fehler beim Abrufen

---

## Error Handling

### Standard-Fehlerformat

Alle Fehler folgen diesem Format:

```json
{
  "error": true,
  "message": "Beschreibung des Fehlers",
  "status_code": 400
}
```

### HTTP-Statuscodes

| Code | Bedeutung | Verwendung |
|------|-----------|------------|
| `200 OK` | Erfolg | Erfolgreiche GET/PUT/DELETE Requests |
| `201 Created` | Erstellt | Erfolgreiche POST Requests (Ressource erstellt) |
| `400 Bad Request` | Ungültige Anfrage | Ungültige Parameter oder Request-Format |
| `401 Unauthorized` | Nicht authentifiziert | Fehlende oder ungültige Authentifizierung |
| `403 Forbidden` | Verboten | Fehlende Berechtigung für die Aktion |
| `404 Not Found` | Nicht gefunden | Ressource existiert nicht |
| `429 Too Many Requests` | Rate-Limit | Zu viele Anfragen (siehe Header) |
| `500 Internal Server Error` | Server-Fehler | Interner Fehler bei Verarbeitung |
| `501 Not Implemented` | Nicht implementiert | Feature noch nicht verfügbar |

### Rate-Limiting

Bei Rate-Limit-Überschreitung (429) werden zusätzliche Header gesendet:

```
Retry-After: 60
X-RateLimit-Limit: 100
X-RateLimit-Remaining: 0
```

**Response:**
```json
{
  "error": true,
  "message": "Rate limit exceeded",
  "status_code": 429
}
```

---

## Anhang

### Berechtigungen

Übersicht der verfügbaren Berechtigungen:

- `admin`: Volle Administrative Rechte
- `audit:read`: Audit-Logs lesen
- `stats:read`: Statistiken abrufen
- `query:execute`: Queries ausführen
- `classify:read`: PII-Klassifizierung
- `reports:read`: Reports generieren
- `*`: Alle Berechtigungen (nur für Admin-Keys)

### Weitere Ressourcen

- [OpenAPI-Spezifikation](../openapi.yaml) - Vollständige maschinell lesbare API-Spec
- [AQL Dokumentation](../aql/README.md) - Query Language Referenz
- [Server Module](../server/README.md) - HTTP Server Implementierung
- [Authentication Guide](../auth/README.md) - Authentifizierung & Autorisierung

---

**Version:** 1.0.1  
**Letzte Aktualisierung:** 10. Dezember 2025  
**Lizenz:** Siehe [LICENSE](../../LICENSE)
