# REST API Specification - ThemisDB v1.4

**Version:** 1.4.0  
**Status:** ✅ Produktionsreif  
**Aktualisiert:** Januar 2026

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Basis-URL und Endpunkte](#basis-url-und-endpunkte)
- [Authentifizierung](#authentifizierung)
- [Versionierung](#versionierung)
- [Datenbank-Operationen](#datenbank-operationen)
- [Dokument-Operationen](#dokument-operationen)
- [Query-Operationen](#query-operationen)
- [Index-Operationen](#index-operationen)
- [Fehlerbehandlung](#fehlerbehandlung)
- [Rate Limiting](#rate-limiting)
- [Beispiel-Requests und Responses](#beispiel-requests-und-responses)

---

## Übersicht

Die ThemisDB REST API bietet vollständigen Zugriff auf alle Datenbankfunktionen über HTTP/HTTPS. Die API folgt RESTful-Prinzipien und verwendet JSON für Daten-Serialisierung.

### API-Features

- ✅ **Multi-Model Support**: Relationale, Dokument-, Graph- und Vektordaten
- ✅ **AQL Query Execution**: Advanced Query Language über REST
- ✅ **Transaction Support**: ACID-Transaktionen mit Rollback
- ✅ **Streaming Responses**: Server-Sent Events für große Ergebnismengen
- ✅ **Batch Operations**: Mehrere Operationen in einem Request
- ✅ **API Versioning**: Accept-Version Header für Abwärtskompatibilität

---

## Basis-URL und Endpunkte

### Basis-URL

```
https://your-themis-instance.com/api/v1
```

### Haupt-Endpunkte

| Kategorie | Endpunkt | Beschreibung |
|-----------|----------|--------------|
| Datenbanken | `/databases` | Datenbank-Management |
| Kollektionen | `/collections` | Kollektion-Management |
| Dokumente | `/documents` | CRUD-Operationen für Dokumente |
| Queries | `/query` | AQL Query Execution |
| Indizes | `/indexes` | Index-Management |
| Transaktionen | `/transactions` | Transaction Management |
| Benutzer | `/users` | Benutzer- und Rechteverwaltung |
| Monitoring | `/monitoring` | Metriken und Statistiken |

---

## Authentifizierung

### JWT Bearer Token

Alle API-Requests erfordern Authentifizierung via JWT Bearer Token:

```http
Authorization: Bearer <your-jwt-token>
```

### Token erhalten

**Endpoint:** `POST /api/v1/auth/login`

**Request:**
```json
{
  "username": "admin",
  "password": "secure_password"
}
```

**Response:**
```json
{
  "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
  "expires_in": 3600,
  "user": {
    "username": "admin",
    "roles": ["admin", "read", "write"]
  }
}
```

---

## Versionierung

ThemisDB unterstützt API-Versionierung über den `Accept-Version` Header:

```http
Accept-Version: 1.4
```

**Versioning-Strategie:**
- **Minor Versions (1.x)**: Abwärtskompatible Änderungen
- **Major Versions (x.0)**: Breaking Changes (mit 24 Monaten Deprecation Period)
- **Deprecation Headers**: Automatische Warnungen bei veralteten Endpoints

**Response Headers:**
```http
API-Version: 1.4
Deprecation: Sun, 01 Jan 2027 00:00:00 GMT
Sunset: Sun, 01 Jan 2028 00:00:00 GMT
Link: </api/v2/new-endpoint>; rel="successor"
```

---

## Datenbank-Operationen

### Datenbank erstellen

**Endpoint:** `POST /api/v1/databases`

**Request:**
```json
{
  "name": "mydb",
  "options": {
    "storage_engine": "rocksdb",
    "replication_factor": 3,
    "sharding_strategy": "hash"
  }
}
```

**Response:** `201 Created`
```json
{
  "id": "db_12345",
  "name": "mydb",
  "created_at": "2026-01-24T14:00:00Z",
  "status": "online"
}
```

### Datenbank auflisten

**Endpoint:** `GET /api/v1/databases`

**Response:** `200 OK`
```json
{
  "databases": [
    {
      "id": "db_12345",
      "name": "mydb",
      "collections": 5,
      "size_bytes": 1048576,
      "status": "online"
    }
  ],
  "total": 1
}
```

### Datenbank löschen

**Endpoint:** `DELETE /api/v1/databases/{database_name}`

**Response:** `204 No Content`

---

## Dokument-Operationen

### Dokument erstellen

**Endpoint:** `POST /api/v1/documents/{database}/{collection}`

**Request:**
```json
{
  "name": "John Doe",
  "email": "john@example.com",
  "age": 30,
  "tags": ["customer", "premium"]
}
```

**Response:** `201 Created`
```json
{
  "id": "doc_67890",
  "rev": "1-abc123",
  "created_at": "2026-01-24T14:00:00Z"
}
```

### Dokument abrufen

**Endpoint:** `GET /api/v1/documents/{database}/{collection}/{doc_id}`

**Response:** `200 OK`
```json
{
  "id": "doc_67890",
  "rev": "1-abc123",
  "name": "John Doe",
  "email": "john@example.com",
  "age": 30,
  "tags": ["customer", "premium"],
  "created_at": "2026-01-24T14:00:00Z",
  "updated_at": "2026-01-24T14:00:00Z"
}
```

### Dokument aktualisieren

**Endpoint:** `PUT /api/v1/documents/{database}/{collection}/{doc_id}`

**Request:**
```json
{
  "rev": "1-abc123",
  "name": "John Doe",
  "email": "john.doe@example.com",
  "age": 31,
  "tags": ["customer", "premium", "vip"]
}
```

**Response:** `200 OK`
```json
{
  "id": "doc_67890",
  "rev": "2-def456",
  "updated_at": "2026-01-24T15:00:00Z"
}
```

### Dokument löschen

**Endpoint:** `DELETE /api/v1/documents/{database}/{collection}/{doc_id}?rev=1-abc123`

**Response:** `204 No Content`

### Batch-Insert

**Endpoint:** `POST /api/v1/documents/{database}/{collection}/batch`

**Request:**
```json
{
  "documents": [
    {"name": "Alice", "age": 25},
    {"name": "Bob", "age": 30},
    {"name": "Charlie", "age": 35}
  ]
}
```

**Response:** `201 Created`
```json
{
  "inserted": 3,
  "ids": ["doc_111", "doc_222", "doc_333"],
  "errors": []
}
```

---

## Query-Operationen

### AQL Query ausführen

**Endpoint:** `POST /api/v1/query`

**Request:**
```json
{
  "query": "FOR doc IN users FILTER doc.age > @minAge SORT doc.age DESC LIMIT 10 RETURN doc",
  "bind_vars": {
    "minAge": 25
  },
  "options": {
    "batch_size": 100,
    "ttl": 30,
    "count": true
  }
}
```

**Response:** `200 OK`
```json
{
  "result": [
    {"name": "John", "age": 35},
    {"name": "Alice", "age": 30},
    {"name": "Bob", "age": 28}
  ],
  "count": 3,
  "has_more": false,
  "stats": {
    "execution_time_ms": 45,
    "scanned": 100,
    "filtered": 3
  }
}
```

### Streaming Query (Server-Sent Events)

**Endpoint:** `POST /api/v1/query/stream`

**Request:** (gleich wie oben)

**Response:** `200 OK` mit `Content-Type: text/event-stream`

```
data: {"name": "John", "age": 35}

data: {"name": "Alice", "age": 30}

data: {"name": "Bob", "age": 28}

event: done
data: {"count": 3, "execution_time_ms": 45}
```

---

## Index-Operationen

### Index erstellen

**Endpoint:** `POST /api/v1/indexes/{database}/{collection}`

**Request:**
```json
{
  "type": "hash",
  "fields": ["email"],
  "unique": true,
  "sparse": false,
  "name": "idx_email"
}
```

**Response:** `201 Created`
```json
{
  "id": "idx_12345",
  "name": "idx_email",
  "type": "hash",
  "fields": ["email"],
  "unique": true,
  "created_at": "2026-01-24T14:00:00Z"
}
```

### Indizes auflisten

**Endpoint:** `GET /api/v1/indexes/{database}/{collection}`

**Response:** `200 OK`
```json
{
  "indexes": [
    {
      "id": "idx_12345",
      "name": "idx_email",
      "type": "hash",
      "fields": ["email"],
      "unique": true,
      "size_bytes": 8192
    }
  ],
  "total": 1
}
```

---

## Fehlerbehandlung

### Fehler-Response-Format

```json
{
  "error": true,
  "code": 404,
  "error_code": "DOCUMENT_NOT_FOUND",
  "message": "Document with ID 'doc_67890' not found in collection 'users'",
  "details": {
    "database": "mydb",
    "collection": "users",
    "document_id": "doc_67890"
  }
}
```

### Standard HTTP Status Codes

| Code | Bedeutung | Verwendung |
|------|-----------|-----------|
| 200 | OK | Erfolgreicher Request |
| 201 | Created | Ressource erfolgreich erstellt |
| 204 | No Content | Erfolgreiche Löschung |
| 400 | Bad Request | Ungültige Request-Parameter |
| 401 | Unauthorized | Fehlende oder ungültige Authentifizierung |
| 403 | Forbidden | Keine Berechtigung |
| 404 | Not Found | Ressource nicht gefunden |
| 409 | Conflict | Konflikt (z.B. Duplikat) |
| 429 | Too Many Requests | Rate Limit überschritten |
| 500 | Internal Server Error | Server-Fehler |
| 503 | Service Unavailable | Service temporär nicht verfügbar |

---

## Rate Limiting

ThemisDB implementiert Rate Limiting zum Schutz der API:

**Headers:**
```http
X-RateLimit-Limit: 1000
X-RateLimit-Remaining: 995
X-RateLimit-Reset: 1706104800
```

**Bei Überschreitung:**
```http
HTTP/1.1 429 Too Many Requests
Retry-After: 60
```

```json
{
  "error": true,
  "code": 429,
  "error_code": "RATE_LIMIT_EXCEEDED",
  "message": "Rate limit of 1000 requests per hour exceeded",
  "retry_after": 60
}
```

---

## Beispiel-Requests und Responses

### Beispiel 1: Vollständige CRUD-Operation

#### 1. Dokument erstellen
```bash
curl -X POST https://api.themisdb.com/api/v1/documents/mydb/users \
  -H "Authorization: Bearer <token>" \
  -H "Content-Type: application/json" \
  -d '{
    "name": "Jane Smith",
    "email": "jane@example.com",
    "role": "developer"
  }'
```

#### 2. Dokument lesen
```bash
curl -X GET https://api.themisdb.com/api/v1/documents/mydb/users/doc_12345 \
  -H "Authorization: Bearer <token>"
```

#### 3. Dokument aktualisieren
```bash
curl -X PUT https://api.themisdb.com/api/v1/documents/mydb/users/doc_12345 \
  -H "Authorization: Bearer <token>" \
  -H "Content-Type: application/json" \
  -d '{
    "rev": "1-abc",
    "name": "Jane Smith",
    "email": "jane.smith@example.com",
    "role": "senior-developer"
  }'
```

#### 4. Dokument löschen
```bash
curl -X DELETE https://api.themisdb.com/api/v1/documents/mydb/users/doc_12345?rev=2-def \
  -H "Authorization: Bearer <token>"
```

### Beispiel 2: Komplexe AQL Query

```bash
curl -X POST https://api.themisdb.com/api/v1/query \
  -H "Authorization: Bearer <token>" \
  -H "Content-Type: application/json" \
  -d '{
    "query": "FOR u IN users FILTER u.role == @role LET projects = (FOR p IN projects FILTER p.assignee == u._id RETURN p) RETURN {user: u, project_count: LENGTH(projects), projects: projects}",
    "bind_vars": {
      "role": "developer"
    }
  }'
```

**Response:**
```json
{
  "result": [
    {
      "user": {
        "name": "Jane Smith",
        "email": "jane.smith@example.com",
        "role": "developer"
      },
      "project_count": 3,
      "projects": [
        {"name": "Project A", "status": "active"},
        {"name": "Project B", "status": "completed"},
        {"name": "Project C", "status": "planning"}
      ]
    }
  ],
  "count": 1,
  "has_more": false
}
```

### Beispiel 3: Transaction mit mehreren Operationen

```bash
curl -X POST https://api.themisdb.com/api/v1/transactions \
  -H "Authorization: Bearer <token>" \
  -H "Content-Type: application/json" \
  -d '{
    "operations": [
      {
        "type": "insert",
        "collection": "orders",
        "document": {"product": "Widget", "quantity": 5, "user_id": "user_123"}
      },
      {
        "type": "update",
        "collection": "inventory",
        "filter": {"product": "Widget"},
        "update": {"$inc": {"stock": -5}}
      },
      {
        "type": "insert",
        "collection": "transactions",
        "document": {"type": "sale", "amount": 50.00, "user_id": "user_123"}
      }
    ]
  }'
```

---

## Best Practices

### Performance-Optimierung

1. **Batch Operations verwenden**: Nutzen Sie Batch-Endpoints für mehrere Dokumente
2. **Indizes richtig setzen**: Erstellen Sie Indizes für häufig abgefragte Felder
3. **Pagination nutzen**: Verwenden Sie `limit` und `offset` für große Ergebnismengen
4. **Projektion verwenden**: Selektieren Sie nur benötigte Felder mit `RETURN {field1: doc.field1}`
5. **Connection Pooling**: Wiederverwendung von HTTP-Connections

### Sicherheit

1. **HTTPS verwenden**: Immer verschlüsselte Verbindungen
2. **Tokens sicher speichern**: Niemals in Version Control committen
3. **Token Rotation**: Regelmäßige Erneuerung von JWT-Tokens
4. **Input Validation**: Alle Eingaben validieren und sanitizen
5. **Rate Limiting beachten**: Anfragen entsprechend verteilen

---

## Siehe auch

- [API Versioning Strategy](API_VERSIONING.md)
- [GraphQL API](GRAPHQL_API.md)
- [gRPC API](GRPC_API.md)
- [AQL Syntax Guide](../aql/AQL_SYNTAX_GUIDE.md)
- [Authentifizierung und RBAC](../security/authentication.md)
