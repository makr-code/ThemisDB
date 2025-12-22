# 🐍 ThemisDB Python SDK

<!-- Dokumentations-Metadaten -->
**Kategorie:** 💻 SDK Implementation  
**Version:** v1.3.0  
**Status:** ✅ Produktionsreif  
**Letztes Update:** 22. Dezember 2025

---

## 📑 Inhaltsverzeichnis

- [📋 Übersicht](#-übersicht)
- [✨ Features & Highlights](#-features--highlights)
- [🚀 Schnellstart](#-schnellstart)
- [📖 Detaillierte Dokumentation](#-detaillierte-dokumentation)
- [💡 Best Practices](#-best-practices)
- [🔧 Troubleshooting](#-troubleshooting)
- [📚 Siehe auch](#-siehe-auch)
- [📝 Changelog](#-changelog)

---

## 📋 Übersicht

Die Python-Clientbibliothek ermöglicht den Zugriff auf alle ThemisDB-Funktionen über eine idiomatische Python-API. Das SDK befindet sich im Alpha-Status (`themis.__version__ == "0.1.0a0"`) – Breaking Changes sind möglich.

### 🎯 Zielgruppe

- Python Entwickler (≥ 3.11)
- Data Scientists & ML Engineers
- Backend-Entwickler
- API-Integratoren

### ⚠️ Voraussetzungen

- Python ≥ 3.11
- Laufende ThemisDB-Instanz (lokal oder Remote)
- HTTP-Zugriff auf mindestens einen Cluster-Knoten (z.B. `http://127.0.0.1:8765`)
- Optional: Zugriff auf Metadaten-Service (etcd) für Topologie

---

## ✨ Features & Highlights

### 🎯 Kern-Features

| Feature | Beschreibung | Status |
|---------|--------------|--------|
| 📝 **CRUD Operations** | Get, Put, Delete für alle Modelle | ✅ Stabil |
| 🔍 **AQL Queries** | Vollständige Query-Language-Unterstützung | ✅ Stabil |
| 🎯 **Vector Search** | Ähnlichkeitssuche mit Filtern | ✅ Stabil |
| 🌐 **Graph Traversal** | Graph-Operationen und Traversierung | ✅ Stabil |
| 📦 **Batch Operations** | Bulk Get/Put/Delete | ✅ Stabil |
| 🔄 **Cursor Pagination** | Effiziente große Datensätze | ✅ Stabil |
| 🗺️ **Topology-Aware** | Automatisches Shard-Routing | ✅ Stabil |
| ♻️ **Retry Logic** | Automatische Wiederholungen bei 5xx | ✅ Stabil |
| 🔒 **Context Manager** | `with`-Statement Support | ✅ Stabil |

### 🆕 Besondere Python-Features

- ✅ **Type Hints** - Vollständige PEP 484 Typisierung
- ✅ **Context Manager** - Automatische Resource-Verwaltung
- ✅ **Async Support** - AsyncClient für `asyncio`
- ✅ **Dataclass Support** - Direkte Serialisierung
- ✅ **httpx Backend** - Moderne HTTP-Library

---

## 🚀 Schnellstart

### 📦 Installation

```bash
# Entwicklung im Repository
pip install -e clients/python

# Direkte Installation (lokales Artefakt)
pip install clients/python

# Mit Development Dependencies
pip install -e "clients/python[dev]"
```

> **Hinweis:** Paketindex-Veröffentlichung steht noch aus. Abhängigkeit `httpx>=0.26` wird automatisch installiert.

### 🎬 Erste Schritte

```python
from themis import ThemisClient

# Client erstellen
client = ThemisClient(
    endpoints=["http://127.0.0.1:8765"],
    namespace="default",
    metadata_endpoint="/_admin/cluster/topology"  # optional
)

# Health Check
health = client.health()
print(health)  # {'status': 'healthy', 'version': '0.1.0', ...}

# Client schließen
client.close()
```

### 🔒 Context Manager Pattern

```python
from themis import ThemisClient

# ✅ Empfohlene Methode: Automatische Cleanup
with ThemisClient(endpoints=["http://127.0.0.1:8765"]) as client:
    result = client.query("FOR doc IN users RETURN doc")
    print(result.entities)
# Client wird automatisch geschlossen
```

---

## 📖 Detaillierte Dokumentation

### ⚙️ Konfiguration & Topologie

```python
client = ThemisClient(
    endpoints=["http://shard-1:8765", "http://shard-2:8766"],
    namespace="production",
    metadata_endpoint="http://etcd:2379/topology",  # vollständige URL
    timeout=60,           # HTTP Timeout in Sekunden
    max_retries=5,        # Retry-Anzahl für 5xx Fehler
    max_workers=8         # Parallele Batch-Requests
)
```

| Parameter | Typ | Beschreibung | Default |
|-----------|-----|--------------|---------|
| `endpoints` | `List[str]` | Bootstrap HTTP-Basen für Cluster | **Pflicht** |
| `namespace` | `str` | Namespace für URN-Keys | `"default"` |
| `metadata_endpoint` | `str` | Relativer Pfad oder vollständige URL zu Topologie-Service | `"/_admin/cluster/topology"` |
| `timeout` | `float` | HTTP Timeout pro Request (Sekunden) | `30.0` |
| `max_retries` | `int` | Anzahl Retries für transiente Fehler | `3` |
| `max_workers` | `int` | Max parallele Batch-Operationen | `min(4, task_count)` |

**Topologie-Verhalten:**
- Client lädt beim ersten Request die Shard-Topologie
- Bei Fehlschlag: Fallback auf Bootstrap-Liste + `TopologyError`
- Automatisches Routing zu richtigen Shards basierend auf Key

### 📝 CRUD Operationen

```python
import uuid

# Eindeutige ID generieren
user_id = str(uuid.uuid4())

# CREATE / UPDATE
client.put("relational", "users", user_id, {
    "name": "Alice Schmidt",
    "email": "alice@example.com",
    "age": 30
})

# READ
user = client.get("relational", "users", user_id)
print(user)  # {'name': 'Alice Schmidt', 'email': '...', ...}

# DELETE
deleted = client.delete("relational", "users", user_id)
print(deleted)  # True wenn vorhanden, False sonst
```

### 📦 Batch-Operationen

```python
# Batch GET
user_ids = ["uuid-1", "uuid-2", "uuid-3", "uuid-missing"]
batch_result = client.batch_get("relational", "users", user_ids)

print(batch_result.found)     # {'uuid-1': {...}, 'uuid-2': {...}, ...}
print(batch_result.missing)   # ['uuid-missing']
print(batch_result.errors)    # {} oder Fehler pro UUID

# Batch PUT
users_data = {
    "uuid-1": {"name": "Alice", "age": 30},
    "uuid-2": {"name": "Bob", "age": 25},
    "uuid-3": {"name": "Charlie", "age": 35}
}
client.batch_put("relational", "users", users_data)
```

> **Hinweis:** Bei Verwendung von `httpx.MockTransport` (Tests) schaltet das SDK automatisch auf sequenzielle Verarbeitung um.

### 🔍 AQL Queries

```python
# Einfache Query
result = client.query("FOR u IN users FILTER u.age > 25 RETURN u")
print(result.entities)  # Liste von User-Dictionaries

# Query mit Parametern
result = client.query(
    "FOR u IN users FILTER u.age > @min_age RETURN u",
    bind_vars={"min_age": 30}
)

# Single-Shard Query (URN-basiert)
result = client.query(
    "FOR u IN urn:themis:relational:users:uuid-123 RETURN u"
)
# Wird automatisch nur an einen Shard geschickt
```

### 📄 Cursor-basierte Pagination

```python
# Erste Seite abrufen
page = client.query(
    "FOR u IN users RETURN u",
    use_cursor=True,
    batch_size=100
)

# Alle Seiten durchiterieren
all_users = page.items
while page.has_more:
    page = client.query(
        "FOR u IN users RETURN u",
        use_cursor=True,
        cursor=page.next_cursor
    )
    all_users.extend(page.items)

print(f"Gesamt: {len(all_users)} Benutzer")
```

**Cursor-Parameter:**
- `use_cursor=True` - Aktiviert Cursor-Modus
- `batch_size=N` - Items pro Seite
- `cursor=<token>` - Fortsetzung ab Position

### 🎯 Vector Search

```python
# Embedding-Vektor (z.B. von Sentence Transformers)
query_vector = [0.13, -0.4, 0.9, ...]  # 384 Dimensionen

# Ähnlichkeitssuche
result = client.vector_search(
    query_vector,
    top_k=10,
    filter={"category": "electronics", "price": {"$lt": 500}},
    namespace="products"
)

# Ergebnisse verarbeiten
for hit in result.results:
    print(f"{hit['id']}: Score {hit['score']:.4f}")
    print(f"  {hit['metadata']}")
```

### 🌐 Graph Traversal

```python
# Graph traversieren
result = client.graph_traverse(
    start_vertex="users/alice",
    direction="outbound",
    edge_collection="follows",
    max_depth=3,
    filters={"verified": True}
)

# Pfade analysieren
for path in result.paths:
    print(" -> ".join(path.vertices))
```

### 🔍 Spezielle Endpunkte

```python
# Health Check
health = client.health()
print(health)
# {'status': 'healthy', 'version': '0.1.0', 'uptime': 3600, ...}

# Topologie abrufen
topology = client.get_topology()
print(topology)
# {'shards': [...], 'replicas': [...], ...}
```

### ⚠️ Fehlerbehandlung

```python
from themis import TopologyError
import httpx

try:
    client = ThemisClient(endpoints=["http://localhost:8765"])
    result = client.query("FOR u IN users RETURN u")
    
except TopologyError as e:
    # Topologie konnte nicht geladen werden
    print(f"Topologie-Fehler: {e}")
    # Fallback auf statische Endpunkte
    
except httpx.HTTPStatusError as e:
    # HTTP Status ≥ 400
    print(f"HTTP-Fehler {e.response.status_code}: {e.response.text}")
    
except httpx.RequestError as e:
    # Netzwerkfehler, Timeout, etc.
    print(f"Netzwerkfehler: {e}")
    
finally:
    client.close()
```

**Error-Typen:**
- `TopologyError` - Topologie-Probleme
- `httpx.HTTPStatusError` - HTTP-Fehler (≥ 400)
- `httpx.RequestError` - Netzwerkausfälle, Timeouts
- `httpx.TimeoutException` - Request-Timeout

---

## 💡 Best Practices

### ✅ DO: Context Manager verwenden

```python
# ✅ Gut: Automatische Resource-Cleanup
with ThemisClient(endpoints=["http://localhost:8765"]) as client:
    result = client.query("FOR doc IN users RETURN doc")
```

### ✅ DO: Batch-Operationen für viele Items

```python
# ❌ Schlecht: N einzelne Requests
for user_id in user_ids:
    user = client.get("relational", "users", user_id)

# ✅ Gut: Ein Batch-Request
batch = client.batch_get("relational", "users", user_ids)
```

### ✅ DO: Type Hints verwenden

```python
from typing import Dict, List, Optional
from dataclasses import dataclass

@dataclass
class User:
    name: str
    email: str
    age: int

def get_users(client: ThemisClient) -> List[Dict]:
    result = client.query("FOR u IN users RETURN u")
    return result.entities
```

### ✅ DO: Cursor für große Datenmengen

```python
# ✅ Gut: Cursor-basierte Pagination
def process_all_users(client: ThemisClient):
    page = client.query("FOR u IN users RETURN u", use_cursor=True, batch_size=100)
    
    while True:
        for user in page.items:
            process_user(user)
        
        if not page.has_more:
            break
            
        page = client.query("FOR u IN users RETURN u", 
                          use_cursor=True, cursor=page.next_cursor)
```

### ❌ DON'T: Client für jeden Request neu erstellen

```python
# ❌ Schlecht: Overhead durch Topologie-Fetch
def get_user(user_id: str):
    client = ThemisClient(endpoints=["http://localhost:8765"])
    user = client.get("relational", "users", user_id)
    client.close()
    return user

# ✅ Gut: Client wiederverwenden
client = ThemisClient(endpoints=["http://localhost:8765"])

def get_user(user_id: str):
    return client.get("relational", "users", user_id)
```

### ⚙️ Produktions-Konfiguration

```python
# Robuste Konfiguration für Produktion
client = ThemisClient(
    endpoints=[
        "http://shard-1:8765",
        "http://shard-2:8765",
        "http://shard-3:8765"
    ],
    namespace="production",
    timeout=60,           # Erhöhter Timeout für komplexe Queries
    max_retries=5,        # Mehr Retries für instabile Netzwerke
    max_workers=8,        # Parallele Batch-Verarbeitung
    metadata_endpoint="http://etcd:2379/v3/keys/topology"
)
```

---

## 🔧 Troubleshooting

### ❌ ModuleNotFoundError

**Problem:**
```python
ModuleNotFoundError: No module named 'themis'
```

**Lösung:**
```bash
# Installation prüfen
pip show themisdb-client

# Neuinstallation
pip install -e clients/python

# Virtual Environment prüfen
which python
pip list | grep themis
```

### ❌ TopologyError

**Problem:**
```python
TopologyError: Failed to load cluster topology from /_admin/cluster/topology
```

**Lösung:**
```python
# Option 1: Vollständige URL angeben
client = ThemisClient(
    endpoints=["http://localhost:8765"],
    metadata_endpoint="http://etcd:2379/v3/keys/topology"
)

# Option 2: Topologie-Fetch deaktivieren
client = ThemisClient(
    endpoints=["http://shard1:8765", "http://shard2:8765"],
    metadata_endpoint=None  # Kein Topologie-Fetch
)
```

### ❌ Connection Refused

**Problem:**
```python
httpx.ConnectError: [Errno 111] Connection refused
```

**Lösung:**
```bash
# Server-Status prüfen
docker ps | grep themis

# Health-Endpoint testen
curl http://localhost:8765/health

# Port-Mapping prüfen
docker port themis-server
```

### ❌ Timeout Errors

**Problem:**
```python
httpx.TimeoutException: Request timeout after 30.0s
```

**Lösung:**
```python
# Timeout erhöhen
client = ThemisClient(
    endpoints=["http://localhost:8765"],
    timeout=120  # 2 Minuten
)

# Oder Cursor-Pagination verwenden
result = client.query(
    "FOR doc IN huge_collection RETURN doc",
    use_cursor=True,
    batch_size=1000
)
```

### ❌ Memory Issues bei großen Resultsets

**Problem:** `MemoryError` bei großen Query-Results

**Lösung:**
```python
# ✅ Cursor-Pagination verwenden
page = client.query("FOR doc IN users RETURN doc", 
                   use_cursor=True, batch_size=100)

while page.has_more:
    # Verarbeite kleine Batches
    process_batch(page.items)
    
    # Nächste Seite abrufen
    page = client.query("FOR doc IN users RETURN doc",
                       use_cursor=True, cursor=page.next_cursor)
```

---

## 📚 Siehe auch

### 🔗 Verwandte Dokumentation

- [📜 JavaScript SDK](clients_javascript_sdk.md) - JavaScript/TypeScript Client
- [🦀 Rust SDK](clients_rust_sdk.md) - Rust Client
- [📡 HTTP API Reference](../apis/HTTP_API_REFERENCE.md) - REST API Details
- [📝 AQL Reference](../aql/AQL_REFERENCE.md) - Query Language
- [🎯 Vector Search Guide](../features/FEATURE_VECTOR_SEARCH.md) - Vector Operations
- [🌐 Graph Traversal](../features/FEATURE_GRAPH_TRAVERSAL.md) - Graph Features

### 📖 Python-spezifische Ressourcen

- [asyncio Guide](https://docs.python.org/3/library/asyncio.html) - Async Programming
- [httpx Documentation](https://www.python-httpx.org/) - HTTP Client
- [Type Hints PEP 484](https://www.python.org/dev/peps/pep-0484/) - Type Annotations

### 🧪 Testing & Quality

```bash
# Tests ausführen
pytest clients/python/tests

# Mit Coverage
pytest --cov=themis clients/python/tests

# Spezifische Tests
pytest clients/python/tests/test_topology.py -v
```

**Test-Files:**
- `test_topology.py` - Topologie-Tests
- `test_batch.py` - Batch-Operations
- `test_cursor.py` - Cursor-Pagination
- `conftest.py` - Pytest Fixtures

---

## 📝 Changelog

### Version 1.3.0 (22.12.2025)
- ✅ Aktualisierung auf v1.3.0 Template-Standard
- ✅ Erweiterte Code-Beispiele mit Type Hints
- ✅ Detaillierte Fehlerbehandlung dokumentiert
- ✅ Best Practices Sektion erweitert
- ✅ Troubleshooting Guide hinzugefügt
- ✅ Alle relativen Links zu docs/de/ aktualisiert
- ✅ Context Manager Pattern dokumentiert

### Version 1.0.0 (05.12.2025)
- ✅ Alpha Release
- ✅ CRUD, AQL, Vector Search, Graph Support
- ✅ Batch Operations implementiert
- ✅ Cursor Pagination
- ✅ Topology-Aware Routing
- ✅ httpx Backend

### Version 0.1.0a0 (10.11.2025)
- ✅ Initial Alpha Build
- ✅ Basic CRUD Operations
- ✅ AQL Query Support
