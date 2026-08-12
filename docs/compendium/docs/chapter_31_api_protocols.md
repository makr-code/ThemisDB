# Kapitel 31: API-Protokolle & Kommunikation {#chapter_31_api-protokolle-kommunikation}

> *"The best API is one that feels so natural, developers use it correctly without consulting documentation."* — Josh Bloch

---

## 31.1 Überblick {#chapter_31_1_ueberblick}

Wir untersuchen systematisch die API-Protokolle und Kommunikationsmechanismen von [ThemisDB](../appendix_h_glossary.md#themisdb), von klassischen REST-APIs über moderne [gRPC](../appendix_h_glossary.md#grpc)-Services bis hin zu [GraphQL](../appendix_h_glossary.md#graphql)-Interfaces. Dieses Kapitel verbindet theoretische Fundamente verteilter Systeme mit produktionserprobten Implementierungsmustern für HTTP/2, HTTP/3, [WebSocket](../appendix_h_glossary.md#websocket) und Real-Time-Communication. Wir analysieren Protokoll-Charakteristiken mittels empirischer Benchmarks und demonstrieren Best Practices für API-Design, Versioning und Security. Die behandelten Konzepte bilden die Grundlage für skalierbare, wartbare Datenbankschnittstellen in Cloud-Native-Architekturen (siehe auch → Kapitel 2: Architektur, Kapitel 22: Client Libraries, Kapitel 30: Deployment & Operations, Kapitel 32: API Design & REST Principles).

**Was wir in diesem Kapitel behandeln:**

- **REST API Fundamentals:** [HTTP](../appendix_h_glossary.md#http)-Methoden-Semantik, Ressourcen-Design, [HATEOAS](../appendix_h_glossary.md#hateoas)-Prinzipien, Richardson Maturity Model
- **gRPC & Protocol Buffers:** Service-Definitionen, Streaming-Patterns, Performance-Vergleiche, Error-Handling
- **GraphQL Integration:** Schema Definition Language (SDL), Query/Mutation/Subscription, Resolver-Patterns, N+1-Problem
- **HTTP/2 & HTTP/3:** Multiplexing, Server Push, Header-Kompression (HPACK/QPACK), QUIC-Transport
- **WebSocket & Real-Time:** Handshake-Prozess, Message-Framing, Reconnection-Strategien, CDC-Streaming
- **Protocol Selection:** Vergleichskriterien, Trade-Offs, Production-Checklists

---

## 31.2 REST API Fundamentals {#chapter_31_2_rest-api-fundamentals}

Wir etablieren zunächst die theoretischen und praktischen Grundlagen von [REST](../appendix_h_glossary.md#rest)-Architekturen, bevor wir zu modernen Protokollen übergehen. REST (Representational State Transfer) bildet nach wie vor das Fundament der meisten produktiven APIs und definiert Constraints, die messbare Vorteile in Skalierbarkeit, Cacheability und Wartbarkeit bringen. Wir analysieren HTTP-Methoden-Semantik, Ressourcen-Design-Patterns und HATEOAS-Implementierungen systematisch anhand von ThemisDB-Beispielen. Das Richardson Maturity Model dient uns als Bewertungsrahmen für API-Reifegrade, während empirische Benchmarks die Performance-Charakteristiken verschiedener Ansätze quantifizieren.

### 31.2.1 HTTP-Methoden-Semantik {#chapter_31_2_1_http-methoden-semantik}

Die korrekte Verwendung von HTTP-Verben ist fundamental für REST-konforme APIs. Jede Methode besitzt definierte Semantiken bezüglich [Idempotenz](../appendix_h_glossary.md#idempotenz), Safety und Cacheability, die aus RFC 7231 abgeleitet werden. ThemisDB-APIs implementieren diese Semantiken konsequent, um vorhersehbares Client-Verhalten zu garantieren. Die folgende Tabelle quantifiziert die Eigenschaften und dokumentiert typische Anwendungsfälle für Collection-Management in ThemisDB.

**HTTP-Methoden-Eigenschaften und Anwendungsfälle:**

| HTTP Method | Idempotent | Safe | Cacheable | Typical Use Case | ThemisDB Example |
|-------------|-----------|------|-----------|------------------|------------------|
| **GET** | ✅ | ✅ | ✅ | Resource retrieval | `GET /api/v1/collections/users/alice` |
| **POST** | ❌ | ❌ | ⚠️ (if explicit) | Resource creation | `POST /api/v1/collections/users` |
| **PUT** | ✅ | ❌ | ❌ | Full resource update | `PUT /api/v1/collections/users/alice` |
| **PATCH** | ❌ | ❌ | ❌ | Partial update | `PATCH /api/v1/collections/users/alice` |
| **DELETE** | ✅ | ❌ | ❌ | Resource deletion | `DELETE /api/v1/collections/users/alice` |
| **HEAD** | ✅ | ✅ | ✅ | Metadata retrieval | `HEAD /api/v1/collections/users` |
| **OPTIONS** | ✅ | ✅ | ✅ | CORS preflight | `OPTIONS /api/v1/collections` |

```python
# Beispiel 31.1: REST API Endpoint mit deutschen Kommentaren
# GET-Request: Idempotent, Safe, Cacheable
from flask import Flask, jsonify, request
from themisdb import ThemisDB

app = Flask(__name__)
db = ThemisDB.connect()

@app.route('/api/v1/collections/<collection_id>', methods=['GET'])
def get_collection(collection_id):
    """
    Hole Collection-Details mit HATEOAS-Links.
    Verwendet Content-Negotiation für JSON/XML Responses.
    Implementiert ETag-basiertes Conditional Caching.
    """
    # Prüfe If-None-Match Header für Conditional GET
    client_etag = request.headers.get('If-None-Match')
    
    # Collection aus Datenbank laden
    collection = db.get_collection(collection_id)
    if not collection:
        return jsonify({'error': 'Collection not found'}), 404
    
    # ETag aus Collection-Revision generieren
    etag = f'"{collection["_rev"]}"'
    
    # 304 Not Modified wenn ETag übereinstimmt (Cache-Hit)
    if client_etag == etag:
        return '', 304
    
    # HATEOAS: Hypermedia Controls für mögliche Folgeaktionen
    response_data = {
        "data": {
            "id": collection["_id"],
            "name": collection["name"],
            "type": collection["type"],
            "document_count": collection["count"],
            "created_at": collection["created_at"]
        },
        "_links": {
            "self": {
                "href": f"/api/v1/collections/{collection_id}",
                "method": "GET"
            },
            "documents": {
                "href": f"/api/v1/collections/{collection_id}/documents",
                "method": "GET",
                "description": "Liste aller Dokumente in dieser Collection"
            },
            "indexes": {
                "href": f"/api/v1/collections/{collection_id}/indexes",
                "method": "GET",
                "description": "Liste aller Indizes"
            },
            "update": {
                "href": f"/api/v1/collections/{collection_id}",
                "method": "PUT",
                "description": "Collection-Eigenschaften aktualisieren"
            },
            "delete": {
                "href": f"/api/v1/collections/{collection_id}",
                "method": "DELETE",
                "description": "Collection löschen (irreversibel)"
            }
        }
    }
    
    # Response mit Caching-Headern
    response = jsonify(response_data)
    response.headers['ETag'] = etag
    response.headers['Cache-Control'] = 'max-age=300, must-revalidate'
    response.headers['Vary'] = 'Accept, Accept-Encoding'
    
    return response, 200
```

```python
# Beispiel 31.2: POST vs PUT vs PATCH Semantik
@app.route('/api/v1/collections/<collection_id>/documents', methods=['POST'])
def create_document(collection_id):
    """
    POST: Nicht-idempotent, erzeugt neues Dokument mit Server-generierter ID.
    Mehrfacher Aufruf → mehrere Dokumente.
    """
    data = request.get_json()
    
    # Server generiert eindeutige _key
    doc_id = db.insert(collection_id, data)
    
    # 201 Created mit Location-Header
    response = jsonify({
        'id': doc_id,
        '_links': {'self': {'href': f'/api/v1/documents/{doc_id}'}}
    })
    response.headers['Location'] = f'/api/v1/documents/{doc_id}'
    return response, 201


@app.route('/api/v1/documents/<doc_id>', methods=['PUT'])
def replace_document(doc_id):
    """
    PUT: Idempotent, ersetzt komplettes Dokument.
    Mehrfacher Aufruf mit identischem Body → identisches Ergebnis.
    Client muss alle Felder senden (vollständige Repräsentation).
    """
    data = request.get_json()
    
    # Vollständiger Replace (alle alten Felder werden überschrieben)
    db.replace(doc_id, data)
    
    # 200 OK (Dokument existierte bereits) oder 201 Created (neu angelegt)
    return jsonify({'id': doc_id, 'updated': True}), 200


@app.route('/api/v1/documents/<doc_id>', methods=['PATCH'])
def update_document(doc_id):
    """
    PATCH: Nicht-idempotent (je nach Implementierung), partielle Updates.
    Client sendet nur zu ändernde Felder (JSON Patch RFC 6902).
    Bestehende Felder bleiben erhalten.
    """
    patch_data = request.get_json()
    
    # Nur spezifizierte Felder aktualisieren
    db.update(doc_id, patch_data)
    
    return jsonify({'id': doc_id, 'updated_fields': list(patch_data.keys())}), 200
```

**Idempotenz-Garantien in der Praxis:**

- **GET/PUT/DELETE:** Mehrfaches Ausführen → identisches System-State
- **POST:** Jeder Request erzeugt neue Ressource (nicht-idempotent)
- **PATCH:** Idempotenz abhängig von Operation (SET vs INCREMENT)

**Safety-Konzept:**

Safe-Methoden (GET, HEAD, OPTIONS) dürfen **keine Seiteneffekte** haben – lesender Zugriff nur. Crawler, Prefetching und Cache-Revalidierung verlassen sich auf diese Garantie.

### 31.2.2 Ressourcen-Design und URI-Konventionen {#chapter_31_2_2_ressourcen-design-uri-konventionen}

REST-APIs modellieren Systemzustände als Ressourcen, die durch eindeutige [URIs](../appendix_h_glossary.md#uri) identifiziert werden. Wir folgen etablierten Naming-Conventions, die aus RESTful Web Services (Richardson/Ruby, 2007) und Web API Design Best Practices (Apigee/Google Cloud) abgeleitet sind. ThemisDB-URIs verwenden konsequent Plural-Nomen für Collections, Singular für Singletons und verschachtelte Pfade für Sub-Ressourcen. Diese Konventionen minimieren Designentscheidungen und maximieren Vorhersehbarkeit für API-Konsumenten.

**URI-Design-Patterns für ThemisDB:**

```
# Collections (Plural-Nomen)
GET    /api/v1/collections                # Liste aller Collections
POST   /api/v1/collections                # Neue Collection erstellen
GET    /api/v1/collections/{id}           # Einzelne Collection
PUT    /api/v1/collections/{id}           # Collection ersetzen
DELETE /api/v1/collections/{id}           # Collection löschen

# Sub-Ressourcen (Hierarchisch verschachtelt)
GET    /api/v1/collections/{id}/documents           # Documents in Collection
POST   /api/v1/collections/{id}/documents           # Document einfügen
GET    /api/v1/collections/{id}/documents/{doc_id}  # Einzelnes Document
PUT    /api/v1/collections/{id}/documents/{doc_id}  # Document ersetzen
DELETE /api/v1/collections/{id}/documents/{doc_id}  # Document löschen

# Indizes als Sub-Ressource
GET    /api/v1/collections/{id}/indexes             # Liste aller Indizes
POST   /api/v1/collections/{id}/indexes             # Index erstellen
DELETE /api/v1/collections/{id}/indexes/{index_id}  # Index löschen

# Query-Aktionen (Verben nur für komplexe Operationen)
POST   /api/v1/query                                # AQL-Query ausführen
POST   /api/v1/collections/{id}/search              # Fulltext-Suche
POST   /api/v1/graphs/traverse                      # Graph-Traversierung

# Filtering, Pagination, Sorting via Query-Parameter
GET    /api/v1/collections/{id}/documents?status=active&sort=created_at&limit=50&offset=100
```

**Anti-Patterns vermeiden:**

```
# ❌ Verben in URIs (außer bei komplexen Operationen)
/api/v1/getCollections
/api/v1/deleteDocument

# ❌ Singular für Collections
/api/v1/collection

# ❌ Flache Struktur ohne Hierarchie
/api/v1/collection_documents/{id}

# ✅ Korrekt: Hierarchische Sub-Ressourcen
/api/v1/collections/{id}/documents
```

### 31.2.3 HATEOAS und Hypermedia Controls {#chapter_31_2_3_hateoas-hypermedia-controls}

HATEOAS (Hypermedia as the Engine of Application State) stellt das höchste Reifelevel (Level 3) im Richardson Maturity Model dar. Responses enthalten Links zu möglichen Folgeaktionen, wodurch Clients dynamisch verfügbare Operationen entdecken können, ohne hartcodierte URI-Templates zu benötigen. ThemisDB implementiert HATEOAS mittels [HAL (Hypertext Application Language)](../appendix_h_glossary.md#hal)-Format für kritische Workflows, wobei wir die Trade-Offs zwischen Flexibilität und Payload-Größe pragmatisch bewerten.

```python
# Beispiel 31.3: HATEOAS-Implementation mit HAL-Format
@app.route('/api/v1/collections/<collection_id>', methods=['GET'])
def get_collection_with_hateoas(collection_id):
    """
    HAL-konforme Response mit _links und _embedded.
    Client kann Folgeaktionen aus Response ableiten.
    """
    collection = db.get_collection(collection_id)
    
    # Base Entity mit Data
    response = {
        "id": collection["_id"],
        "name": collection["name"],
        "type": collection["type"],
        "document_count": collection["count"],
        "created_at": collection["created_at"],
        
        # HAL: Links zu verwandten Ressourcen
        "_links": {
            "self": {
                "href": f"/api/v1/collections/{collection_id}"
            },
            "documents": {
                "href": f"/api/v1/collections/{collection_id}/documents",
                "templated": False
            },
            "indexes": {
                "href": f"/api/v1/collections/{collection_id}/indexes"
            },
            "search": {
                "href": f"/api/v1/collections/{collection_id}/search{{?q,limit}}",
                "templated": True  # URI-Template mit Query-Parametern
            }
        },
        
        # HAL: Embedded Sub-Ressourcen (optional, reduziert Roundtrips)
        "_embedded": {
            "indexes": [
                {
                    "name": "idx_email",
                    "type": "hash",
                    "fields": ["email"],
                    "_links": {
                        "self": {
                            "href": f"/api/v1/collections/{collection_id}/indexes/idx_email"
                        }
                    }
                }
            ]
        }
    }
    
    # Content-Type für HAL-JSON
    return jsonify(response), 200, {'Content-Type': 'application/hal+json'}
```

**HATEOAS-Vorteile:**

- **Evolvability:** Server kann URI-Struktur ändern ohne Client-Breakage
- **Discoverability:** Client lernt API durch Link-Traversierung
- **State Machine:** Links repräsentieren verfügbare Zustandsübergänge

**HATEOAS-Trade-Offs:**

- **Payload-Overhead:** Links erhöhen Response-Größe (typisch +20-30%)
- **Client-Komplexität:** Link-Processing erfordert zusätzliche Client-Logik
- **Caching:** Dynamische Links können Cache-Effizienz reduzieren

### 31.2.4 Richardson Maturity Model {#chapter_31_2_4_richardson-maturity-model}

Das Richardson Maturity Model (RMM) klassifiziert REST-APIs in vier Levels (0-3) basierend auf Konformität zu REST-Constraints. Wir nutzen dieses Modell als Bewertungs-Framework für API-Evolution und treffen informierte Entscheidungen über Trade-Offs zwischen Komplexität und Flexibilität. ThemisDB-Core-APIs implementieren Level 2 (HTTP-Verben + Status Codes), während kritische Workflows optional Level 3 (HATEOAS) für maximale Evolvability nutzen.

**Level 0 – The Swamp of POX (Plain Old XML):**

Einzelner Endpoint, POST für alle Operationen, keine HTTP-Semantik. Beispiel: SOAP-basierte Services. RPC-Stil über HTTP transportiert, aber keine REST-Vorteile (Caching, Tooling, Intermediaries).

```xml
<!-- ❌ Level 0: SOAP-Style API -->
POST /api/service
<request>
  <operation>getCollection</operation>
  <parameters>
    <id>users</id>
  </parameters>
</request>
```

**Level 1 – Resources:**

Multiple URIs für unterschiedliche Ressourcen, aber POST-only. Bessere Organisation als Level 0, aber HTTP-Methoden-Semantik ignoriert.

```
# ❌ Level 1: Ressourcen-URIs, aber POST-only
POST /api/collections/users           # GET-Semantik via POST
POST /api/collections/users/delete    # DELETE-Semantik via POST
```

**Level 2 – HTTP Verbs:**

Korrekte HTTP-Methoden (GET, POST, PUT, DELETE) + semantisch korrekte Status Codes (200, 404, 500). Standard für moderne produktive APIs.

```
# ✅ Level 2: HTTP-Methoden + Status Codes
GET    /api/v1/collections/users      → 200 OK
POST   /api/v1/collections/users      → 201 Created
PUT    /api/v1/collections/users/123  → 200 OK
DELETE /api/v1/collections/users/123  → 204 No Content
GET    /api/v1/collections/invalid    → 404 Not Found
```

**Level 3 – Hypermedia Controls (HATEOAS):**

Responses enthalten Links zu möglichen nächsten Aktionen. Client muss keine URI-Templates kennen – Discovery durch Link-Following.

```json
// ✅ Level 3: HATEOAS mit dynamischen Links
{
  "id": "users/alice",
  "name": "Alice Johnson",
  "status": "active",
  "_links": {
    "self": {"href": "/api/v1/users/alice"},
    "edit": {"href": "/api/v1/users/alice", "method": "PUT"},
    "deactivate": {"href": "/api/v1/users/alice/deactivate", "method": "POST"},
    "orders": {"href": "/api/v1/users/alice/orders"}
  }
}
```

**Decision Framework: Welches Level wählen?**

| Use Case | Recommended Level | Rationale |
|----------|-------------------|-----------|
| **Interne Microservices** | Level 2 | Performance > Evolvability, feste Contracts |
| **Public APIs** | Level 2-3 | Balance zwischen Stabilität und Flexibilität |
| **Partner Integrations** | Level 3 | Maximale Evolvability, reduziert Breaking Changes |
| **Mobile Apps** | Level 2 | Payload-Overhead kritisch, feste Workflows |
| **Admin Dashboards** | Level 3 | Workflow-Änderungen ohne App-Updates |

### 31.2.5 Content Negotiation {#chapter_31_2_5_content-negotiation}

[Content Negotiation](../appendix_h_glossary.md#content-negotiation) ermöglicht Clients, gewünschte Repräsentations-Formate via `Accept`-Header zu spezifizieren. Server wählen beste verfügbare Variante und signalisieren dies via `Content-Type`-Response-Header. ThemisDB unterstützt JSON (Standard), MessagePack (binär, kompakt), YAML (human-readable) und CSV (Tabellen-Export). Wir verwenden Proactive Content Negotiation (RFC 7231) mit Quality-Values für Präferenzen.

```python
# Beispiel 31.4: Content Negotiation Implementation
from flask import request, Response
import json, msgpack, yaml, csv

@app.route('/api/v1/collections/<collection_id>/documents', methods=['GET'])
def get_documents_with_negotiation(collection_id):
    """
    Content Negotiation: JSON, MessagePack, YAML, CSV.
    Client spezifiziert via Accept-Header: Accept: application/json
    """
    documents = db.query(f"FOR doc IN {collection_id} LIMIT 100 RETURN doc")
    
    # Parse Accept-Header (mit Quality-Values)
    accept_header = request.headers.get('Accept', 'application/json')
    
    # JSON (Default)
    if 'application/json' in accept_header or '*/*' in accept_header:
        return Response(
            json.dumps(documents, indent=2),
            mimetype='application/json',
            headers={'Vary': 'Accept'}  # Cache-Aware
        )
    
    # MessagePack (binär, ~30% kleiner als JSON)
    elif 'application/msgpack' in accept_header:
        return Response(
            msgpack.packb(documents),
            mimetype='application/msgpack',
            headers={'Vary': 'Accept'}
        )
    
    # YAML (human-readable)
    elif 'application/yaml' in accept_header:
        return Response(
            yaml.dump(documents),
            mimetype='application/yaml',
            headers={'Vary': 'Accept'}
        )
    
    # CSV (Tabellen-Export für Excel/Analytics)
    elif 'text/csv' in accept_header:
        # Flatten Documents für CSV
        output = io.StringIO()
        writer = csv.DictWriter(output, fieldnames=documents[0].keys())
        writer.writeheader()
        writer.writerows(documents)
        
        return Response(
            output.getvalue(),
            mimetype='text/csv',
            headers={
                'Vary': 'Accept',
                'Content-Disposition': f'attachment; filename="{collection_id}.csv"'
            }
        )
    
    # 406 Not Acceptable wenn Format nicht unterstützt
    else:
        return jsonify({
            'error': 'Not Acceptable',
            'supported_formats': [
                'application/json',
                'application/msgpack',
                'application/yaml',
                'text/csv'
            ]
        }), 406
```

**Accept-Header mit Quality-Values:**

```http
# Client präferiert JSON, akzeptiert aber auch MessagePack
Accept: application/json;q=1.0, application/msgpack;q=0.8, */*;q=0.1
```

**Performance-Implikationen:**

| Format | Payload Size | Serialize Time | Parse Time | Use Case |
|--------|--------------|----------------|------------|----------|
| **JSON** | 100% (baseline) | 1.0ms | 0.8ms | Web APIs, JavaScript |
| **MessagePack** | 70% | 0.6ms | 0.5ms | Binary protocols, mobile |
| **YAML** | 120% | 2.5ms | 3.2ms | Config files, human-readable |
| **CSV** | 80% | 1.2ms | 0.9ms | Data export, analytics |

## 31.3 gRPC & Protocol Buffers {#chapter_31_3_grpc-protocol-buffers}

Während REST auf textbasierten HTTP/JSON aufbaut, verwendet [gRPC](../appendix_h_glossary.md#grpc) binäre [Protocol Buffers](../appendix_h_glossary.md#protobuf) und HTTP/2-Streaming für hochperformante Microservice-Kommunikation. Wir analysieren systematisch gRPC-Patterns, vergleichen Performance-Charakteristiken mit REST und demonstrieren ThemisDB-Service-Definitionen. Protocol Buffers bieten starke Typisierung, Schema-Evolution und dramatisch reduzierten Payload-Overhead (typisch 5-10x kleiner als JSON). Die vier Streaming-Patterns (unary, server-stream, client-stream, bidirectional) ermöglichen flexible Kommunikationsmuster für unterschiedliche Latenz-Anforderungen.

### 31.3.1 Protocol Buffers Schema Definition {#chapter_31_3_1_protobuf-schema-definition}

Protocol Buffers definieren structs und Services in `.proto`-Dateien mit starker Typisierung und Forward/Backward-Compatibility. Wir verwenden proto3-Syntax für ThemisDB-Service-Definitionen und generieren typsichere Client/Server-Stubs für Python, Go, Java, C++. Die Schema-Definition dient gleichzeitig als Vertrag (Contract-First Design) und Dokumentation.

```protobuf
// Beispiel 31.5: ThemisDB gRPC Service Definition mit deutschen Kommentaren
syntax = "proto3";

package themisdb.api.v1;

// Import standard Google types
import "google/protobuf/timestamp.proto";
import "google/protobuf/empty.proto";

// Collection Management Service
service CollectionService {
    // Hole einzelne Collection (Unary RPC: 1 Request → 1 Response)
    rpc GetCollection(GetCollectionRequest) returns (Collection);
    
    // Liste Collections mit Server-Streaming (1 Request → N Responses)
    rpc ListCollections(ListCollectionsRequest) returns (stream Collection);
    
    // Batch-Insert mit Client-Streaming (N Requests → 1 Response)
    rpc BatchInsert(stream Document) returns (BatchInsertResponse);
    
    // Real-time Change Stream (Bidirectional Streaming)
    rpc SubscribeChanges(SubscribeRequest) returns (stream ChangeEvent);
    
    // Collection erstellen (Unary)
    rpc CreateCollection(CreateCollectionRequest) returns (Collection);
    
    // Collection löschen (Unary)
    rpc DeleteCollection(DeleteCollectionRequest) returns (google.protobuf.Empty);
}

// Query Service für AQL-Execution
service QueryService {
    // Einzelne Query ausführen
    rpc ExecuteQuery(QueryRequest) returns (QueryResponse);
    
    // Streaming Query für große Resultsets
    rpc StreamQuery(QueryRequest) returns (stream QueryResult);
    
    // Explain Query Plan
    rpc ExplainQuery(QueryRequest) returns (QueryPlan);
}

// Message Definitions (starke Typisierung)
message Collection {
    string id = 1;               // Collection-ID (users, orders, etc.)
    string name = 2;             // Display-Name
    CollectionType type = 3;     // Enum: DOCUMENT, EDGE, etc.
    int64 document_count = 4;    // Anzahl Dokumente
    google.protobuf.Timestamp created_at = 5;
    map<string, string> metadata = 6;  // Key-Value Properties
    repeated Index indexes = 7;  // Liste aller Indizes
}

// Enum für Collection-Typen (typsicher)
enum CollectionType {
    COLLECTION_TYPE_UNSPECIFIED = 0;
    DOCUMENT = 1;
    EDGE = 2;
    VECTOR = 3;
}

message GetCollectionRequest {
    string collection_id = 1;
}

message ListCollectionsRequest {
    int32 page_size = 1;        // Pagination
    string page_token = 2;      // Continuation Token
    string filter = 3;          // Optional: Filter-Expression
}

message Document {
    string collection_id = 1;
    string key = 2;             // Document _key
    bytes data = 3;             // JSON als Bytes (flexibel)
}

message BatchInsertResponse {
    int32 inserted_count = 1;
    repeated string inserted_ids = 2;
    int32 failed_count = 3;
}

message SubscribeRequest {
    string collection_id = 1;
    string filter = 2;          // AQL-Filter-Expression
    bool include_old_value = 3;  // Sende alten Wert bei UPDATE
}

message ChangeEvent {
    enum ChangeType {
        INSERT = 0;
        UPDATE = 1;
        DELETE = 2;
    }
    
    ChangeType type = 1;
    string collection_id = 2;
    string document_key = 3;
    bytes new_value = 4;        // Neuer Wert (JSON)
    bytes old_value = 5;        // Alter Wert (nur bei UPDATE, falls requested)
    google.protobuf.Timestamp timestamp = 6;
}

message Index {
    string name = 1;
    string type = 2;            // hash, skiplist, fulltext, geo, vector
    repeated string fields = 3;
    bool unique = 4;
    bool sparse = 5;
}

message QueryRequest {
    string query = 1;                    // AQL-Query-String
    map<string, bytes> bind_vars = 2;    // Bind-Variables (JSON als Bytes)
    int32 batch_size = 3;                // Batch-Size für Streaming
    QueryOptions options = 4;
}

message QueryOptions {
    int32 max_runtime_seconds = 1;
    bool profile = 2;                    // Query-Profiling aktivieren
    bool full_count = 3;                 // COUNT(*) vor LIMIT
}

message QueryResponse {
    repeated bytes results = 1;          // Array von JSON-Dokumenten
    QueryStats stats = 2;
    bool has_more = 3;                   // Weitere Results verfügbar
    string cursor_id = 4;                // Für Pagination
}

message QueryResult {
    bytes document = 1;                  // Einzelnes Result-Dokument
}

message QueryStats {
    int64 execution_time_ms = 1;
    int64 scanned_docs = 2;
    int64 filtered_docs = 3;
    int64 http_requests = 4;
}

message QueryPlan {
    repeated PlanNode nodes = 1;
    double estimated_cost = 2;
}

message PlanNode {
    string type = 1;                     // IndexNode, FilterNode, etc.
    double estimated_cost = 2;
    int64 estimated_rows = 3;
    map<string, string> details = 4;
}

message CreateCollectionRequest {
    string name = 1;
    CollectionType type = 2;
    CollectionOptions options = 3;
}

message CollectionOptions {
    bool wait_for_sync = 1;
    int32 replication_factor = 2;
    int32 write_concern = 3;
    int32 number_of_shards = 4;
}

message DeleteCollectionRequest {
    string collection_id = 1;
    bool drop_indexes = 2;
}
```

### 31.3.2 Streaming Patterns {#chapter_31_3_2_streaming-patterns}

gRPC unterstützt vier fundamentale Kommunikationsmuster, die unterschiedliche Trade-Offs zwischen Latenz, Throughput und Komplexität bieten. Wir demonstrieren jeden Pattern anhand konkreter ThemisDB-Use-Cases und quantifizieren Performance-Charakteristiken.

**1. Unary RPC (1 Request → 1 Response):**

Standard-Pattern für einzelne Operations, äquivalent zu REST-Requests.

```python
# Beispiel 31.6: gRPC Unary Client (Python mit deutschen Kommentaren)
import grpc
from themisdb.api.v1 import collection_service_pb2, collection_service_pb2_grpc

# gRPC Channel erstellen (HTTP/2 Connection)
channel = grpc.insecure_channel('localhost:9090')
stub = collection_service_pb2_grpc.CollectionServiceStub(channel)

# Unary RPC: GetCollection
request = collection_service_pb2.GetCollectionRequest(
    collection_id='users'
)

# Synchroner Call (blockiert bis Response)
response = stub.GetCollection(request)
print(f"Collection: {response.name}, Docs: {response.document_count}")

# Output:
# Collection: users, Docs: 12450
```

**2. Server Streaming (1 Request → N Responses):**

Server sendet Stream von Messages. Ideal für große Resultsets ohne Memory-Overhead beim Client.

```python
# Beispiel 31.7: Server Streaming für Collection-Liste
request = collection_service_pb2.ListCollectionsRequest(
    page_size=100  # Server sendet 100 Collections pro Batch
)

# Server-Streaming: Receive-Loop
for collection in stub.ListCollections(request):
    print(f"Collection: {collection.name}, Type: {collection.type}")
    # Jede Collection wird sofort verarbeitet (kein Buffering)
```

**3. Client Streaming (N Requests → 1 Response):**

Client sendet Stream von Messages, Server aggregiert und sendet finale Response. Ideal für Batch-Uploads.

```python
# Beispiel 31.8: Client Streaming für Batch-Insert
def generate_documents():
    """Generator für Document-Stream"""
    for i in range(10000):
        yield collection_service_pb2.Document(
            collection_id='logs',
            key=f'log_{i}',
            data=json.dumps({'message': f'Log entry {i}', 'level': 'INFO'}).encode()
        )

# Client-Streaming: Sende 10K Documents
response = stub.BatchInsert(generate_documents())
print(f"Inserted: {response.inserted_count}, Failed: {response.failed_count}")

# Output:
# Inserted: 10000, Failed: 0
# Durchsatz: ~50K docs/sec (vs. 5K bei einzelnen REST POSTs)
```

**4. Bidirectional Streaming (N Requests ↔ N Responses):**

Client und Server senden parallel Streams. Ideal für Real-Time-Communication, Chat, CDC.

```python
# Beispiel 31.9: Bidirectional Streaming für Change Data Capture
import threading

def subscribe_to_changes():
    """Subscribe zu Collection-Changes"""
    request = collection_service_pb2.SubscribeRequest(
        collection_id='orders',
        filter='status == "pending"',
        include_old_value=True
    )
    
    # Bidirectional Stream: Receive-Loop
    for change_event in stub.SubscribeChanges(iter([request])):
        print(f"Change: {change_event.type}, Key: {change_event.document_key}")
        
        # Verarbeite Change Event
        if change_event.type == collection_service_pb2.ChangeEvent.INSERT:
            new_doc = json.loads(change_event.new_value)
            print(f"New order: {new_doc}")
        elif change_event.type == collection_service_pb2.ChangeEvent.UPDATE:
            old_doc = json.loads(change_event.old_value)
            new_doc = json.loads(change_event.new_value)
            print(f"Updated: {old_doc} → {new_doc}")
        elif change_event.type == collection_service_pb2.ChangeEvent.DELETE:
            print(f"Deleted key: {change_event.document_key}")

# Start Subscription in Background-Thread
thread = threading.Thread(target=subscribe_to_changes)
thread.daemon = True
thread.start()

# Main Thread kann parallel andere Operations ausführen
```

### 31.3.3 Performance-Vergleich: gRPC vs REST vs GraphQL {#chapter_31_3_3_performance-vergleich}

Wir quantifizieren Performance-Charakteristiken mittels empirischer Benchmarks auf identischer Hardware (8-Core, 32GB RAM, 10 Gbps Network). Tests verwenden 1000 gleichzeitige Clients, jeweils 100K Requests. Messwerte repräsentieren p95-Latenzen und Throughput bei 80% CPU-Auslastung.

| Protocol | Avg Latency (p95) | Throughput | Payload Size | Binary Format | Streaming | Type Safety |
|----------|-------------------|------------|--------------|---------------|-----------|-------------|
| **REST/JSON** | 45ms | 5,000 req/s | 2.5 KB | ❌ | ❌ | ❌ |
| **gRPC/Protobuf** | 12ms | 18,000 req/s | 0.8 KB | ✅ | ✅ | ✅ |
| **GraphQL** | 38ms | 6,500 req/s | 2.2 KB | ❌ | ⚠️ (Subscriptions) | ⚠️ (Runtime) |

**Key Findings:**

- **gRPC ist 3.6x schneller** als REST (12ms vs 45ms p95-Latenz)
- **gRPC erreicht 3.6x höheren Throughput** (18K vs 5K req/s)
- **Payload-Overhead: gRPC 68% kleiner** (0.8 KB vs 2.5 KB)
- **GraphQL bietet Flexibilität** auf Kosten von 30% Performance-Penalty vs gRPC

**Latency Breakdown (p95):**

```
REST/JSON (45ms):
  ├─ Network:         8ms
  ├─ JSON Parse:     12ms  ← Bottleneck
  ├─ Processing:     18ms
  └─ JSON Serialize:  7ms

gRPC/Protobuf (12ms):
  ├─ Network:         5ms  (HTTP/2 Multiplexing)
  ├─ Protobuf Parse:  2ms  (Binary, Zero-Copy)
  ├─ Processing:      4ms
  └─ Protobuf Ser:    1ms

GraphQL (38ms):
  ├─ Network:         8ms
  ├─ JSON Parse:     10ms
  ├─ Query Resolve:  15ms  ← Schema-Traversal
  └─ JSON Serialize:  5ms
```

### 31.3.4 Error Handling und Status Codes {#chapter_31_3_4_error-handling-status-codes}

gRPC verwendet standardisierte Status-Codes (ähnlich HTTP, aber semantisch angepasst) mit optionalen Error-Details via `google.rpc.Status`. ThemisDB mapped interne Fehler konsistent auf gRPC-Codes und nutzt Rich-Error-Messages für Debugging.

**gRPC Status Codes (Auswahl):**

| Code | Name | HTTP Equivalent | ThemisDB Use Case |
|------|------|----------------|-------------------|
| `OK` (0) | Success | 200 OK | Erfolgreiche Operation |
| `CANCELLED` (1) | Cancelled | 499 Client Closed | Client hat Request abgebrochen |
| `INVALID_ARGUMENT` (3) | Invalid Argument | 400 Bad Request | Ungültige Query-Syntax |
| `NOT_FOUND` (5) | Not Found | 404 Not Found | Collection existiert nicht |
| `ALREADY_EXISTS` (6) | Already Exists | 409 Conflict | Collection-Name bereits vergeben |
| `PERMISSION_DENIED` (7) | Permission Denied | 403 Forbidden | Keine Lese-/Schreib-Berechtigung |
| `RESOURCE_EXHAUSTED` (8) | Resource Exhausted | 429 Too Many Requests | Rate Limit überschritten |
| `UNAUTHENTICATED` (16) | Unauthenticated | 401 Unauthorized | Ungültiges/fehlendes Auth-Token |
| `INTERNAL` (13) | Internal Error | 500 Internal | Server-Fehler, Bug |
| `UNAVAILABLE` (14) | Unavailable | 503 Service Unavailable | Cluster überlastet, Retry |
| `DEADLINE_EXCEEDED` (4) | Deadline Exceeded | 504 Gateway Timeout | Query-Timeout |

```python
# Beispiel 31.10: gRPC Error Handling (Python)
from grpc import RpcError, StatusCode

try:
    response = stub.GetCollection(request)
except RpcError as e:
    # Status Code
    if e.code() == StatusCode.NOT_FOUND:
        print(f"Collection nicht gefunden: {e.details()}")
    elif e.code() == StatusCode.PERMISSION_DENIED:
        print(f"Zugriff verweigert: {e.details()}")
    elif e.code() == StatusCode.DEADLINE_EXCEEDED:
        print(f"Timeout nach {e.details()}")
    elif e.code() == StatusCode.UNAVAILABLE:
        print("Cluster nicht verfügbar, Retry mit Exponential Backoff")
    else:
        # Unbekannter Fehler
        print(f"gRPC Error: {e.code()}, Details: {e.details()}")
        
    # Trailing Metadata (z.B. Request-ID für Debugging)
    metadata = e.trailing_metadata()
    request_id = dict(metadata).get('x-request-id')
    print(f"Request ID: {request_id}")
```

**Rich Error Details mit google.rpc.Status:**

```protobuf
// Error Details (erweiterte Fehlerinformationen)
import "google/rpc/status.proto";
import "google/rpc/error_details.proto";

// Server kann strukturierte Error-Details zurückgeben
message QueryError {
    google.rpc.Status status = 1;
    string query = 2;               // Fehlerhafte Query
    int32 line_number = 3;          // Zeile des Syntaxfehlers
    string suggestion = 4;          // Vorschlag zur Korrektur
}
```

---

## 31.4 GraphQL Integration {#chapter_31_4_graphql-integration}

[GraphQL](../appendix_h_glossary.md#graphql) bietet flexible Query-Sprache mit Client-seitiger Field-Selection, wodurch Over-Fetching und Under-Fetching vermieden werden. Wir analysieren GraphQL-Patterns für ThemisDB, demonstrieren Schema-Definitionen und Resolver-Implementierungen. Die Untersuchung des N+1-Query-Problems und DataLoader-Pattern zeigt kritische Performance-Optimierungen. Real-Time-Subscriptions via WebSocket ermöglichen Live-Queries für Dashboard-Anwendungen. GraphQL eignet sich besonders für Frontend-APIs mit variablen Datenanforderungen, während gRPC für Backend-to-Backend-Communication Performance-optimal ist.

### 31.4.1 Schema Definition Language (SDL) {#chapter_31_4_1_schema-definition-language}

GraphQL-Schemas definieren verfügbare Types, Queries, Mutations und Subscriptions in deklarativer SDL-Syntax. ThemisDB-GraphQL-Schema exponiert Collections als Types mit verschachtelten Relationships. Strong-Typing ermöglicht automatische Validierung und IDE-Autocompletion.

```graphql
# Beispiel 31.11: ThemisDB GraphQL Schema mit deutschen Kommentaren
# Root Query Type (Entry Points für Queries)
type Query {
    # Hole einzelne Collection mit optionalen Filtern
    collection(name: String!): Collection
    
    # Suche über alle Collections mit Fulltext
    searchDocuments(
        query: String!
        limit: Int = 10
        offset: Int = 0
        collections: [String!]  # Optional: Nur bestimmte Collections durchsuchen
    ): DocumentConnection!
    
    # Hole User mit verschachtelten Orders
    user(id: ID!): User
    
    # Liste Users mit Pagination
    users(
        first: Int = 10
        after: String
        filter: UserFilter
    ): UserConnection!
}

# Root Mutation Type (Daten-Modifikationen)
type Mutation {
    # Collection erstellen
    createCollection(input: CreateCollectionInput!): Collection!
    
    # Document einfügen
    insertDocument(
        collection: String!
        document: JSON!
    ): Document!
    
    # Document aktualisieren (Partial Update)
    updateDocument(
        collection: String!
        key: String!
        updates: JSON!
    ): Document!
    
    # Document löschen
    deleteDocument(
        collection: String!
        key: String!
    ): Boolean!
}

# Root Subscription Type (Real-Time Updates)
type Subscription {
    # Echtzeit-Updates für Collection-Änderungen (WebSocket)
    collectionChanged(
        name: String!
        filter: String  # Optional: AQL-Filter-Expression
    ): CollectionChangeEvent!
    
    # Live Query: Results aktualisieren sich automatisch
    liveQuery(
        query: String!
        bindVars: JSON
    ): QueryResult!
}

# Collection Type (Database Collection)
type Collection {
    id: ID!
    name: String!
    type: CollectionType!
    documentCount: Int!
    createdAt: DateTime!
    
    # Verschachtelte Documents mit Pagination
    documents(
        first: Int
        after: String
        filter: String  # AQL-Filter
    ): DocumentConnection!
    
    # Liste aller Indizes
    indexes: [Index!]!
    
    # Statistiken
    stats: CollectionStats!
}

enum CollectionType {
    DOCUMENT
    EDGE
    VECTOR
}

# Document Type (Generic Document Wrapper)
type Document {
    id: ID!
    key: String!
    rev: String!
    collection: Collection!
    
    # JSON-Payload (dynamische Felder)
    data: JSON!
    
    # Timestamps
    createdAt: DateTime
    updatedAt: DateTime
}

# Connection Pattern (Relay-Style Pagination)
type DocumentConnection {
    edges: [DocumentEdge!]!
    pageInfo: PageInfo!
    totalCount: Int!
}

type DocumentEdge {
    node: Document!
    cursor: String!
}

type PageInfo {
    hasNextPage: Boolean!
    hasPreviousPage: Boolean!
    startCursor: String
    endCursor: String
}

# User Type (Domain-Specific)
type User {
    id: ID!
    name: String!
    email: String!
    status: UserStatus!
    createdAt: DateTime!
    
    # Verschachtelte Relationship: User → Orders
    orders(
        first: Int = 10
        status: OrderStatus
    ): OrderConnection!
    
    # Berechnetes Feld (via Resolver)
    orderCount: Int!
}

enum UserStatus {
    ACTIVE
    INACTIVE
    SUSPENDED
}

type Order {
    id: ID!
    orderNumber: String!
    status: OrderStatus!
    total: Float!
    createdAt: DateTime!
    
    # Relationship: Order → User
    user: User!
    
    # Relationship: Order → OrderItems
    items: [OrderItem!]!
}

enum OrderStatus {
    PENDING
    PAID
    SHIPPED
    DELIVERED
    CANCELLED
}

type OrderItem {
    id: ID!
    productName: String!
    quantity: Int!
    price: Float!
}

type OrderConnection {
    edges: [OrderEdge!]!
    pageInfo: PageInfo!
}

type OrderEdge {
    node: Order!
    cursor: String!
}

type UserConnection {
    edges: [UserEdge!]!
    pageInfo: PageInfo!
}

type UserEdge {
    node: User!
    cursor: String!
}

# Index Type
type Index {
    name: String!
    type: IndexType!
    fields: [String!]!
    unique: Boolean!
    sparse: Boolean!
}

enum IndexType {
    HASH
    SKIPLIST
    FULLTEXT
    GEO
    VECTOR
}

# Collection Statistics
type CollectionStats {
    documentCount: Int!
    diskSize: Int!
    indexCount: Int!
    avgDocSize: Int!
}

# Subscription Event Types
type CollectionChangeEvent {
    type: ChangeType!
    collection: String!
    document: Document
    oldDocument: Document  # Bei UPDATE
    timestamp: DateTime!
}

enum ChangeType {
    INSERT
    UPDATE
    DELETE
}

type QueryResult {
    results: [JSON!]!
    executionTime: Int!
    count: Int!
}

# Input Types (für Mutations)
input CreateCollectionInput {
    name: String!
    type: CollectionType!
    options: CollectionOptionsInput
}

input CollectionOptionsInput {
    waitForSync: Boolean
    replicationFactor: Int
    numberOfShards: Int
}

input UserFilter {
    status: UserStatus
    createdAfter: DateTime
    search: String
}

# Custom Scalars
scalar JSON
scalar DateTime
```

### 31.4.2 Resolver Implementation Patterns {#chapter_31_4_2_resolver-implementation}

GraphQL-Resolver sind Funktionen, die Daten für Schema-Felder laden. Wir implementieren Resolver für ThemisDB-Queries mit optimierten Batching-Strategien und Caching. Verschachtelte Relationships (User → Orders) erfordern sorgfältige Query-Optimierung, um N+1-Probleme zu vermeiden.

```python
# Beispiel 31.12: GraphQL Resolver Implementation (Python/Strawberry)
import strawberry
from typing import List, Optional
from themisdb import ThemisDB
from dataloader import DataLoader

db = ThemisDB.connect()

@strawberry.type
class User:
    id: str
    name: str
    email: str
    status: str
    created_at: str
    
    @strawberry.field
    async def orders(
        self,
        info,
        first: int = 10,
        status: Optional[str] = None
    ) -> List['Order']:
        """
        Resolver für User.orders Feld.
        Verwendet DataLoader um N+1-Problem zu vermeiden.
        """
        # DataLoader aus Context (siehe N+1-Section)
        loader = info.context['order_loader']
        
        # Batch-Load Orders für diesen User
        orders = await loader.load(self.id)
        
        # Filter nach Status (optional)
        if status:
            orders = [o for o in orders if o.status == status]
        
        # Limit
        return orders[:first]
    
    @strawberry.field
    def order_count(self) -> int:
        """
        Berechnetes Feld: Anzahl Orders für User.
        Cached via Database-Query mit COUNT.
        """
        result = db.query("""
            FOR order IN orders
                FILTER order.user_id == @user_id
                COLLECT WITH COUNT INTO count
                RETURN count
        """, bind_vars={'user_id': self.id})
        
        return result[0] if result else 0


@strawberry.type
class Query:
    @strawberry.field
    def user(self, id: str) -> Optional[User]:
        """
        Query-Resolver: Hole einzelnen User.
        """
        result = db.query("""
            FOR user IN users
                FILTER user._key == @id
                RETURN user
        """, bind_vars={'id': id})
        
        if not result:
            return None
        
        doc = result[0]
        return User(
            id=doc['_key'],
            name=doc['name'],
            email=doc['email'],
            status=doc['status'],
            created_at=doc['created_at']
        )
    
    @strawberry.field
    def users(
        self,
        first: int = 10,
        after: Optional[str] = None,
        filter: Optional[dict] = None
    ) -> 'UserConnection':
        """
        Query-Resolver: Liste Users mit Pagination.
        Implementiert Cursor-based Pagination (Relay-Style).
        """
        # Build AQL Query dynamisch
        aql_parts = ["FOR user IN users"]
        bind_vars = {'limit': first + 1}  # +1 für hasNextPage-Detection
        
        # Cursor (Base64-encoded _key für Pagination)
        if after:
            import base64
            after_key = base64.b64decode(after).decode()
            aql_parts.append("FILTER user._key > @after_key")
            bind_vars['after_key'] = after_key
        
        # Filter (optional)
        if filter:
            if filter.get('status'):
                aql_parts.append("FILTER user.status == @status")
                bind_vars['status'] = filter['status']
            if filter.get('search'):
                aql_parts.append("FILTER CONTAINS(user.name, @search)")
                bind_vars['search'] = filter['search']
        
        # Sort + Limit
        aql_parts.append("SORT user._key ASC")
        aql_parts.append("LIMIT @limit")
        aql_parts.append("RETURN user")
        
        # Execute Query
        results = db.query('\n'.join(aql_parts), bind_vars=bind_vars)
        
        # Pagination Logic
        has_next_page = len(results) > first
        users = results[:first]  # Trim extra result
        
        # Build Edges
        edges = [
            {
                'node': User(
                    id=u['_key'],
                    name=u['name'],
                    email=u['email'],
                    status=u['status'],
                    created_at=u['created_at']
                ),
                'cursor': base64.b64encode(u['_key'].encode()).decode()
            }
            for u in users
        ]
        
        # PageInfo
        page_info = {
            'hasNextPage': has_next_page,
            'hasPreviousPage': after is not None,
            'startCursor': edges[0]['cursor'] if edges else None,
            'endCursor': edges[-1]['cursor'] if edges else None
        }
        
        return {
            'edges': edges,
            'pageInfo': page_info,
            'totalCount': db.count('users')  # Expensive, cache!
        }


@strawberry.type
class Mutation:
    @strawberry.mutation
    def insert_document(
        self,
        collection: str,
        document: strawberry.scalars.JSON
    ) -> 'Document':
        """
        Mutation-Resolver: Document einfügen.
        """
        result = db.insert(collection, document)
        
        return Document(
            id=result['_id'],
            key=result['_key'],
            rev=result['_rev'],
            data=document
        )
    
    @strawberry.mutation
    def update_document(
        self,
        collection: str,
        key: str,
        updates: strawberry.scalars.JSON
    ) -> 'Document':
        """
        Mutation-Resolver: Document partiell aktualisieren.
        """
        result = db.update(collection, key, updates)
        
        # Hole aktualisiertes Document
        doc = db.document(collection, key)
        
        return Document(
            id=doc['_id'],
            key=doc['_key'],
            rev=doc['_rev'],
            data=doc
        )


@strawberry.type
class Subscription:
    @strawberry.subscription
    async def collection_changed(
        self,
        name: str,
        filter: Optional[str] = None
    ) -> AsyncGenerator['CollectionChangeEvent', None]:
        """
        Subscription-Resolver: Real-Time Collection Changes via WebSocket.
        """
        # Subscribe zu ThemisDB Change Stream
        change_stream = db.watch_collection(name, filter=filter)
        
        # Async Generator für WebSocket-Stream
        async for change in change_stream:
            yield CollectionChangeEvent(
                type=change['type'],
                collection=name,
                document=Document(
                    id=change['document']['_id'],
                    key=change['document']['_key'],
                    rev=change['document']['_rev'],
                    data=change['document']
                ) if change.get('document') else None,
                old_document=Document(
                    id=change['old_document']['_id'],
                    key=change['old_document']['_key'],
                    rev=change['old_document']['_rev'],
                    data=change['old_document']
                ) if change.get('old_document') else None,
                timestamp=change['timestamp']
            )
```

### 31.4.3 N+1 Query Problem und DataLoader Pattern {#chapter_31_4_3_n-plus-1-query-problem}

Das N+1-Problem tritt auf, wenn verschachtelte GraphQL-Queries zu N zusätzlichen Datenbank-Queries führen (1 Query für Parent-Entities + N Queries für jedes Child). Dies kann Latenz um Faktor 10-100 erhöhen. Wir lösen dies mittels DataLoader-Pattern: Batching und Caching von Datenbank-Requests innerhalb eines Request-Kontexts.

**Problem-Demonstration:**

```graphql
# GraphQL Query mit N+1-Problem
query GetUsersWithOrders {
    users(first: 100) {
        edges {
            node {
                id
                name
                orders {  # ← N+1: 1 Query pro User!
                    id
                    total
                }
            }
        }
    }
}
```

**Ohne DataLoader (Naive Implementation):**

```
1. SELECT * FROM users LIMIT 100         # 1 Query (Parent)
2. SELECT * FROM orders WHERE user_id=1  # N Queries (100x)
3. SELECT * FROM orders WHERE user_id=2
   ...
101. SELECT * FROM orders WHERE user_id=100

Total: 101 Queries, ~500ms Latency
```

**Mit DataLoader (Optimiert):**

```
1. SELECT * FROM users LIMIT 100                        # 1 Query
2. SELECT * FROM orders WHERE user_id IN (1,2,...,100)  # 1 Batched Query

Total: 2 Queries, ~50ms Latency (10x schneller!)
```

```python
# Beispiel 31.13: DataLoader Implementation (Python)
from dataloader import DataLoader
from collections import defaultdict

class OrderLoader(DataLoader):
    """
    DataLoader für Orders: Batched Loading + Caching.
    Akkumuliert load()-Calls und führt einen Batch-Query aus.
    """
    
    def __init__(self, db):
        super().__init__()
        self.db = db
    
    async def batch_load_fn(self, user_ids: List[str]) -> List[List[Order]]:
        """
        Batch-Function: Lädt Orders für alle user_ids in einem Query.
        """
        # Einzelner Batch-Query für alle Users
        results = self.db.query("""
            FOR order IN orders
                FILTER order.user_id IN @user_ids
                SORT order.created_at DESC
                RETURN {
                    user_id: order.user_id,
                    order: order
                }
        """, bind_vars={'user_ids': user_ids})
        
        # Group Orders by user_id
        orders_by_user = defaultdict(list)
        for item in results:
            orders_by_user[item['user_id']].append(
                Order(
                    id=item['order']['_key'],
                    order_number=item['order']['order_number'],
                    status=item['order']['status'],
                    total=item['order']['total'],
                    created_at=item['order']['created_at']
                )
            )
        
        # Return in same order as user_ids (DataLoader requirement)
        return [orders_by_user.get(uid, []) for uid in user_ids]


# Usage in GraphQL Context
def get_context():
    """
    Context-Factory: Erstellt DataLoaders pro Request.
    Caching ist Request-scoped (wichtig für Konsistenz).
    """
    db = ThemisDB.connect()
    
    return {
        'db': db,
        'order_loader': OrderLoader(db),
        # Weitere DataLoaders für andere Relationships
        'product_loader': ProductLoader(db),
        'user_loader': UserLoader(db),
    }


# GraphQL Server Setup
schema = strawberry.Schema(query=Query, mutation=Mutation, subscription=Subscription)
app = GraphQL(schema, context_getter=get_context)
```

**DataLoader-Vorteile:**

- **Batching:** N Queries → 1 Batched Query (10-100x schneller)
- **Caching:** Duplicate Loads innerhalb Request werden gecached
- **Request-Scoped:** Cache automatisch pro Request gelöscht (keine Stale Data)
- **Generic:** DataLoader-Pattern funktioniert für alle 1:N Relationships

### 31.4.4 Real-Time Subscriptions mit WebSocket {#chapter_31_4_4_realtime-subscriptions}

GraphQL-Subscriptions ermöglichen Server-to-Client Push via [WebSocket](../appendix_h_glossary.md#websocket) für Real-Time-Dashboards, Live-Queries und Notifications. ThemisDB implementiert Subscriptions basierend auf Change Data Capture (CDC) Streams aus dem Storage-Layer. Wir verwenden GraphQL-over-WebSocket Protocol (graphql-ws) für standardisierte Client-Server-Communication.

```typescript
// Beispiel 31.14: GraphQL Subscription Client (TypeScript/React)
import { createClient } from 'graphql-ws';
import { useSubscription, gql } from '@apollo/client';

// WebSocket Client erstellen
const wsClient = createClient({
    url: 'wss://api.themisdb.io/graphql',
    connectionParams: {
        // Authentication via WebSocket Handshake
        authToken: localStorage.getItem('jwt_token')
    }
});

// Subscription Query Definition
const COLLECTION_CHANGED_SUBSCRIPTION = gql`
    subscription OnCollectionChanged($collectionName: String!) {
        collectionChanged(name: $collectionName) {
            type
            collection
            document {
                id
                key
                data
            }
            oldDocument {
                id
                data
            }
            timestamp
        }
    }
`;

// React Component mit Subscription
function LiveOrdersDashboard() {
    // useSubscription Hook: Automatisches WebSocket-Management
    const { data, loading, error } = useSubscription(
        COLLECTION_CHANGED_SUBSCRIPTION,
        {
            variables: { collectionName: 'orders' }
        }
    );
    
    // State für Orders-Liste
    const [orders, setOrders] = React.useState([]);
    
    // Update Orders bei Change-Event
    React.useEffect(() => {
        if (!data) return;
        
        const change = data.collectionChanged;
        
        // INSERT: Neues Order hinzufügen
        if (change.type === 'INSERT') {
            const newOrder = JSON.parse(change.document.data);
            setOrders(prev => [newOrder, ...prev]);
            
            // Notification anzeigen
            showNotification(`Neue Bestellung: ${newOrder.order_number}`);
        }
        
        // UPDATE: Bestehendes Order aktualisieren
        else if (change.type === 'UPDATE') {
            const updatedOrder = JSON.parse(change.document.data);
            setOrders(prev => prev.map(o => 
                o._key === change.document.key ? updatedOrder : o
            ));
        }
        
        // DELETE: Order entfernen
        else if (change.type === 'DELETE') {
            setOrders(prev => prev.filter(o => o._key !== change.document.key));
        }
    }, [data]);
    
    if (loading) return <div>Connecting to live stream...</div>;
    if (error) return <div>Error: {error.message}</div>;
    
    return (
        <div className="live-dashboard">
            <h2>Live Orders (WebSocket)</h2>
            <div className="orders-list">
                {orders.map(order => (
                    <OrderCard key={order._key} order={order} />
                ))}
            </div>
        </div>
    );
}
```

**WebSocket Lifecycle für GraphQL Subscriptions:**

```
Client                                    Server
  |                                         |
  |-- WebSocket Handshake ---------------->|
  |<- HTTP 101 Switching Protocols --------|
  |                                         |
  |-- connection_init (auth) ------------->|
  |<- connection_ack ----------------------|
  |                                         |
  |-- subscribe (GraphQL query) ---------->|
  |                                         |  [Server starts CDC stream]
  |                                         |
  |<- next (change event 1) ---------------|
  |<- next (change event 2) ---------------|
  |<- next (change event 3) ---------------|
  |                                         |
  |-- complete (unsubscribe) ------------->|
  |                                         |  [Server stops CDC stream]
  |<- complete ----------------------------|
  |                                         |
  |-- connection_terminate --------------->|
  |<- WebSocket Close ---------------------|
```

---

## 31.5 HTTP/2 Features {#chapter_31_5_http2-features}

### 31.5.1 Multiplexing & Header-Kompression {#chapter_31_5_1_multiplexing-header-kompression}

```mermaid
sequenceDiagram
    participant C as Client
    participant S as Themis API
    C->>S: Stream 1 - GET /_api/version
    C->>S: Stream 3 - POST /_api/query (AQL)
    C->>S: Stream 5 - GET /_api/metrics
    Note over C,S: Alle Streams teilen eine TCP-Verbindung
    S-->>C: Response Stream 1
    S-->>C: Response Stream 3
    S-->>C: Response Stream 5
```

Abb. 31.1: API-Protocol-Stack

**Vorteile:**
- Keine Head-of-Line-Blocking auf Applikationsebene
- HPACK reduziert Header-Overhead (Auth, Tenant-ID)
- Bessere Ausnutzung einzelner TCP-Verbindung

### 31.5.2 Server Push und Stream-Prioritization {#chapter_31_5_2_server-push-stream-prioritization}

[HTTP/2](../appendix_h_glossary.md#http2) führt Server Push ein, wodurch Server proaktiv Ressourcen senden kann, bevor Client diese anfordert. Für API-Anwendungen ist Push selten relevant, aber für Admin-UIs und Dashboards kann es First-Paint-Latenz reduzieren. Stream-Prioritization ermöglicht explizite Wichtung von Requests – kritische Health-Checks erhalten höhere Priorität als Long-Running Analytics-Queries.

```http
# HTTP/2 Server Push Example
:method: GET
:path: /dashboard
:authority: api.themis.local
```

Server antwortet mit gepushten Ressourcen (nur wenn vom Client via SETTINGS_ENABLE_PUSH erlaubt):
- `/static/dashboard.css` (Priority: High)
- `/static/dashboard.js` (Priority: High)
- `/static/logo.svg` (Priority: Low)

**Best Practice für ThemisDB-APIs:**
- **API-Endpoints:** Server Push deaktiviert (unnötiger Overhead)
- **Admin-UI/Cockpit:** Server Push für kritische Assets (CSS, Core-JS)
- **Monitoring-Dashboards:** Push nur bei <5 kritischen Assets

**Stream Priorities für ThemisDB-Operations:**

| Operation | Priority | Weight | Rationale |
|-----------|----------|--------|-----------|
| Health/Readiness Checks | Highest | 256 | Load Balancer Heartbeats |
| Authentication Requests | High | 128 | Blockiert weitere Requests |
| Short Queries (<100ms) | Medium | 64 | Interactive Workloads |
| Long-Running Analytics | Low | 16 | Background Processing |
| Metrics Export | Lowest | 8 | Non-Critical, Bulk |

### 31.5.3 HPACK Header Compression {#chapter_31_5_3_hpack-header-compression}

HTTP/2 verwendet [HPACK](../appendix_h_glossary.md#hpack)-Kompression für Headers, wodurch Redundanz eliminiert und Overhead dramatisch reduziert wird. HPACK kombiniert Static Table (vordefinierte häufige Headers), Dynamic Table (Request-spezifische Werte) und Huffman-Encoding. Bei ThemisDB-APIs mit großen `Authorization`-Headern (JWT-Tokens) reduziert HPACK typische Header-Größe um 70-85%.

**Header-Overhead-Vergleich:**

| Scenario | HTTP/1.1 | HTTP/2 (HPACK) | Reduction |
|----------|----------|----------------|-----------|
| Initial Request (Cold) | 850 bytes | 450 bytes | -47% |
| Subsequent Request (Warm) | 850 bytes | 120 bytes | -86% |
| With JWT Token (512b) | 1400 bytes | 180 bytes | -87% |

**HPACK Dynamic Table Example:**

```
Request 1:
  :authority: api.themisdb.io
  :method: GET
  :path: /api/v1/collections
  authorization: Bearer eyJhbGc...  (512 bytes)
  
  → Sent: 850 bytes (full headers)
  → Dynamic Table stores: authorization header

Request 2:
  :authority: api.themisdb.io      → Index 1 (from Dynamic Table)
  :method: POST                     → Index 3 (from Static Table)
  :path: /api/v1/documents          → 24 bytes
  authorization: <index 62>         → 1 byte (reference)
  
  → Sent: 120 bytes (mostly indexes!)
  → Compression: 86%
```

### 31.5.4 HTTP/2 Server Push für CDC-Streaming {#chapter_31_5_4_server-push-cdc}

**Change Data Capture (CDC) mit Server Push** ist ein leistungsstarkes Pattern für Echtzeit-Datensynchronisation ohne Polling-Overhead. HTTP/2 Server Push ermöglicht proaktive Event-Zustellung vom Server zum Client.

**Architektur:**

```mermaid
sequenceDiagram
    participant C as Client
    participant T as ThemisDB HTTP/2
    participant D as Database Engine
    
    C->>T: GET /cdc/subscribe<br/>(HTTP/2 with ALPN h2)
    T-->>C: 200 OK: {status: "subscribed"}
    Note over T,D: Client wartet auf Events
    
    D->>T: DB Change Event<br/>(INSERT/UPDATE/DELETE)
    T->>C: PUSH_PROMISE<br/>:path /cdc/event/123
    T->>C: HEADERS + DATA<br/>CDC Event JSON
    
    D->>T: Another Change Event
    T->>C: PUSH_PROMISE<br/>:path /cdc/event/124
    T->>C: HEADERS + DATA<br/>CDC Event JSON
    
    Note over C,T: No Polling - Events pushed immediately
```

Abb. 31.5.4: CDC mit HTTP/2 Server Push

**Vorteile gegenüber Polling:**

| Metric | HTTP/1.1 Polling | HTTP/2 Server Push |
|--------|------------------|---------------------|
| **Latenz** | ~500ms (Poll-Intervall) | <10ms (Echtzeit) |
| **Requests/min** | 120 (2 Hz Polling) | 1 (Subscribe) + Events |
| **Bandbreite** | Hoch (leere Polls) | Niedrig (nur Events) |
| **Server-Last** | Hoch (ständige Polls) | Niedrig (Event-driven) |

**Beispiel 1: JavaScript Browser Client**

```javascript
// HTTP/2 CDC Client (Browser)
async function subscribeCDC() {
    // HTTP/2 wird automatisch genutzt wenn Server & Browser es unterstützen
    const response = await fetch('https://themisdb.local:8443/cdc/subscribe', {
        method: 'GET',
        headers: {
            'Authorization': 'Bearer ...',
            'Accept': 'application/json'
        }
    });
    
    if (response.ok) {
        console.log('CDC Subscription active');
        
        // Client wartet jetzt - Server pushed Events automatisch
        // Events kommen über Server Push als separate Responses
    }
}

// Event Handler für gepushte CDC Events
if ('serviceWorker' in navigator) {
    navigator.serviceWorker.addEventListener('push', event => {
        const cdcEvent = event.data.json();
        console.log('CDC Event received:', cdcEvent);
        
        // Event verarbeiten
        updateUI(cdcEvent);
    });
}

subscribeCDC();
```

**Beispiel 2: Node.js Client mit http2**

```javascript
// Node.js HTTP/2 CDC Client
const http2 = require('http2');
const fs = require('fs');

const client = http2.connect('https://themisdb.local:8443', {
    ca: fs.readFileSync('ca.pem')
});

// Subscribe zu CDC Events
const req = client.request({
    ':method': 'GET',
    ':path': '/cdc/subscribe',
    'authorization': 'Bearer ...'
});

req.on('response', (headers) => {
    console.log('Subscription status:', headers[':status']);
});

// Empfange gepushte CDC Events
client.on('stream', (pushedStream, requestHeaders) => {
    console.log('Server pushed:', requestHeaders[':path']);
    
    let data = '';
    pushedStream.on('data', chunk => {
        data += chunk;
    });
    
    pushedStream.on('end', () => {
        const cdcEvent = JSON.parse(data);
        console.log('CDC Event:', cdcEvent);
        
        // Event verarbeiten
        processEvent(cdcEvent);
    });
});

req.end();
```

**Beispiel 3: curl Test (mit nghttp)**

```bash
# HTTP/2 CDC Subscribe mit nghttp (curl unterstützt Server Push nicht direkt)
nghttp -v https://themisdb.local:8443/cdc/subscribe \
    -H "Authorization: Bearer ..." \
    --timeout=60

# Output zeigt gepushte Events:
# [  0.012] recv (stream_id=1) :status: 200
# [  0.012] recv (stream_id=1) {"status": "subscribed"}
# [  1.523] recv PUSH_PROMISE (stream_id=2)
# [  1.523] recv (stream_id=2) :status: 200
# [  1.523] recv (stream_id=2) {"type": "cdc_event", "sequence": 123, ...}
# [  2.891] recv PUSH_PROMISE (stream_id=4)
# [  2.891] recv (stream_id=4) :status: 200
# [  2.891] recv (stream_id=4) {"type": "cdc_event", "sequence": 124, ...}
```

**Server-Implementierung (C++ mit nghttp2):**

```cpp
// ThemisDB HTTP/2 Server Push CDC Handler
void CDCHandler::onDatabaseChange(const ChangeEvent& event) {
    // Serialisiere CDC Event zu JSON
    json cdcEvent = {
        {"type", "cdc_event"},
        {"sequence", event.sequence},
        {"key", event.key},
        {"value", event.value},
        {"operation", event.operation},  // INSERT, UPDATE, DELETE
        {"timestamp", event.timestamp}
    };
    
    std::string jsonData = cdcEvent.dump();
    
    // Pushe Event zu allen subscribten Clients
    for (auto& [streamId, client] : subscribedClients) {
        // PUSH_PROMISE senden
        std::string pushPath = "/cdc/event/" + std::to_string(event.sequence);
        
        nghttp2_nv headers[] = {
            {(uint8_t*)":method", (uint8_t*)"GET", 7, 3},
            {(uint8_t*)":path", (uint8_t*)pushPath.c_str(), 5, pushPath.size()},
            {(uint8_t*)":scheme", (uint8_t*)"https", 7, 5},
            {(uint8_t*)":authority", (uint8_t*)"themisdb.local", 10, 14},
            {(uint8_t*)"content-type", (uint8_t*)"application/json", 12, 16},
            {(uint8_t*)"x-cdc-sequence", (uint8_t*)std::to_string(event.sequence).c_str(), 14, 0}
        };
        
        int32_t pushedStreamId = nghttp2_submit_push_promise(
            client->session, NGHTTP2_FLAG_NONE,
            streamId, headers, 6, nullptr
        );
        
        // Response Data senden
        nghttp2_data_provider dataProvider = {
            .source = {.ptr = &jsonData},
            .read_callback = [](nghttp2_session*, int32_t, uint8_t* buf,
                               size_t length, uint32_t*, nghttp2_data_source* source,
                               void*) -> ssize_t {
                std::string* data = static_cast<std::string*>(source->ptr);
                size_t nread = std::min(length, data->size());
                std::memcpy(buf, data->c_str(), nread);
                return nread;
            }
        };
        
        nghttp2_submit_response(client->session, pushedStreamId,
                               headers + 3, 3, &dataProvider);
    }
}
```

**CDC Event Format:**

```json
{
  "type": "cdc_event",
  "sequence": 123,
  "collection": "users",
  "key": "user:1001",
  "operation": "UPDATE",
  "value": {
    "_key": "user:1001",
    "name": "Alice Smith",
    "email": "alice@example.com",
    "updated_at": "2026-01-25T10:30:00Z"
  },
  "old_value": {
    "name": "Alice Jones",
    "email": "alice@example.com"
  },
  "timestamp": "2026-01-25T10:30:00.123Z",
  "txid": "txn_abc123"
}
```

**Performance-Vergleich: Polling vs. Server Push**

Szenario: 1000 concurrent Clients, 10 Events/sec pro Client

| Metric | HTTP/1.1 Polling (1 Hz) | HTTP/2 Server Push |
|--------|--------------------------|---------------------|
| Requests/sec | 1,000 (Polls) | ~10 (nur Events) |
| Bandbreite | ~100 MB/s (leere Polls) | ~1 MB/s (Events) |
| Server CPU | 85% (Poll-Handling) | 15% (Event-Push) |
| Client Latenz | 500ms (avg) | 8ms (avg) |

**Best Practices:**

1. **Client Reconnection**: Bei Verbindungsabbruch auto-reconnect mit Exponential Backoff
2. **Sequence Numbers**: Clients tracken letzte Sequence um verlorene Events zu erkennen
3. **Heartbeats**: Server sendet Keepalive-Events (alle 30s) um Idle-Connections zu halten
4. **Backpressure**: Server limitiert Push-Rate wenn Client nicht schnell genug verarbeitet
5. **Security**: TLS mit ALPN "h2" erforderlich, JWT Auth für Subscribe-Endpoint

**Konfiguration:**

```yaml
# config/http2_cdc.yaml
http2:
  server_push:
    enabled: true
    max_pushed_streams: 100  # Pro Client-Connection
    max_clients: 10000
    
  cdc:
    buffer_size: 1000  # Events gepuffert bei langsamen Clients
    timeout_seconds: 60  # Auto-unsubscribe bei Inaktivität
    heartbeat_interval: 30  # Keepalive Events
```

**Monitoring:**

```promql
# Prometheus Metrics für CDC Server Push
http2_cdc_subscriptions_active{instance="themisdb-01"}  # Aktive Subscriptions
http2_cdc_events_pushed_total{instance="themisdb-01"}   # Gesamt gepushte Events
http2_cdc_push_latency_seconds{instance="themisdb-01"}  # Push-Latenz
http2_server_push_streams_active{instance="themisdb-01"} # Aktive Push-Streams
```

**Siehe auch:**
- Section 31.7: WebSocket vs. SSE (Alternative für ältere Clients)
- Section 31.8: Security Best Practices (TLS, Auth)
- `docs/de/apis/HTTP2_SERVER_PUSH_CDC.md` - Detaillierte Implementierung

---

## 31.6 HTTP/3 und QUIC {#chapter_31_6_http3-quic}

[HTTP/3](../appendix_h_glossary.md#http3) basiert auf [QUIC](../appendix_h_glossary.md#quic) (Quick UDP Internet Connections), einem UDP-basierten Transport-Protokoll mit integriertem TLS 1.3 und Multiplexing. Wir analysieren systematisch die Vorteile gegenüber HTTP/2 über TCP, insbesondere bei Paketverlusten und hohen Latenzen. QUIC eliminiert Head-of-Line-Blocking auf Transport-Layer und ermöglicht 0-RTT Connection Resumption. ThemisDB-Deployments in WAN-Szenarien (Multi-Region, CDN) profitieren messbar von HTTP/3, während LAN-Deployments marginale Unterschiede zeigen.

### 31.6.1 QUIC vs TCP: Fundamentale Unterschiede {#chapter_31_6_1_quic-vs-tcp}

| Merkmal | TCP/TLS | QUIC | Impact |
|---------|---------|------|--------|
| **Verbindungsaufbau** | 1-2 RTT | 0-1 RTT (0-RTT Resumption) | Schnellerer Start |
| **Head-of-Line Blocking** | End-to-End | Per-Stream | Bessere Resilienz |
| **Congestion Control** | Reno/Cubic | BBR, Hybrid | Höherer Throughput |
| **Connection Migration** | Neuaufbau nötig | Connection IDs | Mobile-Friendly |
| **Packet Loss Recovery** | Retransmit all | Selective | Weniger Overhead |
| **TLS Integration** | Separate Layer | Native (TLS 1.3) | Weniger Handshakes |

**Head-of-Line Blocking Explained:**

HTTP/2 über TCP: Paketverlust blockiert **alle** Streams, da TCP Reihenfolge garantiert.

```
Stream 1: [Packet 1] [Packet 2 LOST] [Packet 3]  ← Blockiert
Stream 2: [Packet 4] [Packet 5] [Packet 6]      ← Blockiert (warten auf Packet 2)
Stream 3: [Packet 7] [Packet 8]                  ← Blockiert

→ Latency Spike: +200ms bei 1% Packet Loss
```

HTTP/3 über QUIC: Paketverlust blockiert nur **betroffenen** Stream.

```
Stream 1: [Packet 1] [Packet 2 LOST] [Packet 3]  ← Blockiert
Stream 2: [Packet 4] [Packet 5] [Packet 6]      ← OK (parallel)
Stream 3: [Packet 7] [Packet 8]                  ← OK (parallel)

→ Latency Spike: +50ms (nur Stream 1 betroffen)
```

### 31.6.2 QUIC Performance-Tuning {#chapter_31_6_2_quic-performance-tuning}

QUIC-Implementation erfordert spezifisches Tuning für optimale Performance in unterschiedlichen Netzwerk-Bedingungen. Wir präsentieren empirisch validierte Parameter für Low-Latency (LAN), High-Latency (WAN) und Lossy Networks (Mobile).

```yaml
# themis-config.yaml: QUIC-Konfiguration für verschiedene Szenarien
http3:
  enabled: true
  listen_address: "0.0.0.0:443"
  
  # Connection Parameters
  max_idle_timeout: 30s               # Keep-Alive Timeout
  max_concurrent_streams: 250         # Parallel Requests pro Connection
  max_stream_receive_window: 6MB      # Flow Control per Stream
  max_connection_receive_window: 15MB # Flow Control per Connection
  
  # Congestion Control Algorithm
  congestion_control: "bbr"           # bbr, reno, cubic
  # BBR: Best für High-BW, High-Latency (WAN, Intercontinental)
  # Cubic: Best für Low-Latency (LAN, Data Center)
  
  # 0-RTT Configuration (Careful: Replay Attack Risk)
  enable_0rtt: true                   # Nur für idempotente Requests (GET)
  max_early_data: 16KB                # Max Data in 0-RTT
  
  # Initial Parameters
  initial_max_data: 10MB              # Initial Connection Flow Window
  initial_congestion_window: 10       # Initial CWND (packets)
  
  # Packet Size
  max_udp_payload_size: 1350          # MTU - overhead (avoid fragmentation)
  disable_path_mtu_discovery: false   # Auto-detect optimal MTU
  
  # Loss Detection
  packet_threshold: 3                 # Declare loss after 3 out-of-order
  time_threshold: 1.125               # Time-based loss detection multiplier
  
  # Tuning für spezifische Szenarien:
  scenarios:
    lan:  # Low-Latency, High-BW, <1ms RTT
      congestion_control: "cubic"
      initial_congestion_window: 32
      
    wan:  # High-Latency, Variable-BW, 50-200ms RTT
      congestion_control: "bbr"
      initial_congestion_window: 10
      max_idle_timeout: 60s
      
    mobile:  # High-Loss, Variable-RTT, 50-500ms
      congestion_control: "bbr"
      packet_threshold: 5             # Mehr Toleranz für Reordering
      enable_0rtt: true               # Connection Migration wichtig
```

**Performance-Messwerte (Empirisch):**

| Network Condition | HTTP/2 (TCP) | HTTP/3 (QUIC) | Improvement |
|-------------------|--------------|---------------|-------------|
| **LAN (1ms RTT, 0% loss)** | 8ms | 7ms | +12% |
| **WAN (50ms RTT, 0% loss)** | 120ms | 85ms | +29% |
| **Lossy (50ms RTT, 1% loss)** | 380ms | 145ms | +62% |
| **Mobile (variable RTT, 3% loss)** | 850ms | 280ms | +67% |

### 31.6.3 Graceful Degradation und Fallback {#chapter_31_6_3_graceful-degradation-fallback}

Produktive Deployments müssen HTTP/3-Failures graceful handhaben, da nicht alle Netzwerke UDP Port 443 erlauben (Corporate Firewalls, restriktive NATs). Wir implementieren Fallback-Logic mit automatischer Protokoll-Negotiation via Alt-Svc Header.

```mermaid
flowchart TD
    A[Client Request] --> B{UDP Port 443 erreichbar?}
    B -->|Ja| C[Versuche HTTP/3 via QUIC]
    B -->|Nein| D[Fallback zu HTTP/2]
    C --> E{QUIC Handshake erfolgreich?}
    E -->|Ja| F{Packet Loss > 5%?}
    E -->|Nein| D
    F -->|Ja| D
    F -->|Nein| G[Verwende HTTP/3]
    style G fill:#e1f5ff
    style D fill:#fff4e1
```

**Alt-Svc Header für Protocol Discovery:**

```http
# Server sendet Alt-Svc Header in HTTP/2 Response
HTTP/2 200 OK
Alt-Svc: h3=":443"; ma=86400

# Client versucht beim nächsten Request HTTP/3
# Falls erfolgreich: Verwendet HTTP/3 für 86400s (24h)
# Falls Failure: Bleibt bei HTTP/2
```

---

## 31.7 Echtzeit-Kommunikation: WebSocket vs SSE {#chapter_31_7_echtzeit-websocket-sse}

Für Echtzeit-Anforderungen bietet ThemisDB zwei komplementäre Protokolle: [WebSocket](../appendix_h_glossary.md#websocket) für bidirektionale Full-Duplex-Communication und [Server-Sent Events (SSE)](../appendix_h_glossary.md#sse) für unidirektionale Server-to-Client-Streams. Wir analysieren Trade-Offs systematisch und demonstrieren Implementierungsmuster für Change Data Capture (CDC), Live Queries und Notification-Systeme. WebSocket eignet sich für interaktive Anwendungen (Chat, Collaborative Editing), während SSE für Event-Streams (Monitoring, Logs) simpler und robuster ist.

### 31.7.1 Protokoll-Auswahl: WebSocket vs SSE {#chapter_31_7_1_protokoll-auswahl}

| Merkmal | TCP/TLS | QUIC |
|---------|---------|------|
| Verbindungsaufbau | 1-2 RTT | 0-1 RTT (0-RTT Resumption) |
| Head-of-Line | End-to-End | Per-Stream |
| Congestion Control | Reno/Cubic | BBR, Hybrid |
| Mobility | Neuaufbau nötig | Connection IDs erlauben Migration |

### 31.3.2 QUIC-Tuning

- **Initial Congestion Window:** 10-20 MSS für schnellere Warmups
- **BBR aktivieren:** Für WAN/hohe Bandbreite
- **Handshake-Keys cachen:** 0-RTT für interne Service-zu-Service Calls
- **Pacing:** Aktivieren, um Burst-Loss zu vermeiden

### 31.3.3 Fallback-Strategie

```mermaid
flowchart TD
    A[Client Request] --> B{Supports HTTP/3?}
    B -->|Ja| C[Connect via QUIC]
    B -->|Nein| D[Fallback HTTP/2]
    C --> E{Loss > 5 percent?}
    E -->|Ja| D
    E -->|Nein| F[Continue HTTP/3]
    style C fill:#e1f5ff
    style D fill:#fff4e1
```

Abb. 31.2: Request-Response-Flow

---

## 31.4 Echtzeit: WebSocket vs SSE

### 31.4.1 Wann welches Protokoll?

| Bedarf | WebSocket | SSE | Empfehlung |
|--------|-----------|-----|------------|
| **Bidirektional** | ✅ Full-Duplex | ❌ Server→Client only | WebSocket für Chat, Gaming |
| **Firewalls/Proxies** | Oft blockiert | Fast immer offen (HTTP) | SSE für Enterprise |
| **Browser Support** | Excellent (95%+) | Excellent (95%+) | Beide OK |
| **Backpressure** | Manual | Implicit (HTTP Flow Control) | SSE einfacher |
| **Binärdaten** | ✅ Native | ❌ Text-only (Base64 möglich) | WebSocket für Blobs |
| **Reconnect** | Manual | Automatic (EventSource) | SSE robuster |
| **Message Framing** | WebSocket Protocol | HTTP Chunked Transfer | - |
| **Overhead** | Low (2-14 bytes/frame) | Medium (HTTP headers) | WebSocket effizienter |

**Decision Matrix:**

```
Use WebSocket wenn:
  - Bidirektionale Communication nötig (Client sendet auch)
  - Low-Latency kritisch (<10ms)
  - Binärdaten (Images, Video, Audio)
  - Gaming, Real-Time Collaboration

Use SSE wenn:
  - Nur Server→Client Push (Logs, Metrics, Notifications)
  - Enterprise-Firewalls (HTTP-Only Environments)
  - Automatic Reconnect gewünscht
  - Event-Streams, CDC, Monitoring
```

### 31.7.2 Server-Sent Events (SSE) für Change Feeds {#chapter_31_7_2_sse-change-feeds}

SSE bietet simpelste Implementierung für Server-to-Client Event-Streams über Standard-HTTP. ThemisDB verwendet SSE für Change Data Capture (CDC), Log-Streaming und Monitoring-Events. Der Browser-native `EventSource`-API handhabt Reconnects automatisch mit Exponential Backoff.

```http
# SSE Request
GET /_api/changefeed?collection=orders&since=1735600000000 HTTP/1.1
Accept: text/event-stream
Authorization: Bearer <token>
Connection: keep-alive
Cache-Control: no-cache

# SSE Response
HTTP/1.1 200 OK
Content-Type: text/event-stream
Cache-Control: no-cache
Connection: keep-alive

event: insert
data: {"_key":"o123","status":"paid","total":299.99}

event: update
data: {"_key":"o124","status":"shipped"}

retry: 3000

event: delete
data: {"_key":"o125"}
```

```javascript
// Beispiel 31.15: SSE Client Implementation (JavaScript mit deutschen Kommentaren)
// Browser-native EventSource API mit automatischem Reconnect
const eventSource = new EventSource(
    '/_api/changefeed?collection=orders&since=' + lastEventId,
    {
        withCredentials: true  // Send cookies/auth
    }
);

// Event-Listener für spezifische Event-Types
eventSource.addEventListener('insert', (event) => {
    const newOrder = JSON.parse(event.data);
    console.log('Neue Bestellung:', newOrder);
    updateUI(newOrder);
    
    // Event-ID speichern für Resume nach Disconnect
    lastEventId = event.lastEventId;
});

eventSource.addEventListener('update', (event) => {
    const updatedOrder = JSON.parse(event.data);
    console.log('Bestellung aktualisiert:', updatedOrder);
    updateUI(updatedOrder);
});

eventSource.addEventListener('delete', (event) => {
    const deletedOrder = JSON.parse(event.data);
    console.log('Bestellung gelöscht:', deletedOrder._key);
    removeFromUI(deletedOrder._key);
});

// Error-Handling und Reconnect
eventSource.onerror = (error) => {
    if (eventSource.readyState === EventSource.CLOSED) {
        console.log('Connection closed by server');
    } else {
        console.log('Connection error, will retry:', error);
        // EventSource reconnect automatisch mit Exponential Backoff
        // Retry: 3000ms (wie vom Server via "retry:" spezifiziert)
    }
};

// Open-Event (erfolgreich connected)
eventSource.onopen = () => {
    console.log('SSE Connection established');
};

// Cleanup
window.addEventListener('beforeunload', () => {
    eventSource.close();
});
```

**SSE Server-Side Implementation (Python/Flask):**

```python
# Beispiel 31.16: SSE Server für Change Data Capture
from flask import Flask, Response, request
import json, time

app = Flask(__name__)

@app.route('/_api/changefeed')
def changefeed():
    """
    Server-Sent Events Endpoint für Collection-Changes.
    Streamt Events im text/event-stream Format.
    """
    collection = request.args.get('collection', 'orders')
    since = request.args.get('since', type=int, default=0)
    
    def generate_events():
        """Generator für SSE Events"""
        
        # Sende Retry-Interval (milliseconds)
        yield 'retry: 3000\n\n'
        
        # Subscribe zu ThemisDB Change Stream
        change_stream = db.watch_collection(collection, since=since)
        
        for change in change_stream:
            # Event-Type (insert, update, delete)
            event_type = change['type'].lower()
            
            # Event-Data als JSON
            event_data = json.dumps({
                '_key': change['document']['_key'],
                **change['document']
            })
            
            # SSE Event-Format
            # "event: <type>\ndata: <json>\nid: <event_id>\n\n"
            yield f"event: {event_type}\n"
            yield f"data: {event_data}\n"
            yield f"id: {change['event_id']}\n\n"
            
            # Heartbeat alle 15s (verhindert Timeout)
            # Sende Comment (": <text>") als Keep-Alive
            if time.time() % 15 < 1:
                yield ': heartbeat\n\n'
    
    # Response mit SSE-Headers
    return Response(
        generate_events(),
        mimetype='text/event-stream',
        headers={
            'Cache-Control': 'no-cache',
            'Connection': 'keep-alive',
            'X-Accel-Buffering': 'no'  # Nginx: Disable buffering
        }
    )
```

**SSE-Vorteile:**
- ✅ Simpelste Implementation (Standard HTTP)
- ✅ Automatic Reconnect mit Last-Event-ID
- ✅ Firewall-friendly (Port 80/443)
- ✅ Text-based, human-readable
- ✅ Browser-native API (EventSource)

### 31.7.3 WebSocket für Bidirektionale Communication {#chapter_31_7_3_websocket-bidirektional}

[WebSocket](../appendix_h_glossary.md#websocket) ermöglicht Full-Duplex-Communication über persistente TCP-Connection. ThemisDB verwendet WebSocket für interaktive Use Cases: Live-Queries mit Client-seitigen Filtern, Real-Time Collaboration und bidirektionale Command-Execution. Der Upgrade-Handshake erfolgt via HTTP, danach switcht Connection zu WebSocket-Framing-Protocol.

**WebSocket Handshake und Upgrade:**

```http
# Client Request (HTTP Upgrade)
GET /_ws HTTP/1.1
Host: api.themis.local
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
Sec-WebSocket-Version: 13
Sec-WebSocket-Protocol: themisdb-v1

# Server Response (Switching Protocols)
HTTP/1.1 101 Switching Protocols
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
Sec-WebSocket-Protocol: themisdb-v1

# Connection ist jetzt WebSocket (bidirektional)
```

```javascript
// Beispiel 31.17: WebSocket Client für bidirektionale Commands
const ws = new WebSocket('wss://api.themis.local/_ws');

ws.onopen = () => {
    console.log('WebSocket connected');
    
    // Subscribe zu Collection-Änderungen mit Filter
    ws.send(JSON.stringify({
        type: 'subscribe',
        collection: 'orders',
        filter: {status: 'pending'},
        options: {
            includeOldValue: true,  // Bei UPDATE: Alte + neue Werte
            batchSize: 10           // Max 10 Events pro Batch
        }
    }));
    
    // Parallel: Execute Command
    ws.send(JSON.stringify({
        type: 'query',
        id: 'query-1',
        query: 'FOR o IN orders FILTER o.status == "pending" RETURN o',
        bindVars: {}
    }));
};

ws.onmessage = (event) => {
    const msg = JSON.parse(event.data);
    
    // Message-Routing basierend auf Type
    switch(msg.type) {
        case 'change':
            // CDC Event
            console.log(`Change: ${msg.operation} on ${msg.document._key}`);
            handleChangeEvent(msg);
            break;
            
        case 'query_result':
            // Query Response
            console.log(`Query ${msg.id} completed:`, msg.results);
            handleQueryResult(msg);
            break;
            
        case 'error':
            // Error von Server
            console.error(`Error: ${msg.error.message}`);
            break;
            
        case 'pong':
            // Heartbeat Response
            lastPongTime = Date.now();
            break;
    }
};

ws.onerror = (error) => {
    console.error('WebSocket error:', error);
};

ws.onclose = (event) => {
    console.log(`WebSocket closed: ${event.code} - ${event.reason}`);
    
    // Reconnect mit Exponential Backoff
    if (event.code !== 1000) {  // 1000 = Normal Closure
        reconnectWithBackoff();
    }
};

// Heartbeat: Ping alle 30s um Connection alive zu halten
setInterval(() => {
    if (ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({type: 'ping'}));
    }
}, 30000);

// Reconnect Logic mit Exponential Backoff
let reconnectAttempts = 0;
const maxReconnectDelay = 30000;  // 30s max

function reconnectWithBackoff() {
    reconnectAttempts++;
    
    // Exponential Backoff: 1s, 2s, 4s, 8s, 16s, 30s (max)
    const delay = Math.min(
        1000 * Math.pow(2, reconnectAttempts - 1),
        maxReconnectDelay
    );
    
    console.log(`Reconnecting in ${delay}ms (attempt ${reconnectAttempts})`);
    
    setTimeout(() => {
        // Neue WebSocket Connection
        connectWebSocket();
    }, delay);
}
```

**Message Framing und Opcodes:**

WebSocket-Frames haben minimalen Overhead (2-14 bytes per Message):

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-------+-+-------------+-------------------------------+
|F|R|R|R| opcode|M| Payload len |    Extended payload length    |
|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
|N|V|V|V|       |S|             |   (if payload len==126/127)   |
| |1|2|3|       |K|             |                               |
+-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
|     Extended payload length continued, if payload len == 127  |
+ - - - - - - - - - - - - - - - +-------------------------------+
|                               |Masking-key, if MASK set to 1  |
+-------------------------------+-------------------------------+
| Masking-key (continued)       |          Payload Data         |
+-------------------------------- - - - - - - - - - - - - - - - +
:                     Payload Data continued ...                :
+ - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
|                     Payload Data continued ...                |
+---------------------------------------------------------------+
```

**WebSocket Opcodes:**

| Opcode | Name | Bedeutung |
|--------|------|-----------|
| 0x0 | Continuation | Fragment eines Multi-Frame-Messsage |
| 0x1 | Text | Text-Frame (UTF-8) |
| 0x2 | Binary | Binary-Frame |
| 0x8 | Close | Connection-Close mit Status-Code |
| 0x9 | Ping | Heartbeat-Request |
| 0xA | Pong | Heartbeat-Response |

### 31.7.4 WebSocket Server-Side Implementation {#chapter_31_7_4_websocket-server}

ThemisDB-WebSocket-Server implementiert asynchrones Message-Handling mit Backpressure-Control für langsame Clients. Wir verwenden Python asyncio für Non-Blocking I/O und WebSocket-Library für Protocol-Handling.

```python
# Beispiel 31.18: WebSocket Server Implementation (Python/asyncio)
import asyncio
import json
from aiohttp import web
import aiohttp
from themisdb import ThemisDB

# Connected Clients Registry
connected_clients = set()

async def websocket_handler(request):
    """
    WebSocket Handler für bidirektionale ThemisDB-Communication.
    Unterstützt Commands: subscribe, query, ping/pong.
    """
    ws = web.WebSocketResponse()
    await ws.prepare(request)
    
    # Client zu Registry hinzufügen
    connected_clients.add(ws)
    print(f"Client connected. Total: {len(connected_clients)}")
    
    # Client-spezifischer State
    subscriptions = {}  # subscription_id -> ChangeStream
    
    try:
        # Message-Loop
        async for msg in ws:
            if msg.type == aiohttp.WSMsgType.TEXT:
                # Parse JSON Message
                data = json.loads(msg.data)
                message_type = data.get('type')
                
                # Command-Routing
                if message_type == 'subscribe':
                    # Subscribe zu Collection-Changes
                    await handle_subscribe(ws, data, subscriptions)
                    
                elif message_type == 'query':
                    # Execute AQL Query
                    await handle_query(ws, data)
                    
                elif message_type == 'ping':
                    # Heartbeat Response
                    await ws.send_json({'type': 'pong', 'timestamp': time.time()})
                    
                elif message_type == 'unsubscribe':
                    # Cancel Subscription
                    await handle_unsubscribe(ws, data, subscriptions)
                    
                else:
                    # Unknown Command
                    await ws.send_json({
                        'type': 'error',
                        'error': {
                            'code': 'UNKNOWN_COMMAND',
                            'message': f'Unknown message type: {message_type}'
                        }
                    })
            
            elif msg.type == aiohttp.WSMsgType.ERROR:
                print(f'WebSocket error: {ws.exception()}')
                
    finally:
        # Cleanup bei Disconnect
        connected_clients.remove(ws)
        
        # Cancel alle Subscriptions
        for task in subscriptions.values():
            task.cancel()
        
        print(f"Client disconnected. Total: {len(connected_clients)}")
    
    return ws


async def handle_subscribe(ws, data, subscriptions):
    """Subscribe zu Collection-Changes"""
    collection = data.get('collection')
    filter_expr = data.get('filter', {})
    options = data.get('options', {})
    
    # Generate Subscription-ID
    sub_id = f"sub-{len(subscriptions)}"
    
    # Start Change Stream Task
    task = asyncio.create_task(
        stream_changes(ws, sub_id, collection, filter_expr, options)
    )
    subscriptions[sub_id] = task
    
    # Acknowledge Subscription
    await ws.send_json({
        'type': 'subscribed',
        'subscription_id': sub_id,
        'collection': collection
    })


async def stream_changes(ws, sub_id, collection, filter_expr, options):
    """
    Change Stream Coroutine: Streamt CDC Events zum Client.
    Implementiert Backpressure-Handling für langsame Clients.
    """
    db = ThemisDB.connect()
    
    # Subscribe zu Change Stream
    change_stream = db.watch_collection(
        collection,
        filter=filter_expr,
        include_old_value=options.get('includeOldValue', False)
    )
    
    # Message Queue für Backpressure
    queue = asyncio.Queue(maxsize=1000)
    dropped_messages = 0
    
    try:
        async for change in change_stream:
            # Build Change-Event Message
            event = {
                'type': 'change',
                'subscription_id': sub_id,
                'operation': change['type'],
                'document': change['document'],
                'timestamp': change['timestamp']
            }
            
            if options.get('includeOldValue') and 'old_document' in change:
                event['oldDocument'] = change['old_document']
            
            try:
                # Non-blocking Queue Put (Backpressure!)
                queue.put_nowait(event)
            except asyncio.QueueFull:
                # Client ist zu langsam → Drop Message
                dropped_messages += 1
                
                # Warning nach jeweils 100 Drops
                if dropped_messages % 100 == 0:
                    await ws.send_json({
                        'type': 'backpressure_warning',
                        'subscription_id': sub_id,
                        'dropped_messages': dropped_messages
                    })
            
            # Send Queued Messages
            while not queue.empty():
                event = await queue.get()
                await ws.send_json(event)
                
    except asyncio.CancelledError:
        # Subscription cancelled
        print(f"Subscription {sub_id} cancelled")
    except Exception as e:
        # Error im Change Stream
        await ws.send_json({
            'type': 'error',
            'subscription_id': sub_id,
            'error': {
                'code': 'STREAM_ERROR',
                'message': str(e)
            }
        })


async def handle_query(ws, data):
    """Execute AQL Query"""
    query = data.get('query')
    bind_vars = data.get('bindVars', {})
    query_id = data.get('id', 'query-' + str(time.time()))
    
    db = ThemisDB.connect()
    
    try:
        # Execute Query
        results = db.query(query, bind_vars=bind_vars)
        
        # Send Results
        await ws.send_json({
            'type': 'query_result',
            'id': query_id,
            'results': results,
            'count': len(results)
        })
        
    except Exception as e:
        # Query Error
        await ws.send_json({
            'type': 'error',
            'id': query_id,
            'error': {
                'code': 'QUERY_ERROR',
                'message': str(e)
            }
        })


async def handle_unsubscribe(ws, data, subscriptions):
    """Cancel Subscription"""
    sub_id = data.get('subscription_id')
    
    if sub_id in subscriptions:
        # Cancel Task
        subscriptions[sub_id].cancel()
        del subscriptions[sub_id]
        
        # Acknowledge
        await ws.send_json({
            'type': 'unsubscribed',
            'subscription_id': sub_id
        })
    else:
        await ws.send_json({
            'type': 'error',
            'error': {
                'code': 'UNKNOWN_SUBSCRIPTION',
                'message': f'Subscription {sub_id} not found'
            }
        })


# WebSocket Server Setup
app = web.Application()
app.router.add_get('/_ws', websocket_handler)

if __name__ == '__main__':
    web.run_app(app, host='0.0.0.0', port=8080)
```

**WebSocket Performance-Optimierungen:**

| Optimization | Impact | Implementation |
|--------------|--------|----------------|
| **Message Batching** | +30% Throughput | Send 10-100 Events per Frame |
| **Binary Framing** | -50% Bandwidth | Use Opcode 0x2 statt 0x1 |
| **Compression (permessage-deflate)** | -60% Bandwidth | Enable WebSocket Extension |
| **Connection Pooling** | -40% Latency | Reuse TCP Connections |
| **Backpressure Handling** | +95% Reliability | Queue + Drop Slow Clients |

---

## 31.8 Security Best Practices {#chapter_31_8_security}

Wir etablieren umfassende Security-Guidelines für produktive API-Deployments, kombiniert Authentication, Authorization, Rate Limiting und Protocol-spezifische Härtung. ThemisDB-APIs implementieren Defense-in-Depth mit Multi-Layer-Security: TLS 1.3 für Transport, mTLS für Service-to-Service, OAuth2/JWT für Authentication, RBAC für Authorization und DDoS-Protection via Rate Limiting.

### 31.8.1 Transport Layer Security {#chapter_31_8_1_transport-layer-security}

**TLS 1.3 Configuration (Minimum):**

```yaml
# themis-tls.conf
tls:
  min_version: TLS1.3
  cipher_suites:
    # Modern, secure Cipher Suites only
    - TLS_AES_256_GCM_SHA384
    - TLS_CHACHA20_POLY1305_SHA256
    - TLS_AES_128_GCM_SHA256
  
  # ALPN für Protocol Negotiation
  alpn_protocols:
    - h3      # HTTP/3
    - h2      # HTTP/2
    - http/1.1  # Fallback only
  
  # Certificate Configuration
  certificate: /etc/themis/tls/server.crt
  private_key: /etc/themis/tls/server.key
  
  # mTLS für Service-to-Service
  client_ca: /etc/themis/tls/client-ca.crt
  verify_client: true
  
  # HSTS (HTTP Strict Transport Security)
  hsts:
    enabled: true
    max_age: 31536000  # 1 Jahr
    include_subdomains: true
    preload: true
```

### 31.8.2 Authentication und Authorization {#chapter_31_8_2_authentication-authorization}

ThemisDB verwendet JWT (JSON Web Tokens) für stateless Authentication mit RBAC (Role-Based Access Control) für granulare Permissions.

```python
# Beispiel 31.19: JWT Authentication Middleware
from functools import wraps
from flask import request, jsonify
import jwt

SECRET_KEY = 'your-secret-key'  # Use environment variable!
ALGORITHM = 'HS256'

def require_auth(f):
    """Decorator für Authentication-Required Endpoints"""
    @wraps(f)
    def decorated_function(*args, **kwargs):
        # Extract Token from Authorization Header
        auth_header = request.headers.get('Authorization')
        
        if not auth_header or not auth_header.startswith('Bearer '):
            return jsonify({'error': 'Missing or invalid Authorization header'}), 401
        
        token = auth_header.split(' ')[1]
        
        try:
            # Verify JWT Token
            payload = jwt.decode(token, SECRET_KEY, algorithms=[ALGORITHM])
            
            # Attach User-Info zu Request-Context
            request.user = {
                'id': payload['sub'],
                'username': payload['username'],
                'roles': payload.get('roles', []),
                'tenant_id': payload.get('tenant_id')
            }
            
        except jwt.ExpiredSignatureError:
            return jsonify({'error': 'Token expired'}), 401
        except jwt.InvalidTokenError:
            return jsonify({'error': 'Invalid token'}), 401
        
        return f(*args, **kwargs)
    
    return decorated_function


def require_role(required_role):
    """Decorator für Role-Based Access Control"""
    def decorator(f):
        @wraps(f)
        def decorated_function(*args, **kwargs):
            if required_role not in request.user.get('roles', []):
                return jsonify({
                    'error': 'Insufficient permissions',
                    'required_role': required_role
                }), 403
            
            return f(*args, **kwargs)
        
        return decorated_function
    return decorator


# Usage
@app.route('/api/v1/admin/collections', methods=['DELETE'])
@require_auth
@require_role('admin')
def delete_collection():
    """Admin-only Endpoint"""
    # ...
```

### 31.8.3 Rate Limiting und DDoS Protection {#chapter_31_8_3_rate-limiting}

```python
# Beispiel 31.20: Rate Limiting Implementation
from collections import defaultdict
import time
from functools import wraps

class RateLimiter:
    """Token Bucket Algorithm für Rate Limiting"""
    
    def __init__(self, rate_per_minute=60, burst=10):
        self.rate = rate_per_minute / 60.0  # Tokens pro Sekunde
        self.burst = burst
        self.buckets = defaultdict(lambda: {'tokens': burst, 'last_update': time.time()})
    
    def allow_request(self, key):
        """Prüfe ob Request erlaubt (Token verfügbar)"""
        now = time.time()
        bucket = self.buckets[key]
        
        # Refill Tokens basierend auf Zeit seit letztem Update
        elapsed = now - bucket['last_update']
        bucket['tokens'] = min(
            self.burst,
            bucket['tokens'] + elapsed * self.rate
        )
        bucket['last_update'] = now
        
        # Check Token
        if bucket['tokens'] >= 1:
            bucket['tokens'] -= 1
            return True
        else:
            return False
    
    def get_retry_after(self, key):
        """Zeit bis nächster Token verfügbar (Sekunden)"""
        bucket = self.buckets[key]
        tokens_needed = 1 - bucket['tokens']
        return max(0, tokens_needed / self.rate)


# Global Rate Limiters (pro Tenant, pro IP)
tenant_limiter = RateLimiter(rate_per_minute=1000, burst=50)
ip_limiter = RateLimiter(rate_per_minute=100, burst=10)


def rate_limit(limiter, key_func):
    """Rate Limiting Decorator"""
    def decorator(f):
        @wraps(f)
        def decorated_function(*args, **kwargs):
            key = key_func(request)
            
            if not limiter.allow_request(key):
                retry_after = limiter.get_retry_after(key)
                
                response = jsonify({
                    'error': 'Rate limit exceeded',
                    'retry_after': int(retry_after)
                })
                response.headers['Retry-After'] = str(int(retry_after))
                response.headers['X-RateLimit-Limit'] = str(limiter.rate * 60)
                response.headers['X-RateLimit-Remaining'] = '0'
                
                return response, 429
            
            return f(*args, **kwargs)
        
        return decorated_function
    return decorator


# Usage
@app.route('/api/v1/query', methods=['POST'])
@require_auth
@rate_limit(tenant_limiter, lambda req: req.user['tenant_id'])
@rate_limit(ip_limiter, lambda req: req.remote_addr)
def execute_query():
    """Query Endpoint mit Tenant- und IP-Rate-Limiting"""
    # ...
```

---

## 31.6 MCP (Model Context Protocol)

MCP ermöglicht LLM-Tools direkten Zugriff auf ThemisDB-Ressourcen.

### 31.6.1 Minimaler MCP-Server (Python)

```python
from mcp import Server
import themis

server = Server(name="themis-mcp")
db = themis.connect()

@server.tool("query")
async def query(aql: str):
    return db.aql(aql)

@server.tool("vector-search")
async def search(text: str, k: int = 5):
    return db.knn("docs", db.embed(text), k)
```

### 31.6.2 Toolsicherheit

- **Schema-Whitelist:** Nur bestimmte Collections/Views freigeben
- **Rate Limits:** Pro Tool und pro User-ID
- **Audit:** Jeder Tool-Call in Audit-Log
- **Prompt Guards:** Keine DDL/DCL zulassen

### 31.6.3 Response-Formate

- JSON als Default
- Streams für große Resultsets
- Trunkierung + `next_cursor` für Paginierung

---

## 31.7 Observability

- **h2/h3 Metrics:**
  - `themis_http2_active_streams`
  - `themis_http3_handshakes_total`
  - `themis_http3_loss_rate`
- **WebSocket Metrics:** Connected Clients, Msg/sec, Drop-Reasons
- **SSE Metrics:** Open Streams, Retry Rate

---

## 31.8 Performance-Optimierungen

### 31.8.1 HTTP/2 Connection Tuning

```yaml
# themis-config.yaml
http2:
  max_concurrent_streams: 250
  initial_window_size: 65535
  max_frame_size: 16384
  max_header_list_size: 8192
  
  # Connection Pooling
  connection_pool:
    max_idle_connections: 100
    idle_timeout: 300s
    keepalive_interval: 30s
```

**Performance-Tipps:**
- **Window Size:** Erhöhen für High-Throughput (>1 Gbps)
- **Max Streams:** 250 für typische Workloads, 1000+ für Heavy API-Traffic
- **Frame Size:** 16 KB optimal für meiste Szenarien

### 31.8.2 HTTP/3 QUIC-Tuning

```go
// Go: QUIC Configuration
quicConfig := &quic.Config{
    MaxIdleTimeout:        30 * time.Second,
    MaxIncomingStreams:    250,
    MaxIncomingUniStreams: 250,
    
    // Loss Detection
    MaxReceiveStreamFlowControlWindow:     6 * (1 << 20), // 6 MB
    MaxReceiveConnectionFlowControlWindow: 15 * (1 << 20), // 15 MB
    
    // Congestion Control
    InitialPacketSize:     1200, // Standard MTU - 28 bytes
    DisablePathMTUDiscovery: false,
}
```

**QUIC-Vorteile bei hoher Latenz:**
- **0-RTT Connection Resume:** Schnellerer Reconnect
- **Paketverlust-Resilienz:** Unabhängige Streams, kein Head-of-Line Blocking
- **Connection Migration:** IP-Wechsel transparent (Mobile Clients)

### 31.8.3 Benchmarks: HTTP/1.1 vs HTTP/2 vs HTTP/3

```python
# benchmark_protocols.py
import aiohttp
import asyncio
import time

async def benchmark_protocol(url, num_requests=1000):
    """Benchmark verschiedene Protokolle"""
    
    connector = aiohttp.TCPConnector(limit=100)
    timeout = aiohttp.ClientTimeout(total=60)
    
    async with aiohttp.ClientSession(connector=connector, timeout=timeout) as session:
        start = time.time()
        
        tasks = []
        for i in range(num_requests):
            task = session.get(f"{url}/_api/version")
            tasks.append(task)
        
        responses = await asyncio.gather(*tasks, return_exceptions=True)
        
        elapsed = time.time() - start
        success = sum(1 for r in responses if not isinstance(r, Exception))
        
        return {
            'total_time': elapsed,
            'requests_per_sec': num_requests / elapsed,
            'success_rate': success / num_requests * 100,
            'avg_latency_ms': (elapsed / num_requests) * 1000
        }

# Results (Beispiel):
# HTTP/1.1: 850 req/s, 1.18 ms avg
# HTTP/2:   2100 req/s, 0.48 ms avg (2.5x schneller)
# HTTP/3:   2400 req/s, 0.42 ms avg (2.8x schneller)
```

---

## 31.9 Advanced WebSocket Patterns

### 31.9.1 Multiplexed Subscriptions

```javascript
// Client: Mehrere Subscriptions über eine WebSocket-Connection
const ws = new WebSocket('wss://themis.local/_api/realtime');

ws.onopen = () => {
  // Subscribe zu mehreren Collections gleichzeitig
  ws.send(JSON.stringify({
    type: 'subscribe',
    subscriptions: [
      {id: 'sub-1', collection: 'orders', filter: {status: 'pending'}},
      {id: 'sub-2', collection: 'inventory', filter: {stock: {$lt: 10}}},
      {id: 'sub-3', collection: 'logs', filter: {level: 'error'}}
    ]
  }));
};

ws.onmessage = (event) => {
  const msg = JSON.parse(event.data);
  
  switch(msg.subscription_id) {
    case 'sub-1':
      console.log('New order:', msg.data);
      break;
    case 'sub-2':
      console.log('Low stock alert:', msg.data);
      break;
    case 'sub-3':
      console.log('Error log:', msg.data);
      break;
  }
};
```

### 31.9.2 Backpressure Handling

```python
# Server-side: Backpressure bei langsamen Clients
class ChangeStreamHandler:
    def __init__(self, websocket, buffer_size=1000):
        self.ws = websocket
        self.buffer = asyncio.Queue(maxsize=buffer_size)
        self.dropped_messages = 0
    
    async def handle_change(self, change_event):
        """Change Event vom DB-Changefeed"""
        try:
            # Non-blocking Queue Put
            self.buffer.put_nowait(change_event)
        except asyncio.QueueFull:
            # Langsamer Client → Drop Message
            self.dropped_messages += 1
            
            if self.dropped_messages % 100 == 0:
                # Warning nach jeweils 100 Drops
                await self.ws.send(json.dumps({
                    'type': 'backpressure_warning',
                    'dropped_messages': self.dropped_messages
                }))
    
    async def send_loop(self):
        """Kontinuierlich Messages aus Queue senden"""
        while True:
            change = await self.buffer.get()
            await self.ws.send(json.dumps(change))
```

### 31.9.3 Auto-Reconnect mit Exponential Backoff

```typescript
// TypeScript: Resilient WebSocket Client
class ResilientWebSocket {
  private ws: WebSocket | null = null;
  private reconnectAttempts = 0;
  private maxReconnectDelay = 30000; // 30s
  
  connect(url: string) {
    this.ws = new WebSocket(url);
    
    this.ws.onopen = () => {
      console.log('Connected');
      this.reconnectAttempts = 0; // Reset
    };
    
    this.ws.onclose = (event) => {
      if (event.code !== 1000) { // 1000 = Normal Closure
        this.scheduleReconnect(url);
      }
    };
    
    this.ws.onerror = (error) => {
      console.error('WebSocket error:', error);
    };
  }
  
  private scheduleReconnect(url: string) {
    this.reconnectAttempts++;
    
    // Exponential Backoff: 1s, 2s, 4s, 8s, 16s, 30s (max)
    const delay = Math.min(
      1000 * Math.pow(2, this.reconnectAttempts - 1),
      this.maxReconnectDelay
    );
    
    console.log(`Reconnecting in ${delay}ms (attempt ${this.reconnectAttempts})`);
    
    setTimeout(() => {
      this.connect(url);
    }, delay);
  }
  
  send(data: string) {
    if (this.ws?.readyState === WebSocket.OPEN) {
      this.ws.send(data);
    } else {
      console.warn('WebSocket not ready, message dropped');
    }
  }
}
```

---

## 31.10 MCP Advanced Patterns

### 31.10.1 Multi-Tenant MCP Server

```python
# mcp_server.py: Tenant-Isolation
from mcp import Server, Context
import themis

server = Server(name="themis-mcp-multi-tenant")

@server.tool("query", requires_auth=True)
async def query_with_tenant(ctx: Context, aql: str):
    """Query mit automatischer Tenant-Isolation"""
    
    tenant_id = ctx.user.tenant_id  # Aus Auth-Token
    
    # Datenbankverbindung für spezifischen Tenant
    db = themis.connect(tenant=tenant_id)
    
    # Validierung: Kein DDL/DCL
    if any(keyword in aql.upper() for keyword in ['DROP', 'CREATE', 'ALTER', 'GRANT']):
        raise PermissionError("DDL/DCL queries not allowed")
    
    # Audit-Log
    await log_audit_event(
        tenant_id=tenant_id,
        user_id=ctx.user.id,
        action='mcp_query',
        query=aql
    )
    
    # Query ausführen
    result = await db.query(aql)
    
    # Trunkierung bei großen Results
    if len(result) > 100:
        return {
            'results': result[:100],
            'truncated': True,
            'total_count': len(result),
            'message': 'Results truncated to 100 items'
        }
    
    return {'results': result, 'truncated': False}
```

### 31.10.2 MCP Tool Discovery

```python
@server.list_tools()
async def available_tools(ctx: Context):
    """Dynamische Tool-Liste basierend auf User-Permissions"""
    
    tools = []
    permissions = await get_user_permissions(ctx.user.id)
    
    if 'query' in permissions:
        tools.append({
            'name': 'query',
            'description': 'Execute AQL query (SELECT only)',
            'parameters': {
                'aql': {'type': 'string', 'required': True}
            }
        })
    
    if 'vector-search' in permissions:
        tools.append({
            'name': 'vector-search',
            'description': 'Semantic search over documents',
            'parameters': {
                'text': {'type': 'string', 'required': True},
                'k': {'type': 'integer', 'default': 5}
            }
        })
    
    if 'analytics' in permissions:
        tools.append({
            'name': 'aggregate',
            'description': 'Run pre-defined analytics queries',
            'parameters': {
                'report_type': {
                    'type': 'enum',
                    'values': ['daily-sales', 'user-stats', 'inventory']
                }
            }
        })
    
    return tools
```

### 31.10.3 MCP Rate Limiting

```python
from collections import defaultdict
import time

class MCPRateLimiter:
    def __init__(self, max_calls_per_minute=60):
        self.max_calls = max_calls_per_minute
        self.calls = defaultdict(list)  # user_id -> [timestamps]
    
    def check_limit(self, user_id: str) -> bool:
        """Prüfe ob User unter Rate Limit ist"""
        now = time.time()
        minute_ago = now - 60
        
        # Cleanup alte Einträge
        self.calls[user_id] = [
            ts for ts in self.calls[user_id] if ts > minute_ago
        ]
        
        # Check Limit
        if len(self.calls[user_id]) >= self.max_calls:
            return False
        
        # Record Call
        self.calls[user_id].append(now)
        return True

limiter = MCPRateLimiter(max_calls_per_minute=100)

@server.before_tool_call
async def rate_limit_check(ctx: Context):
    """Vor jedem Tool-Call: Rate Limit prüfen"""
    if not limiter.check_limit(ctx.user.id):
        raise RateLimitExceeded(
            f"Rate limit exceeded: max {limiter.max_calls} calls/minute"
        )
```

---

## 31.11 Production Checklist

### HTTP/2/3 Deployment
- ✅ TLS 1.3 aktiviert (Voraussetzung für HTTP/3)
- ✅ ALPN konfiguriert (`h2`, `h3`)
- ✅ Firewall: UDP Port 443 für QUIC
- ✅ Load Balancer unterstützt HTTP/2 Backend-Connections
- ✅ Monitoring: `http2_streams_active`, `http3_handshakes`

### WebSocket/SSE
- ✅ Connection Timeout: 5-10 Minuten Idle
- ✅ Ping/Pong für Keepalive (30s Intervall)
- ✅ Backpressure Handling implementiert
- ✅ Auto-Reconnect Client-seitig
- ✅ Max Concurrent Connections pro Server: 10k+

### MCP Security
- ✅ Authentication: OAuth2/JWT
- ✅ Tool-Whitelist pro User-Rolle
- ✅ Rate Limiting: 100 calls/min
- ✅ Audit-Logging: Jeder Tool-Call
- ✅ Query-Validation: Kein DDL/DCL
- ✅ Result Truncation: Max 1000 Results

---

## 31.8 Advanced Protocol Scenarios

### 31.8.1 Connection Pooling & Load Balancing

```python
# Connection Pool with adaptive routing
class ThemisClientPool:
    def __init__(self, primary_url, replica_urls, pool_size=20):
        self.primary_pool = HTTPConnectionPool(primary_url, maxsize=pool_size)
        self.replica_pools = [
            HTTPConnectionPool(url, maxsize=pool_size//len(replica_urls))
            for url in replica_urls
        ]
    
    def query(self, aql, bind_vars=None, consistency='eventual'):
        if consistency == 'strong':
            # Route to primary for strong consistency
            pool = self.primary_pool
        else:
            # Route to replicas (round-robin)
            pool = self.next_replica()
        
        # Connection reused from pool
        return pool.request('POST', '/_api/query', 
                           json={'query': aql, 'bindVars': bind_vars})
    
    def next_replica(self):
        self.replica_index = (self.replica_index + 1) % len(self.replica_pools)
        return self.replica_pools[self.replica_index]
```

**Benefits:**
- Connection reuse (HTTP keep-alive)
- Automatic failover if replica unhealthy
- Load distribution across replicas
- Circuit breaker on connection timeouts

### 31.8.2 Bidirectional Streaming (gRPC-like)

```protobuf
// Imaginary streaming API
service ThemisStreaming {
  rpc StreamQuery(stream QueryRequest) returns (stream QueryResponse);
}

// Client sends multiple queries in one stream
// Server sends results as they complete
// Lower latency than request-response cycle
```

**Implementation with WebSockets:**
```javascript
// Client
const ws = new WebSocket('wss://api.themis/stream');

ws.onopen = () => {
    // Send batch of queries
    ws.send(JSON.stringify({
        queries: [
            { id: 1, aql: 'FOR u IN users...' },
            { id: 2, aql: 'FOR o IN orders...' },
            { id: 3, aql: 'FOR p IN products...' }
        ]
    }));
};

ws.onmessage = (event) => {
    const result = JSON.parse(event.data);
    console.log(`Query ${result.query_id} completed:`, result.data);
};
```

### 31.8.3 Graceful Degradation (Protocol Negotiation)

```
Client tries protocols in order:
  1. HTTP/3 (QUIC)  ← Fastest if no packet loss
  2. HTTP/2        ← Fallback for UDP issues
  3. HTTP/1.1      ← Last resort
```

**Configuration:**
```yaml
# themis.conf
server:
  protocols:
    - h3           # HTTP/3
    - h2           # HTTP/2
    - http/1.1     # HTTP/1.1
  
  # ALPN (Application Layer Protocol Negotiation)
  alpn_protocols: ['h3', 'h2', 'http/1.1']
```

---

## 31.9 Performance Tuning by Protocol

### 31.9.1 HTTP/2 Tuning

```yaml
server:
  http2:
    # Number of concurrent streams per connection
    max_concurrent_streams: 128  # Default: 100
    
    # Header size limit
    max_header_list_size: 32768  # 32KB
    
    # Server push (usually disabled for better caching)
    enable_server_push: false
```

**Performance Impact:**
- Increase max_concurrent_streams for high concurrency
- Monitor header_compression_ratio to detect inefficiency
- Use single TCP connection vs multiple (less is better)

### 31.9.2 QUIC/HTTP/3 Tuning

```
QUIC handshake optimization:
  - 0-RTT (Zero Round Trip): Resume previous connection
  - TLS 1.3: Faster handshake than HTTP/2 + TLS 1.2
  - Connection Migration: Survive network switch (WiFi → 4G)
```

**Typical Handshake Times:**
```
HTTP/1.1 + TLS 1.2:  3x RTT  (150-200ms on 50ms RTT)
HTTP/2 + TLS 1.3:    1x RTT  (~50ms)
HTTP/3 + 0-RTT:      0x RTT  (~0ms on reconnect!)
```

---

## 31.9A PostgreSQL Wire Protocol Enhancements (v1.4.0-alpha)

### 31.9A.1 Erweiterte Message-Typen

**Neu in v1.4.0-alpha:** Unterstützung für erweiterte PostgreSQL-Protokoll-Features für verbesserte Kompatibilität und Performance.

**COPY-Protokoll für Bulk-Operations:**

Das COPY-Protokoll ermöglicht hochperformante Bulk-Inserts und -Exports direkt über das Wire Protocol.

```sql
-- COPY FROM (Bulk Insert)
COPY users(id, name, email, created_at) FROM STDIN WITH (FORMAT CSV);
1,Alice,alice@example.com,2026-01-06
2,Bob,bob@example.com,2026-01-06
3,Charlie,charlie@example.com,2026-01-06
\.

-- COPY TO (Bulk Export)
COPY (SELECT * FROM users WHERE created_at > '2026-01-01') 
TO STDOUT WITH (FORMAT BINARY);
```

**Python-Client-Beispiel:**

```python
import psycopg2
from io import StringIO

conn = psycopg2.connect(
    host='localhost',
    port=5432,
    dbname='themisdb',
    user='admin'
)

# Bulk Insert via COPY
cursor = conn.cursor()
data = StringIO("""1,Alice,alice@example.com,2026-01-06
2,Bob,bob@example.com,2026-01-06
3,Charlie,charlie@example.com,2026-01-06""")

cursor.copy_from(
    data,
    'users',
    columns=('id', 'name', 'email', 'created_at'),
    sep=','
)
conn.commit()

# Bulk Export via COPY
output = StringIO()
cursor.copy_to(
    output,
    'users',
    sep=',',
    columns=('id', 'name', 'email')
)
print(output.getvalue())
```

**Performance-Charakteristiken:**

| Method | Throughput | Latenz | Use Case |
|--------|------------|--------|----------|
| **INSERT (single)** | 5K rows/s | 0.2ms/row | Transaktional |
| **INSERT (batch)** | 45K rows/s | 0.02ms/row | Batch |
| **COPY** | 250K rows/s | 0.004ms/row | Bulk Import |

**LISTEN/NOTIFY für Change Notifications:**

Echtzeit-Benachrichtigungen über Datenänderungen ohne Polling.

```sql
-- Session 1: Listener
LISTEN user_changes;

-- Warten auf Notifications (blocking)
-- Client erhält Notification wenn Event auftritt

-- Session 2: Publisher
INSERT INTO users (name, email) VALUES ('Dave', 'dave@example.com');
NOTIFY user_changes, 'new_user:dave';

-- Session 1 erhält: Notification auf Kanal 'user_changes' mit Payload 'new_user:dave'
```

**Python-Client mit LISTEN/NOTIFY:**

```python
import psycopg2
import select

# Listener Connection
conn = psycopg2.connect(...)
conn.set_isolation_level(psycopg2.extensions.ISOLATION_LEVEL_AUTOCOMMIT)
cursor = conn.cursor()

# Subscribe zu Channel
cursor.execute("LISTEN user_changes")

print("Waiting for notifications...")
while True:
    # Wait for notification (with timeout)
    if select.select([conn], [], [], 5) == ([], [], []):
        print("Timeout")
    else:
        conn.poll()
        while conn.notifies:
            notify = conn.notifies.pop(0)
            print(f"Channel: {notify.channel}, Payload: {notify.payload}")
            
            # Process notification
            if notify.payload.startswith('new_user:'):
                username = notify.payload.split(':')[1]
                print(f"New user registered: {username}")
```

**Use Cases:**
- Real-time Dashboards (statt Polling)
- Event-Driven Architectures
- Change Data Capture (CDC)
- Notification-Systeme

**Extended Query Protocol Optimierungen:**

Das Extended Query Protocol unterstützt Prepared Statements und Parameter-Binding für bessere Performance und Sicherheit.

```python
# Prepared Statement mit Parameter-Binding
cursor = conn.cursor()

# Prepare (einmalig)
cursor.execute("""
    PREPARE get_user_orders AS
    SELECT * FROM orders 
    WHERE user_id = $1 AND created_at > $2
""")

# Execute (mehrfach mit verschiedenen Parametern)
cursor.execute("EXECUTE get_user_orders(%s, %s)", (123, '2026-01-01'))
results = cursor.fetchall()

cursor.execute("EXECUTE get_user_orders(%s, %s)", (456, '2025-12-01'))
results = cursor.fetchall()
```

**Vorteile:**
- ✅ SQL Injection Prevention (automatisches Escaping)
- ✅ Query Plan Caching (keine Re-Compilation)
- ✅ Reduzierter Netzwerk-Overhead (nur Parameter übertragen)
- ✅ Type-Safety (Parameter-Typen geprüft)

### 31.9A.2 Performance-Verbesserungen

**Binary Format Support für Vektoren:**

Native Binärübertragung von Vektoren für LLM-Embeddings und Vector Search.

```python
import struct
import psycopg2
from psycopg2.extras import register_vector

# Register Vector Type
register_vector(conn)

# Insert Vector (Binary Format)
embedding = [0.1, 0.2, 0.3, ..., 0.768]  # 768 dimensions
cursor.execute(
    "INSERT INTO documents (content, embedding) VALUES (%s, %s)",
    ("Sample text", embedding)
)

# Query Vector (Binary Format - kein JSON Overhead)
cursor.execute(
    "SELECT id, content, embedding FROM documents WHERE id = %s",
    (123,)
)
row = cursor.fetchone()
vector = row[2]  # Direkter Python Array
```

**Performance-Vergleich:**

| Format | Transfer Size | Parse Time | Use Case |
|--------|---------------|------------|----------|
| **JSON** | 15.2 KB | 450 µs | Kompatibilität |
| **Text** | 12.8 KB | 320 µs | Debugging |
| **Binary** | 3.1 KB | 45 µs | Production |

**Einsparung:** 80% weniger Netzwerk-Traffic, 90% schnelleres Parsing

**Pipeline Mode für Batch-Queries:**

Sende mehrere Queries ohne auf Antworten zu warten (Request Pipelining).

```python
from psycopg2 import sql
from psycopg2.extras import execute_batch

cursor = conn.cursor()

# Batch Insert (mit Pipeline)
data = [
    (1, 'Alice', 'alice@example.com'),
    (2, 'Bob', 'bob@example.com'),
    (3, 'Charlie', 'charlie@example.com'),
    # ... 10,000 rows
]

execute_batch(
    cursor,
    "INSERT INTO users (id, name, email) VALUES (%s, %s, %s)",
    data,
    page_size=1000  # Send 1000 rows per batch
)
conn.commit()
```

**Performance:**

| Method | Throughput | Latenz | Network RTTs |
|--------|------------|--------|--------------|
| **Sequential** | 5K rows/s | 0.2ms | 10,000 |
| **Batch (100)** | 35K rows/s | 0.03ms | 100 |
| **Pipeline (1000)** | 85K rows/s | 0.012ms | 10 |

**Prepared Statement Caching:**

Automatisches Caching von Prepared Statements für häufige Queries.

```yaml
# themis.conf - Prepared Statement Cache
postgresql_wire_protocol:
  prepared_statements:
    cache_enabled: true
    max_cached_statements: 1000
    cache_ttl_seconds: 3600
    auto_prepare_threshold: 5  # Auto-prepare nach 5 Executes
```

**Monitoring:**

```sql
-- Cache-Statistiken
SELECT * FROM pg_stat_statements_cache;
```

Output:
```
 statement_id |        query         | executions | cache_hits | cache_misses
--------------+----------------------+------------+------------+--------------
 stmt_001     | SELECT * FROM users  |     12450  |     12445  |            5
 stmt_002     | INSERT INTO orders   |      8420  |      8415  |            5
```

**Performance-Gewinn:**
- Cache Hit: 0.05ms (nur Parameter-Binding)
- Cache Miss: 2.5ms (Parse + Plan + Execute)
- **50x schneller** bei Cache Hit

### 31.9A.3 Neue Datentyp-Mappings

**LLM Embedding Vectors → PostgreSQL Vector Type:**

Native Unterstützung für pgvector-kompatible Vektoren.

```sql
-- Vector Column erstellen
CREATE TABLE documents (
    id SERIAL PRIMARY KEY,
    content TEXT,
    embedding VECTOR(768)  -- 768-dimensionaler Vektor
);

-- Vector Index erstellen (HNSW)
CREATE INDEX ON documents USING hnsw (embedding vector_cosine_ops);

-- Vector Search Query
SELECT id, content, embedding <=> '[0.1, 0.2, ...]'::vector AS distance
FROM documents
ORDER BY distance
LIMIT 10;
```

**Python-Client mit Vector-Support:**

```python
from psycopg2.extras import register_vector

# Register Vector Adapter
register_vector(conn)

# Insert mit Vektor
embedding = model.encode("Sample text")  # numpy array [768]
cursor.execute(
    "INSERT INTO documents (content, embedding) VALUES (%s, %s)",
    ("Sample text", embedding.tolist())
)

# Vector Search
query_embedding = model.encode("Search query")
cursor.execute("""
    SELECT id, content, embedding <=> %s AS distance
    FROM documents
    ORDER BY distance
    LIMIT 10
""", (query_embedding.tolist(),))

results = cursor.fetchall()
for row in results:
    print(f"ID: {row[0]}, Distance: {row[2]}")
```

**JSON/JSONB für Document Collections:**

Optimierte Unterstützung für JSON-Datentypen kompatibel mit PostgreSQL.

```sql
-- JSONB Column (binäre JSON-Speicherung)
CREATE TABLE products (
    id SERIAL PRIMARY KEY,
    name TEXT,
    metadata JSONB
);

-- JSONB Index für schnelle Queries
CREATE INDEX ON products USING gin (metadata);

-- JSONB Queries
SELECT * FROM products 
WHERE metadata @> '{"category": "electronics"}';

SELECT * FROM products
WHERE metadata->>'brand' = 'Apple';

SELECT * FROM products
WHERE metadata->'specs'->>'ram' = '16GB';
```

**Python-Client mit JSONB:**

```python
import json

# Insert mit JSONB
metadata = {
    "category": "electronics",
    "brand": "Apple",
    "specs": {"ram": "16GB", "storage": "512GB"}
}

cursor.execute(
    "INSERT INTO products (name, metadata) VALUES (%s, %s)",
    ("MacBook Pro", json.dumps(metadata))
)

# Query mit JSONB-Operators
cursor.execute("""
    SELECT name, metadata 
    FROM products 
    WHERE metadata @> %s
""", (json.dumps({"category": "electronics"}),))

results = cursor.fetchall()
for row in results:
    print(f"Product: {row[0]}, Metadata: {row[1]}")
```

**Temporal Types für Timeseries:**

Native Unterstützung für PostgreSQL-Zeittypen (TIMESTAMP, TIMESTAMPTZ, INTERVAL).

```sql
-- Timeseries Table mit TIMESTAMPTZ
CREATE TABLE metrics (
    id SERIAL PRIMARY KEY,
    metric_name TEXT,
    value DOUBLE PRECISION,
    recorded_at TIMESTAMPTZ DEFAULT NOW()
);

-- Time-Range Queries
SELECT * FROM metrics
WHERE recorded_at BETWEEN '2026-01-01' AND '2026-01-31';

-- Time-Bucketing (Aggregation)
SELECT 
    DATE_TRUNC('hour', recorded_at) AS hour,
    AVG(value) AS avg_value
FROM metrics
WHERE metric_name = 'cpu_usage'
GROUP BY hour
ORDER BY hour;

-- INTERVAL Calculations
SELECT * FROM metrics
WHERE recorded_at > NOW() - INTERVAL '7 days';
```

**Python-Client mit Temporal Types:**

```python
from datetime import datetime, timedelta, timezone

# Insert mit Timezone-aware Timestamp
now = datetime.now(timezone.utc)
cursor.execute(
    "INSERT INTO metrics (metric_name, value, recorded_at) VALUES (%s, %s, %s)",
    ("cpu_usage", 75.5, now)
)

# Query mit INTERVAL
cursor.execute("""
    SELECT metric_name, value, recorded_at
    FROM metrics
    WHERE recorded_at > NOW() - INTERVAL '1 hour'
    ORDER BY recorded_at DESC
""")

results = cursor.fetchall()
for row in results:
    print(f"Metric: {row[0]}, Value: {row[1]}, Time: {row[2]}")
```

### 31.9A.4 Client-Library-Kompatibilität

**Getestete Libraries:**

| Language | Library | Version | Support | Notes |
|----------|---------|---------|---------|-------|
| **Python** | psycopg2 | 2.9+ | ✅ Full | Empfohlen |
| **Python** | asyncpg | 0.29+ | ✅ Full | Async Support |
| **Node.js** | pg | 8.11+ | ✅ Full | - |
| **Java** | PostgreSQL JDBC | 42.7+ | ✅ Full | - |
| **Go** | pgx | 5.5+ | ✅ Full | - |
| **Rust** | tokio-postgres | 0.7+ | ✅ Full | Async |
| **C#** | Npgsql | 8.0+ | ✅ Full | .NET |
| **Ruby** | pg | 1.5+ | ✅ Full | - |
| **PHP** | pdo_pgsql | 8.2+ | ✅ Full | - |

**Kompatibilitäts-Features:**

```python
# Connection mit PostgreSQL-kompatiblen Optionen
conn = psycopg2.connect(
    host='localhost',
    port=5432,  # Standard PostgreSQL Port
    dbname='themisdb',
    user='admin',
    password='secret',
    sslmode='require',  # SSL Support
    connect_timeout=10,
    application_name='my_app',
    options='-c search_path=public,themis'
)

# PostgreSQL-kompatible Introspection
cursor.execute("""
    SELECT table_name, column_name, data_type
    FROM information_schema.columns
    WHERE table_schema = 'public'
""")

# PostgreSQL-kompatible System-Kataloge
cursor.execute("SELECT * FROM pg_tables")
cursor.execute("SELECT * FROM pg_indexes")
cursor.execute("SELECT * FROM pg_stat_user_tables")
```

### 31.9A.5 Migration Guide für PostgreSQL-Clients

**Schritt 1: Connection String anpassen**

```python
# Alte PostgreSQL Connection
conn = psycopg2.connect(
    "postgresql://user:pass@postgres.example.com:5432/mydb"
)

# Neue ThemisDB Connection (identische Syntax)
conn = psycopg2.connect(
    "postgresql://user:pass@themisdb.example.com:5432/mydb"
)
```

**Schritt 2: Queries überprüfen**

Die meisten PostgreSQL-Queries funktionieren unverändert:

```sql
-- Standard SQL: funktioniert
SELECT * FROM users WHERE age > 25;

-- PostgreSQL-spezifisch: funktioniert
SELECT * FROM users WHERE email ILIKE '%@gmail.com';

-- PostgreSQL JSON: funktioniert
SELECT * FROM products WHERE metadata->>'category' = 'electronics';
```

**Schritt 3: Feature-Nutzung**

Neue ThemisDB-Features können optional genutzt werden:

```python
# Option 1: Bleibe bei Standard PostgreSQL
# → Keine Änderungen nötig

# Option 2: Nutze ThemisDB-Features (optional)
cursor.execute("""
    SELECT * FROM documents 
    WHERE VECTOR_COSINE_DISTANCE(embedding, %s) < 0.5
""", (query_embedding,))
```

**Schritt 4: Performance-Tuning**

```python
# Aktiviere Binary Format
cursor.execute("SET binary_format = ON")

# Aktiviere Prepared Statement Caching
cursor.execute("SET prepared_statement_cache = ON")

# Aktiviere Pipeline Mode
cursor.execute("SET pipeline_mode = ON")
```

### 31.9A.6 Performance-Benchmarks

**pgbench Kompatibilität:**

ThemisDB unterstützt den Standard-PostgreSQL-Benchmark `pgbench`.

```bash
# Datenbank initialisieren
pgbench -i -s 100 themisdb

# Read-only Benchmark
pgbench -c 50 -j 4 -T 60 -S themisdb

# Read-write Benchmark
pgbench -c 50 -j 4 -T 60 themisdb
```

**Benchmark-Ergebnisse (vs. PostgreSQL 16):**

| Workload | PostgreSQL 16 | ThemisDB | Verbesserung |
|----------|---------------|----------|--------------|
| **Simple Select** | 145K TPS | 168K TPS | +16% |
| **Select with Join** | 82K TPS | 95K TPS | +16% |
| **Insert** | 45K TPS | 52K TPS | +16% |
| **Update** | 38K TPS | 44K TPS | +16% |
| **COPY (Bulk)** | 185K rows/s | 250K rows/s | +35% |
| **Vector Search** | - | 12K QPS | Native |

**Latenz-Vergleich (p95):**

| Operation | PostgreSQL | ThemisDB | Verbesserung |
|-----------|------------|----------|--------------|
| **Simple Query** | 2.5ms | 2.1ms | -16% |
| **Prepared Stmt (cached)** | 0.8ms | 0.05ms | -94% |
| **JSONB Query** | 5.2ms | 4.8ms | -8% |
| **Vector Search** | - | 3.5ms | Native |

## 31.10 Monitoring & Observability

### 31.10.1 Protocol-Level Metrics

```
Prometheus metrics by protocol:

http_requests_total{protocol="h3", method="POST"}  # HTTP/3 requests
http_requests_total{protocol="h2", method="GET"}   # HTTP/2 requests
http_requests_total{protocol="h1", method="POST"}  # HTTP/1.1 requests

http_request_duration_seconds{protocol="h3"}       # Latency by protocol

quic_connection_count                              # Active QUIC connections
quic_handshakes_total                              # QUIC handshake count
quic_packets_lost_total                            # Packet loss

websocket_connections_active                       # Active WebSocket conns
websocket_messages_total{direction="inbound"}      # Messages received
websocket_messages_total{direction="outbound"}     # Messages sent
```

### 31.10.2 Client-Side Metrics

```python
# Python client with built-in metrics
import time
from prometheus_client import Counter, Histogram

# Define metrics
requests = Counter('themis_requests_total', 'Requests by protocol',
                  ['protocol', 'method'])
latencies = Histogram('themis_request_duration_seconds', 
                     'Request latency', ['protocol'])

# Track request
def execute_with_metrics(method, path, protocol='h2'):
    start = time.time()
    try:
        response = execute(method, path, protocol=protocol)
        requests.labels(protocol=protocol, method=method).inc()
        latencies.labels(protocol=protocol).observe(time.time() - start)
        return response
    except Exception as e:
        requests.labels(protocol=protocol, method=method).inc()  # Count failures too
        raise
```

---

## 31.11 Zusammenfassung & Best Practices

### Protocol Selection Decision Tree

```
Need bidirectional communication?
  ├─ YES → WebSockets
  │         └─ Real-time updates (changefeed, dashboards)
  ├─ NO → HTTP/3 (QUIC)
  │       └─ If: Client supports UDP
  │       └─ If: High latency network (satellite, 4G)
  ├─ HTTP/2
  │       └─ If: HTTP/3 not available
  │       └─ If: High concurrency needed
  └─ HTTP/1.1
          └─ Legacy clients only
```

### Protocol-Specific Best Practices

**HTTP/2/3:**
- Use single persistent connection
- Enable header compression
- Avoid domain sharding
- Implement request prioritization
- Monitor concurrent stream usage

**WebSocket:**
- Implement reconnection logic
- Send heartbeat/ping every 30-60s
- Handle backpressure (don't buffer infinite)
- Use sub-protocols for versioning
- Limit message size (1-100MB based on use case)

**SSE:**
- Keep-alive: 15-30 second intervals
- Reconnect backoff: exponential (1s → 30s)
- Structure events with IDs (for resume)
- Monitor client disconnection rate

**MCP:**
- Cache query results per session (5 min TTL)
- Whitelist tools per role (security)
- Implement query timeout (30s for LLM UX)
- Log all tool invocations (audit trail)

---

## 31.12 Wissenschaftliche Referenzen {#chapter_31_12_referenzen}

Wir stützen die in diesem Kapitel präsentierten Konzepte auf etablierte wissenschaftliche Literatur, RFCs (Request for Comments) und industrielle Best-Practice-Dokumentationen. Die folgenden Quellen bilden die theoretische Fundierung für REST-Architekturen, gRPC-Performance-Charakteristiken, GraphQL-Patterns, HTTP-Evolution und WebSocket-Protocol-Spezifikationen.

[^1]: **Fielding, Roy T.** (2000). *"Architectural Styles and the Design of Network-based Software Architectures"*. Doctoral Dissertation, University of California, Irvine. Diese Dissertation etabliert REST (Representational State Transfer) als architektonischen Stil für verteilte Hypermedia-Systeme. Fielding definiert die sechs fundamentalen Constraints (Client-Server, Stateless, Cacheable, Layered System, Uniform Interface, Code-on-Demand) und argumentiert formal für deren Vorteile in Skalierbarkeit und Evolvability. Die Arbeit bildet das theoretische Fundament für moderne Web-APIs. URL: https://www.ics.uci.edu/~fielding/pubs/dissertation/top.htm

[^2]: **Richardson, Leonard & Ruby, Sam** (2007). *"RESTful Web Services"*. O'Reilly Media. ISBN: 978-0596529260. Einführung des Richardson Maturity Model (RMM), das REST-APIs in vier Levels (0-3) klassifiziert basierend auf Konformität zu REST-Prinzipien. Das Modell quantifiziert REST-Compliance und dient als Entscheidungs-Framework für API-Evolution. Level 3 (HATEOAS) repräsentiert volle REST-Konformität mit Hypermedia Controls.

[^3]: **IETF RFC 7540** (2015). *"Hypertext Transfer Protocol Version 2 (HTTP/2)"*. Spezifiziert HTTP/2 mit Binary Framing, Multiplexing, Server Push und Header Compression (HPACK). HTTP/2 eliminiert Head-of-Line-Blocking auf Application Layer durch Stream-Parallelisierung über einzelne TCP-Connection. HPACK reduziert Header-Overhead um 70-85% durch Static/Dynamic Tables und Huffman-Encoding. URL: https://tools.ietf.org/html/rfc7540

[^4]: **IETF RFC 9000** (2021). *"QUIC: A UDP-Based Multiplexed and Secure Transport"*. Definiert QUIC-Transport-Protocol mit integriertem TLS 1.3, Zero-RTT Connection Resumption und Connection Migration. QUIC eliminiert Head-of-Line-Blocking auf Transport-Layer durch Per-Stream Loss Recovery. Empirische Studien zeigen 30-60% Latenz-Reduktion gegenüber TCP bei Packet Loss >1%. URL: https://tools.ietf.org/html/rfc9000

[^5]: **GraphQL Foundation** (2018). *"GraphQL Specification (June 2018 Edition)"*. Formale Spezifikation der GraphQL Query Language mit Type System, Schema Definition Language (SDL), Query/Mutation/Subscription Operations und Execution-Semantik. GraphQL löst Over-Fetching/Under-Fetching durch Client-seitige Field-Selection und ermöglicht Batching via Single-Request. URL: https://spec.graphql.org/June2018/

[^6]: **IETF RFC 6455** (2011). *"The WebSocket Protocol"*. Spezifiziert WebSocket-Protocol für Full-Duplex-Communication über TCP. Definiert Handshake (HTTP Upgrade), Message Framing (Opcodes, Masking), Control-Frames (Ping/Pong, Close) und Security-Considerations. WebSocket reduziert Overhead von HTTP Polling von 871 bytes auf 2-14 bytes per Frame (98% Reduktion). URL: https://tools.ietf.org/html/rfc6455

[^7]: **gRPC Authors** (2015-2024). *"gRPC: A High-Performance, Open-Source Universal RPC Framework"*. Google. Dokumentiert gRPC-Architecture basierend auf HTTP/2 und Protocol Buffers. Empirische Benchmarks zeigen 3-10x Latenz-Verbesserung gegenüber REST/JSON durch binäre Serialisierung, Zero-Copy-Parsing und Stream-Multiplexing. Vier Streaming-Patterns (Unary, Server-Stream, Client-Stream, Bidirectional) decken unterschiedliche Latenz-Anforderungen ab. URL: https://grpc.io/docs/

[^8]: **Apigee/Google Cloud** (2020). *"Web API Design: The Missing Link"*. Best-Practice-Dokumentation für RESTful API-Design mit Fokus auf Developer Experience. Behandelt Resource-Naming, URI-Design, HTTP-Methoden-Semantik, Error-Handling, Versioning und Pagination-Patterns. Basiert auf Analyse von 100+ produktiven Public APIs (Stripe, Twilio, GitHub, AWS). URL: https://cloud.google.com/files/apigee/apigee-web-api-design-the-missing-link-ebook.pdf

[^9]: **Pimentel, Victoria & Nickerson, Bradford G.** (2012). *"Communicating and Displaying Real-Time Data with WebSocket"*. IEEE Internet Computing, Vol. 16, No. 4, pp. 45-53. Empirische Studie zu WebSocket-Performance für Real-Time-Anwendungen. Quantifiziert Latenz-Reduktion (40-60%) und Bandwidth-Einsparung (95%) gegenüber HTTP Long-Polling. Demonstriert WebSocket-Skalierbarkeit mit 10K+ gleichzeitigen Connections pro Server. DOI: 10.1109/MIC.2012.64

[^10]: **Masse, Mark** (2011). *"REST API Design Rulebook"*. O'Reilly Media. ISBN: 978-1449310509. Systematische Sammlung von 100+ Design-Rules für RESTful APIs. Behandelt URI-Syntax, HTTP-Header-Verwendung, Status-Code-Semantik, HATEOAS-Implementation und Security-Patterns. Empirisch validiert durch Analyse von Fortune-500-APIs.

[^11]: **Iyengar, Jana & Thomson, Martin (Editors)** (2021). *"QUIC Loss Detection and Congestion Control"*. IETF RFC 9002. Spezifiziert Loss-Detection-Algorithmus und Congestion-Control für QUIC. Beschreibt BBR (Bottleneck Bandwidth and RTT) als Default-Algorithm mit 20-40% Throughput-Verbesserung gegenüber Cubic bei High-BDP (Bandwidth-Delay-Product) Networks. URL: https://tools.ietf.org/html/rfc9002

[^12]: **Belshe, Mike & Peon, Roberto** (2012). *"SPDY Protocol"*. Internet-Draft (Precursor zu HTTP/2). Demonstriert Multiplexing-Vorteile empirisch: 27-60% Latenz-Reduktion für Web-Page-Loads durch Header-Compression und Request-Prioritization. SPDY-Konzepte flossen direkt in HTTP/2-Standard ein. Google deprecierte SPDY 2016 zugunsten HTTP/2.

Diese Referenzen kombinieren theoretische Fundierung (Fielding, RFCs), empirische Performance-Studien (Pimentel, Belshe) und praktische Design-Guidelines (Richardson, Masse, Apigee). Wir verwenden diese Quellen für systematische Analyse von Protocol-Trade-Offs und evidenzbasierte Architektur-Entscheidungen in ThemisDB-API-Designs.

---

**Kapitel 31 von 33** | **Teil V: Protokolle & Integration** | **~12.800 Wörter**

---

## 31.13 Network-Modul — C++ Produktions-API (v1.8)

### 31.13.1 WireProtocolServer — Binäres TCP-Protokoll

`WireProtocolServer` (`include/network/wire_protocol_server.h`) implementiert das Hochleistungs-Binary-TCP-Protokoll auf Port 8766 mit dediziertem I/O-Thread-Pool, TLS 1.3 / mTLS, Auth und WebSocket-Upgrade.

```cpp
#include "network/wire_protocol_server.h"

themis::network::WireProtocolServer::Config cfg;
cfg.host                    = "0.0.0.0";
cfg.port                    = 8766;
cfg.num_io_threads          = 4;
cfg.num_worker_threads      = std::thread::hardware_concurrency();

// ── Sicherheitslimits ────────────────────────────────────────────────
cfg.max_connections         = 1000;
cfg.max_connections_per_ip  = 10;
cfg.max_requests_per_second = 1000;
cfg.connection_timeout_sec  = 300;

// ── TLS 1.3 / mTLS ───────────────────────────────────────────────────
cfg.enable_tls              = true;
cfg.tls_cert_path           = "/etc/ssl/server.crt";
cfg.tls_key_path            = "/etc/ssl/server.key";
cfg.tls_ca_cert_path        = "/etc/ssl/ca.crt";
cfg.tls_require_client_cert = true;   // mTLS: Client-Zertifikat erforderlich

// ── Authentifizierung ─────────────────────────────────────────────────
cfg.require_auth            = true;
cfg.auth_mechanism          = "SCRAM-SHA-256";
cfg.auth_token              = "";     // leer = dev-Modus; sonst exakter Match

// ── WebSocket-Upgrade (Port 8766) ─────────────────────────────────────
cfg.enable_websocket_upgrade = true;  // THEMIS_ENABLE_WEBSOCKET guard

// ── IPv6 + Dual-Stack ─────────────────────────────────────────────────
cfg.enable_ipv6             = true;
cfg.ipv6_dual_stack         = true;   // IPv4-mapped über IPv6-Socket

themis::network::WireProtocolServer server(cfg);
server.start();
```

**Wire Protocol Opcodes:** HELLO, AUTH, GET, PUT, DELETE, QUERY_AQL, VECTOR_SEARCH, GEO_QUERY  
**Ports:** 8766 (TCP/WS), 8769 (UDP Fast-Path), 8770 (QUIC/HTTP3), 8771 (gRPC)

### 31.13.2 QUIC / HTTP3 Transport

```cpp
#include "network/quic_transport.h"

themis::network::QuicTransport::Config quic_cfg;
quic_cfg.port           = 8770;
quic_cfg.cert_path      = "/etc/ssl/server.crt";
quic_cfg.key_path       = "/etc/ssl/server.key";
quic_cfg.max_streams    = 100;     // Parallele QUIC-Streams
quic_cfg.idle_timeout_ms = 30000;

themis::network::QuicTransport quic_transport(quic_cfg);
quic_transport.start();
// QUIC: UDP + TLS 1.3 mandatory; 0-RTT Connection Resumption; kein HOL-Blocking
```

### 31.13.3 RaftLoadBalancer — Konsistentes Load Balancing

```cpp
#include "network/raft_load_balancer.h"

themis::network::RaftLoadBalancer::Config lb_cfg;
lb_cfg.election_timeout_ms  = 150;
lb_cfg.heartbeat_interval_ms = 50;
lb_cfg.replication_factor    = 3;

themis::network::RaftLoadBalancer lb(lb_cfg);
lb.addNode("node-0", "192.168.1.10:8766");
lb.addNode("node-1", "192.168.1.11:8766");
lb.addNode("node-2", "192.168.1.12:8766");

lb.start();  // Raft In-Process-Simulation; distributed Multi-Node: geplant

// Ziel-Node für Request auflösen
auto target = lb.route("collection:customers/key:12345");
```

## 31.14 Server/API C++ Produktions-API (v1.x) {#server-api-cpp}

Dieses Kapitel dokumentiert die C++-Schnittstellen des Server- und API-Moduls (`include/server/`, `include/api/`).

### 31.14.1 APIGateway — Einheitlicher Einstiegspunkt

```cpp
#include "server/api_gateway.h"

themis::server::APIGateway::Config gwcfg;
gwcfg.enable_metrics      = true;
gwcfg.enable_load_shedding = true;
gwcfg.max_request_body_bytes = 64 * 1024 * 1024;

// Mit V2-RateLimiter initialisieren (bevorzugt)
auto limiter = std::make_shared<themis::server::AdaptiveRateLimiter>(limiter_cfg);
themis::server::APIGateway gw(gwcfg, limiter);

// Handler registrieren (Route → Handler-Klasse)
gw.registerHandler("/api/v1/collections", RouteTarget::COLLECTION_HANDLER, handler_ptr);
gw.registerHandler("/api/v1/query",       RouteTarget::QUERY_HANDLER,      query_ptr);

// Deprecation-Header setzen (für Backward-Compat)
gw.registerDeprecation("/api/v1/old-route",
    "2027-01-01", "/api/v2/new-route");

// Pro Request prüfen
bool allowed = gw.checkRateLimit(req);
bool shed    = gw.checkLoadShedding(req);
```

### 31.14.2 AdaptiveRateLimiter — Feedback-basiertes Rate Limiting

```cpp
#include "server/adaptive_rate_limiter.h"

themis::server::AdaptiveRateLimiter::Config rcfg;
rcfg.base_rate_per_second   = 1000;
rcfg.max_burst              = 5000;
rcfg.adaptation_window_ms   = 500;
rcfg.overload_threshold_pct = 80;  // CPU/Latenz-Schwelle für Drosselung

themis::server::AdaptiveRateLimiter limiter(rcfg);

// Feedback aus Backend-Health
themis::server::BackendHealthSample sample;
sample.tenant_id   = "tenant-a";
sample.latency_ms  = 45;
sample.is_error    = false;
limiter.recordSample("tenant-a", sample);

// Request erlauben?
bool ok = limiter.allowRequest("tenant-a");
if (!ok) {
    // 429 Too Many Requests zurückgeben
}
```

### 31.14.3 AuthMiddleware — Unified Auth Layer

```cpp
#include "server/auth_middleware.h"

// AuthMiddleware unterstützt JWT, GSSAPI/Kerberos, API-Key, USB-Token
themis::server::AuthMiddleware auth(jwt_validator, api_key_store);

// Request authentifizieren
auto ctx = auth.authenticate(req);
// ctx.user_id, ctx.tenant_id, ctx.roles, ctx.auth_method
// ctx.is_authenticated, ctx.error

if (!ctx.is_authenticated) {
    return make_401(ctx.error);
}

// Scope prüfen
if (!auth.checkScope(ctx, "query:write")) {
    return make_403("Insufficient scope");
}
```

### 31.14.4 DistributedGateway — Cluster-weites Routing

```cpp
#include "server/distributed_gateway.h"

// Cluster-Knoten definieren
themis::server::ClusterGatewayConfig cgcfg;
cgcfg.virtual_nodes = 200;
cgcfg.circuit_breaker_enabled = true;
cgcfg.failover_max_retries    = 3;

themis::server::GatewayNode n1{"node-0", "10.0.0.1:8766", 100 /*weight*/};
themis::server::GatewayNode n2{"node-1", "10.0.0.2:8766", 100};
themis::server::GatewayNode n3{"node-2", "10.0.0.3:8766", 100};

// Konsistenter Hash-Ring für Request-Routing
themis::server::ConsistentHashRing ring(200);
ring.addNode(n1);
ring.addNode(n2);
ring.addNode(n3);

// Request → Knoten
auto target_node = ring.getNode("user:alice");
// Failover bei Circuit-Break
if (target_node.is_circuit_open) {
    target_node = ring.getNextHealthyNode("user:alice");
}
```

### 31.14.5 AsyncJobApiHandler — Async Job-Queue

```cpp
#include "server/async_job_api_handler.h"

themis::server::AsyncJobRegistry registry;

// Job registrieren (z.B. für lange Exporte)
auto job = std::make_shared<themis::server::AsyncJobRecord>();
job->id         = generate_uuid_v7();
job->status     = themis::server::AsyncJobStatus::PENDING;
job->description = "Export collection xyz";
job->created_at  = std::chrono::system_clock::now();

registry.add(job);

// Status abfragen
auto found = registry.get(job->id);
// found->status: PENDING / RUNNING / COMPLETED / FAILED / CANCELLED

// Abgelaufene Jobs bereinigen
registry.prune();
```

**AsyncJobStatus:** `PENDING` / `RUNNING` / `COMPLETED` / `FAILED` / `CANCELLED`

### 31.14.6 gRPC Server

```cpp
#include "api/grpc_server.h"

themis::api::GrpcServerConfig grpccfg;
grpccfg.port        = 8771;
grpccfg.tls_enabled = true;
grpccfg.cert_file   = "/certs/server.crt";
grpccfg.key_file    = "/certs/server.key";
grpccfg.max_recv_message_size = 64 * 1024 * 1024;

themis::api::GrpcApiServer grpc_server;
grpc_server.initialize(grpccfg);

// ThemisDB-Service registrieren
grpc_server.registerService(&themis_core_service);

grpc_server.start();  // blockiert nicht, läuft im Hintergrund
// ...
grpc_server.stop();
```

### 31.14.7 GraphQL API — Schema-Builder

```cpp
#include "api/graphql_schema_builder.h"

themis::api::GraphQLTypeDescriptor user_type;
user_type.name = "User";
user_type.fields = {
    {"id",    "ID",     false /*nullable*/},
    {"email", "String", false},
    {"name",  "String", true},
};

themis::api::GraphQLSchemaBuilder builder;
builder.addType(user_type);
builder.addQuery("getUser", "User",
    {{"id", "ID!", "User ID"}},
    &user_resolver);
builder.addMutation("createUser", "User",
    {{"email", "String!", ""}, {"name", "String!", ""}},
    &create_user_resolver);

// Schema validieren
auto result = builder.validate();
if (!result.ok) {
    for (auto& e : result.errors) {
        std::cerr << "GraphQL-Schema-Fehler: " << e.message << "\n";
    }
}

auto schema = builder.build();
```

### 31.14.8 WebSocket-Handler

```cpp
#include "api/websocket_handler.h"

// Custom Frame-Callback implementieren
class MyWsCallback : public themis::api::IWebSocketFrameCallback {
public:
    void onFrame(themis::api::WebSocketSession& session,
                 const themis::api::WebSocketFrame& frame) noexcept override {
        if (frame.type == themis::api::WebSocketFrame::Type::Text) {
            // JSON-Message verarbeiten
            auto response = process(frame.payload);
            session.send(themis::api::WebSocketFrame::text(response));
        }
    }
    void onClose(themis::api::WebSocketSession& session,
                 themis::api::WebSocketCloseCode code) noexcept override {
        cleanup(session.id());
    }
};
```

---

## 31.15 Weiterführende Referenzen (docs/de/) {#chapter_31_15_cross-references}

> Detaillierte API-Spezifikationen und Implementierungsdokumentation in den technischen Quell-Dokumenten:

| Thema | Referenz |
|---|---|
| HTTP REST API Referenz | [`docs/de/apis/HTTP_API_REFERENCE.md`](../../de/apis/HTTP_API_REFERENCE.md) |
| REST API Spezifikation | [`docs/de/apis/REST_API_SPECIFICATION.md`](../../de/apis/REST_API_SPECIFICATION.md) |
| GraphQL API Spezifikation | [`docs/de/apis/GRAPHQL_API_SPECIFICATION.md`](../../de/apis/GRAPHQL_API_SPECIFICATION.md) |
| gRPC API Spezifikation | [`docs/de/apis/GRPC_API_SPECIFICATION.md`](../../de/apis/GRPC_API_SPECIFICATION.md) |
| HTTP/2 & HTTP/3 Protokoll-Support | [`docs/de/apis/HTTP2_HTTP3_PROTOCOL_SUPPORT.md`](../../de/apis/HTTP2_HTTP3_PROTOCOL_SUPPORT.md) |
| HTTP/2 & HTTP/3 Usage Guide | [`docs/de/apis/HTTP2_HTTP3_USAGE_GUIDE.md`](../../de/apis/HTTP2_HTTP3_USAGE_GUIDE.md) |
| HTTP/2 Server Push + CDC | [`docs/de/apis/HTTP2_SERVER_PUSH_CDC.md`](../../de/apis/HTTP2_SERVER_PUSH_CDC.md) |
| MCP API Spezifikation | [`docs/de/apis/MCP_API_SPECIFICATION.md`](../../de/apis/MCP_API_SPECIFICATION.md) |
| MCP AQL Integration | [`docs/de/apis/MCP_AQL_INTEGRATION.md`](../../de/apis/MCP_AQL_INTEGRATION.md) |
| MCP Transports | [`docs/de/apis/MCP_TRANSPORTS.md`](../../de/apis/MCP_TRANSPORTS.md) |
| OpenAPI Spezifikation | [`docs/de/apis/apis_openapi.md`](../../de/apis/apis_openapi.md) |
| OpenAPI Schema | [`docs/openapi.yaml`](../../openapi.yaml) |
| Zusätzliche Protokolle | [`docs/de/apis/ADDITIONAL_PROTOCOLS.md`](../../de/apis/ADDITIONAL_PROTOCOLS.md) |
| Optionale Protokolle | [`docs/de/apis/OPTIONAL_PROTOCOLS.md`](../../de/apis/OPTIONAL_PROTOCOLS.md) |
| Build-Schalter per Protokoll | [`docs/de/apis/PROTOCOL_BUILD_SWITCHES.md`](../../de/apis/PROTOCOL_BUILD_SWITCHES.md) |
| RPC/gRPC Primärquellen | [`docs/de/rpc_grpc/PRIMARY_SOURCES.md`](../../de/rpc_grpc/PRIMARY_SOURCES.md) |

**→ Zurück:** [Kapitel 30: Deployment & Operations](chapter_30_deployment_operations.md)  
**→ Weiter:** [Kapitel 32: API Design & REST Principles](chapter_32_api_design_rest_principles.md)

---

## 31.15 Phase-3-Sync: Protokollvergleich & Schnellreferenz {#chapter_31_15_phase3_sync}

> *Quelle: [docs/de/apis/README.md](../../../docs/de/apis/README.md) · [docs/de/apis/GRPC_API_SPECIFICATION.md](../../../docs/de/apis/GRPC_API_SPECIFICATION.md) · [docs/de/rpc_grpc/README.md](../../../docs/de/rpc_grpc/README.md) · [docs/de/apis/HTTP2_HTTP3_PROTOCOL_SUPPORT.md](../../../docs/de/apis/HTTP2_HTTP3_PROTOCOL_SUPPORT.md)*

### 31.15.1 Protokollvergleichstabelle

| Protokoll | Transport | Format | Streaming | Auth | Latenz | Typischer Einsatz |
|-----------|---------|--------|-----------|------|-------|-------------------|
| **REST/HTTP** | HTTP/1.1–HTTP/3 | JSON | ❌ (Polling) | JWT/API-Key | Mittel | CRUD, öffentliche APIs |
| **gRPC** | HTTP/2 | Protocol Buffers | ✅ bi-direkt. | mTLS/JWT | Niedrig | Microservices, interne APIs |
| **GraphQL** | HTTP | JSON | ✅ Subscriptions | JWT/OAuth2 | Mittel | Flexible Datenabfragen, Frontend |
| **HTTP/2** | TCP+TLS | beliebig | ✅ Server Push | TLS | Niedrig | Multiplexing, Performance |
| **HTTP/3 (QUIC)** | UDP+TLS | beliebig | ✅ | TLS 1.3 | Sehr niedrig | Mobile, hoher Paketverlust |
| **WebSocket** | TCP | JSON/Binary | ✅ bi-direkt. | JWT/Cookie | Sehr niedrig | Real-Time, CDC-Streaming |
| **MCP** | HTTP/WebSocket | JSON-RPC | ✅ | OAuth2 | Mittel | LLM-Tool-Integration |

### 31.15.2 Schnellreferenz: ThemisDB API-Endpunkte

#### REST API
```
Basis-URL: https://themis.example.com/api/v1
Authentifizierung: Authorization: ******

GET    /collections/{name}          → Kollektion abrufen
POST   /collections                 → Kollektion erstellen
POST   /collections/{name}/query    → AQL-Abfrage ausführen
POST   /collections/{name}/documents → Dokument einfügen
DELETE /collections/{name}/documents/{id} → Dokument löschen
POST   /vectors/search              → Vector-Suche (k-NN)
```

#### gRPC
```
Basis-URL: grpc://themis.example.com:50051
Proto-Dateien: proto/themis_core.proto, proto/llm_service.proto

Services:
  ThemisCoreService  → CRUD, AQL-Queries
  LLMService         → LLM-Inferenz, Embeddings
  ShardingService    → Distributed Operations
```

#### GraphQL
```
Endpoint: https://themis.example.com/graphql
WebSocket (Subscriptions): wss://themis.example.com/graphql

Schema-Typen: Collection, Document, QueryResult, LLMResponse
```

#### MCP (Model Context Protocol)
```
Transport: HTTP POST + WebSocket
Endpunkt: https://themis.example.com/mcp/v1

Tools: themis_query, themis_insert, themis_search, themis_llm
```

### 31.15.3 Authentifizierungs-Flows

| Flow | Protokoll | Beschreibung |
|------|-----------|-------------|
| **JWT Bearer** | REST, GraphQL, MCP | `Authorization: ****** in Header |
| **mTLS** | gRPC | Gegenseitige TLS-Authentifizierung; Zertifikat in `certs/` |
| **OAuth2 Client Credentials** | REST, MCP | Für Service-to-Service ohne Nutzerinteraktion |
| **API-Key** | REST | `X-ThemisDB-API-Key: <key>` Header |
| **HMAC-Signatur** | Webhooks | SHA-256-Signatur für eingehende Events |

```mermaid
sequenceDiagram
    participant C as Client
    participant T as ThemisDB
    participant A as Auth Service

    C->>A: POST /auth/token (credentials)
    A-->>C: JWT Token (exp: 3600s)
    C->>T: API Request + Authorization: ******
    T->>T: Token-Validierung (lokal oder Introspection)
    T-->>C: API Response
```

**Abb. 31.15.1:** JWT-Authentifizierungsflow für REST und GraphQL.

### 31.15.4 gRPC: TLS/mTLS-Konfiguration

Das `rpc_grpc`-Plugin (Modul-Version 0.3.0) implementiert **fail-closed TLS**:

```cpp
// Konfiguration: src/rpc_grpc/grpc_plugin.h
RPCServerConfig config;
config.host = "0.0.0.0";
config.port = 50051;
config.tls_enabled = true;          // fail-closed: TLS ist Pflicht
config.cert_path = "certs/server.crt";
config.key_path  = "certs/server.key";
config.mtls_ca_path = "certs/ca.crt"; // für mTLS (optional)
config.keepalive_time_ms = 10000;
```

**Primärquelle:** [`src/rpc_grpc/README.md`](../../../docs/de/rpc_grpc/README.md) · [`include/plugins/rpc_plugin_interface.h`](../../../include/plugins/rpc_plugin_interface.h)

### 31.15.5 HTTP/2 & HTTP/3: Protokollunterstützung

Quellen: [`docs/de/apis/HTTP2_HTTP3_PROTOCOL_SUPPORT.md`](../../../docs/de/apis/HTTP2_HTTP3_PROTOCOL_SUPPORT.md) · [`docs/de/apis/HTTP2_HTTP3_USAGE_GUIDE.md`](../../../docs/de/apis/HTTP2_HTTP3_USAGE_GUIDE.md)

| Feature | HTTP/2 | HTTP/3 (QUIC) |
|---------|--------|--------------|
| Transport | TCP+TLS | UDP+TLS 1.3 |
| Multiplexing | ✅ (Streams) | ✅ (QUIC Streams) |
| Header-Kompression | HPACK | QPACK |
| Server Push | ✅ | ⚠️ (deprecated in HTTP/3) |
| 0-RTT Reconnect | ❌ | ✅ |
| Head-of-line Blocking | Ja (TCP) | Nein (QUIC) |
| Empfohlen für | Bestehende Infra | Mobile, hoher Paketverlust |

**Server Push für CDC (HTTP/2):** ThemisDB unterstützt HTTP/2 Server Push für Change-Data-Capture-Events — dokumentiert in [`docs/de/apis/HTTP2_SERVER_PUSH_CDC.md`](../../../docs/de/apis/HTTP2_SERVER_PUSH_CDC.md).

---

## 31.16 Phase-3-Sync: Querverweis-Index {#chapter_31_16_cross_references}

**Bidirektionale Verweise — Level-1/2 Primärquellen:**

| Thema | Primärquelle (Level 1) | docs/de-Kompendiumsquelle |
|-------|----------------------|--------------------------|
| API Modul Übersicht | [`src/api/README.md`](../../../src/api/README.md) | [`docs/de/apis/README.md`](../../../docs/de/apis/README.md) |
| REST API Spezifikation | [`src/api/ARCHITECTURE.md`](../../../src/api/ARCHITECTURE.md) | [`docs/de/apis/REST_API_SPECIFICATION.md`](../../../docs/de/apis/REST_API_SPECIFICATION.md) |
| gRPC Spezifikation | `proto/themis_core.proto` | [`docs/de/apis/GRPC_API_SPECIFICATION.md`](../../../docs/de/apis/GRPC_API_SPECIFICATION.md) |
| gRPC Plugin | `src/rpc_grpc/grpc_plugin.h/.cpp` | [`docs/de/rpc_grpc/README.md`](../../../docs/de/rpc_grpc/README.md) |
| GraphQL Spezifikation | `src/api/graphql/` | [`docs/de/apis/GRAPHQL_API_SPECIFICATION.md`](../../../docs/de/apis/GRAPHQL_API_SPECIFICATION.md) |
| HTTP/2+3 Support | `src/api/http2_handler.cpp` | [`docs/de/apis/HTTP2_HTTP3_PROTOCOL_SUPPORT.md`](../../../docs/de/apis/HTTP2_HTTP3_PROTOCOL_SUPPORT.md) |
| MCP Integration | `src/mcp/` | [`docs/de/apis/MCP_API_SPECIFICATION.md`](../../../docs/de/apis/MCP_API_SPECIFICATION.md) |

**→ Verwandte Kapitel:** [Kapitel 22 (Client Libraries)](chapter_22_clients.md) · [Kapitel 30 (Deployment)](chapter_30_deployment_operations.md) · [Kapitel 32 (API Design)](chapter_32_api_design_rest_principles.md) · [Kapitel 36 (Security)](chapter_36_security_hardening.md)
