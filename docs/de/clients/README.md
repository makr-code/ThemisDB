# 💻 Client SDKs Dokumentation

<!-- Dokumentations-Metadaten -->
**Kategorie:** 💻 Client SDKs  
**Version:** v1.3.0  
**Status:** ✅ Produktionsreif  
**Letztes Update:** 22. Dezember 2025

---

## 📑 Inhaltsverzeichnis

- [📋 Übersicht](#-übersicht)
- [✨ Verfügbare SDKs](#-verfügbare-sdks)
- [🚀 Schnellstart](#-schnellstart)
- [📖 Detaillierte Dokumentation](#-detaillierte-dokumentation)
- [💡 Best Practices](#-best-practices)
- [🔧 Troubleshooting](#-troubleshooting)
- [📚 Siehe auch](#-siehe-auch)
- [📝 Changelog](#-changelog)

---

## 📋 Übersicht

ThemisDB bietet vollständige Client SDKs für die wichtigsten Programmiersprachen. Alle SDKs unterstützen die gesamte API-Funktionalität und werden aktiv gewartet.

---

## ✨ Verfügbare SDKs

| SDK | Version | Status | Features | Repository |
|-----|---------|--------|----------|------------|
| **🐍 Python** | 1.0.0 | ✅ Production | CRUD, AQL, Vektoren, Graph, Transaktionen | [themisdb-python](clients_python_sdk.md) |
| **📜 JavaScript/TypeScript** | 1.0.0 | ✅ Production | CRUD, AQL, Vektoren, Graph, Async/Await | [themisdb-js](clients_javascript_sdk.md) |
| **🦀 Rust** | 1.0.0 | ✅ Production | CRUD, AQL, Vektoren, Type-Safe, Zero-Copy | [themisdb-rust](clients_rust_sdk.md) |

### 🎯 Feature-Matrix

Alle SDKs unterstützen:

| Feature | Python | JavaScript | Rust |
|---------|--------|------------|------|
| ✅ CRUD Operations | ✅ | ✅ | ✅ |
| ✅ AQL Query Execution | ✅ | ✅ | ✅ |
| ✅ Vector Search | ✅ | ✅ | ✅ |
| ✅ Graph Traversal | ✅ | ✅ | ✅ |
| ✅ Batch Operations | ✅ | ✅ | ✅ |
| ✅ Connection Pooling | ✅ | ✅ | ✅ |
| ✅ Retry Logic | ✅ | ✅ | ✅ |
| ✅ Topologie-Aware Routing | ✅ | ✅ | ✅ |
| ✅ Transaktionen (ACID) | ✅ | ✅ | ✅ |

---

## 🚀 Schnellstart

### 🐍 Python

```python
from themisdb import ThemisDB

# Client erstellen
db = ThemisDB("http://localhost:8765")

# CRUD Operationen
db.put("relational", "users", "uuid-123", {"name": "Alice", "age": 30})
user = db.get("relational", "users", "uuid-123")

# AQL Query
result = db.query("FOR doc IN users FILTER doc.age > 25 RETURN doc")
print(result.entities)

# Vector Search
similar = db.vector_search([0.1, 0.2, 0.3], top_k=5)
```

### 📜 JavaScript/TypeScript

```javascript
import { ThemisDB } from 'themisdb';

// Client erstellen
const db = new ThemisDB('http://localhost:8765');

// CRUD Operationen
await db.put('relational', 'users', 'uuid-123', { name: 'Alice', age: 30 });
const user = await db.get('relational', 'users', 'uuid-123');

// AQL Query
const result = await db.query('FOR doc IN users FILTER doc.age > 25 RETURN doc');
console.log(result.entities);

// Vector Search
const similar = await db.vectorSearch([0.1, 0.2, 0.3], { topK: 5 });
```

### 🦀 Rust

```rust
use themisdb::ThemisDB;

// Client erstellen
let db = ThemisDB::new("http://localhost:8765")?;

// CRUD Operationen
db.put("relational", "users", "uuid-123", 
    &serde_json::json!({"name": "Alice", "age": 30}))?;
let user: Option<User> = db.get("relational", "users", "uuid-123")?;

// AQL Query
let result = db.query("FOR doc IN users FILTER doc.age > 25 RETURN doc")?;

// Vector Search
let similar = db.vector_search(&[0.1, 0.2, 0.3], None, Some(5))?;
```

---

## 📖 Detaillierte Dokumentation

### 📚 SDK-Referenzen

| Dokumentation | Beschreibung | Zielgruppe |
|---------------|--------------|------------|
| [🐍 Python SDK](clients_python_sdk.md) | Vollständige Python SDK Dokumentation | Python Entwickler |
| [📜 JavaScript SDK](clients_javascript_sdk.md) | JavaScript/TypeScript SDK Guide | Web/Node.js Entwickler |
| [🦀 Rust SDK](clients_rust_sdk.md) | Rust SDK Dokumentation | Systems Programmierer |
| [🔧 SDK Implementation](clients_sdk_implementation.md) | Technische Details zur SDK-Architektur | SDK Entwickler |

### 📦 Publishing & Analysis

| Dokumentation | Beschreibung | Zielgruppe |
|---------------|--------------|------------|
| [📋 Publishing Checklist](clients_publishing_checklist.md) | Release Checkliste für SDK-Publishing | Maintainer |
| [📦 Publishing Guide](clients_publishing_guide.md) | Vollständiger Publishing-Prozess | Release Manager |
| [🔍 SDK Analysis](clients_sdk_analysis.md) | Analyse relevanter SDK-Sprachen | Projektplaner |
| [✅ SDK Audit](clients_sdk_audit.md) | SDK Status und Feature-Übersicht | QA Team |

---

## 💡 Best Practices

### 🔒 Connection Management

```python
# ✅ Gut: Context Manager verwenden (Python)
with ThemisDB("http://localhost:8765") as db:
    result = db.query("FOR doc IN users RETURN doc")

# ✅ Gut: Try-finally für manuelle Cleanup (JavaScript)
const db = new ThemisDB('http://localhost:8765');
try {
    const result = await db.query('FOR doc IN users RETURN doc');
} finally {
    await db.close();
}
```

### ⚡ Batch-Operationen verwenden

```python
# ❌ Schlecht: Einzelne Requests
for user_id in user_ids:
    db.get("relational", "users", user_id)

# ✅ Gut: Batch Request
batch_result = db.batch_get("relational", "users", user_ids)
found_users = batch_result.found
missing_ids = batch_result.missing
```

### 🔄 Retry Logic konfigurieren

```javascript
// Retry-Konfiguration für instabile Netzwerke
const db = new ThemisDB('http://localhost:8765', {
    maxRetries: 5,
    timeout: 60000,
    retryDelay: 1000
});
```

### 📊 Cursor-basierte Pagination

```python
# Große Datenmengen effizient abrufen
page = db.query("FOR doc IN large_collection RETURN doc", 
                use_cursor=True, batch_size=100)

while page.has_more:
    process_results(page.items)
    page = db.query("FOR doc IN large_collection RETURN doc",
                    use_cursor=True, cursor=page.next_cursor)
```

---

## 🔧 Troubleshooting

### ❌ Connection Refused

**Problem:** `Connection refused` Fehler beim Client-Start

**Lösung:**
```bash
# Server-Status prüfen
docker ps | grep themis

# Health-Endpoint testen
curl http://localhost:8765/health

# Firewall-Regeln prüfen
netstat -an | grep 8765
```

### ❌ Topology Errors

**Problem:** `TopologyError: Failed to load cluster topology`

**Lösung:**
```python
# Fallback auf statische Endpunkte
db = ThemisDB(
    endpoints=["http://shard1:8765", "http://shard2:8765"],
    metadata_endpoint=None  # Topology-Fetch deaktivieren
)
```

### ❌ Query Timeouts

**Problem:** Lange Queries laufen in Timeout

**Lösung:**
```javascript
// Timeout erhöhen
const db = new ThemisDB('http://localhost:8765', {
    timeout: 120000  // 120 Sekunden
});

// Oder Cursor-Pagination verwenden
const result = await db.query('FOR doc IN huge_collection RETURN doc', {
    useCursor: true,
    batchSize: 1000
});
```

### ❌ Import Errors (Python)

**Problem:** `ModuleNotFoundError: No module named 'themis'`

**Lösung:**
```bash
# Installation prüfen
pip show themisdb-client

# Neuinstallation
pip install --upgrade themisdb-client

# Development Mode
pip install -e clients/python
```

---

## 📚 Siehe auch

### 🔗 Verwandte Dokumentation

- [📡 HTTP API Reference](../apis/HTTP_API_REFERENCE.md) - REST API Endpunkte
- [📝 AQL Reference](../aql/AQL_REFERENCE.md) - Query Language Syntax
- [🎯 Vector Search Guide](../features/FEATURE_VECTOR_SEARCH.md) - Vector Operations
- [🔄 Graph Traversal](../features/FEATURE_GRAPH_TRAVERSAL.md) - Graph Operations
- [💾 Transaction Guide](../guides/GUIDE_TRANSACTIONS.md) - ACID Transaktionen

### 🌐 Externe Links

- [Python SDK Repository](https://github.com/themisdb/themisdb-python)
- [JavaScript SDK Repository](https://github.com/themisdb/themisdb-js)
- [Rust SDK Repository](https://github.com/themisdb/themisdb-rust)
- [npm Package](https://www.npmjs.com/package/@themisdb/client)
- [PyPI Package](https://pypi.org/project/themisdb-client/)
- [crates.io Package](https://crates.io/crates/themisdb)

---

## 📝 Changelog

### Version 1.3.0 (22.12.2025)
- ✅ Aktualisierung auf v1.3.0 Template
- ✅ Erweiterte Feature-Matrix hinzugefügt
- ✅ Schnellstart-Beispiele für alle SDKs
- ✅ Best Practices Sektion erweitert
- ✅ Troubleshooting Guide hinzugefügt
- ✅ Alle relativen Links aktualisiert

### Version 1.0.0 (05.12.2025)
- ✅ Initiale Dokumentation
- ✅ Python, JavaScript, Rust SDK Support
- ✅ CRUD und AQL Operationen
- ✅ Vector Search und Graph Traversal
