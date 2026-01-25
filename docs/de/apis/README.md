# 🔌 API & Ingestion Dokumentation

> **Kategorie:** Core API  
> **Seit Version:** 1.3.0  
> **Status:** ✅ Stable  
> **Aktualisiert:** Januar 2026

---

## 📋 Inhaltsverzeichnis

- [🎯 Übersicht](#-übersicht)
- [📊 Verfügbare APIs](#-verfügbare-apis)
- [🚀 Erste Schritte](#-erste-schritte)
- [📖 API-Dokumentation](#-api-dokumentation)
- [💡 Best Practices](#-best-practices)
- [🔧 Troubleshooting](#-troubleshooting)
- [📚 Siehe auch](#-siehe-auch)

---

## 🎯 Übersicht

Daten-Ingestion, Abfragen und API-Dokumentation für ThemisDB. Diese Dokumentation deckt alle HTTP-APIs, GraphQL, gRPC, MCP und weitere Protokolle ab.

### Vollständige API-Spezifikationen (v1.4)

ThemisDB v1.4 bietet umfassende API-Dokumentation für alle unterstützten Protokolle:

- **[REST API Specification](REST_API_SPECIFICATION.md)** - Vollständige REST API mit CRUD-Operationen, AQL-Queries, Authentifizierung, Versionierung und über 10 praxisnahen Beispielen
- **[gRPC API Specification](GRPC_API_SPECIFICATION.md)** - Protocol Buffers Definitionen, Streaming, Code-Beispiele in Go, Python, C++, Java
- **[GraphQL API Specification](GRAPHQL_API_SPECIFICATION.md)** - Schema-Definition, Queries, Mutations, Subscriptions, Client-Integration
- **[MCP API Specification](MCP_API_SPECIFICATION.md)** - Model Context Protocol für LLM-Integration, Claude Desktop Setup, Tool-Definitionen

## Source-Code Referenz

| Komponente | Header | Source |
|------------|--------|--------|
| ContentManager | `include/content/content_manager.h` | `src/content/content_manager.cpp` |
| ContentProcessor | `include/content/content_processor.h` | `src/content/content_processor.cpp` |

## 📊 Verfügbare APIs

### REST API
**Datei:** [REST_API_SPECIFICATION.md](REST_API_SPECIFICATION.md)

- Vollständige HTTP/HTTPS API
- CRUD-Operationen für Dokumente
- AQL Query Execution
- Transaktionen
- Batch-Operationen
- API Versioning mit Accept-Version Header
- Rate Limiting und Error Handling

**Beispiel:**
```bash
curl -X POST https://api.themisdb.com/api/v1/query \
  -H "Authorization: Bearer <token>" \
  -d '{"query": "FOR doc IN users FILTER doc.age > 25 RETURN doc"}'
```

### gRPC API
**Datei:** [GRPC_API_SPECIFICATION.md](GRPC_API_SPECIFICATION.md)

- High-Performance binäres Protokoll
- Protocol Buffers Definitionen
- Bidirektionales Streaming
- Multi-Language Support (Go, Python, C++, Java, etc.)
- LLM Services Integration
- Sharding/Distributed Operations

**Proto Files:**
- `proto/themis_core.proto` - Core Database Services
- `proto/llm_service.proto` - LLM Integration
- `proto/sharding/shard_rpc.proto` - Distributed Sharding

### GraphQL API
**Datei:** [GRAPHQL_API_SPECIFICATION.md](GRAPHQL_API_SPECIFICATION.md)

- Flexible Query Language
- Single Endpoint für alle Operationen
- Strong Typing mit Schema
- Subscriptions für Real-time Updates
- Client Libraries für JS, Python, Go

**Schema Types:**
- `Document`, `Collection`, `Database`
- `Query`, `Mutation`, `Subscription`
- Full-Text und Vector Search Integration

### MCP API
**Datei:** [MCP_API_SPECIFICATION.md](MCP_API_SPECIFICATION.md)

- Model Context Protocol für LLM-Integration
- JSON-RPC 2.0 basiert
- Tools (Database Operations), Resources (Schema/Metadata), Prompts
- Multi-Transport: stdio, HTTP, WebSocket
- Claude Desktop Integration
- LangChain kompatibel

**Use Cases:**
- AI-powered database queries
- Natural language to AQL translation
- Semantic search with LLMs
- Question Answering Systems

## 🚀 Erste Schritte

### 1. REST API Quickstart

```bash
# Login und Token erhalten
curl -X POST https://api.themisdb.com/api/v1/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username": "admin", "password": "your_password"}'

# Dokument erstellen
curl -X POST https://api.themisdb.com/api/v1/documents/mydb/users \
  -H "Authorization: Bearer <token>" \
  -H "Content-Type: application/json" \
  -d '{"name": "John Doe", "email": "john@example.com"}'
```

### 2. gRPC Quickstart (Python)

```python
import grpc
import themis_pb2
import themis_pb2_grpc

channel = grpc.insecure_channel('localhost:50051')
stub = themis_pb2_grpc.ThemisCoreServiceStub(channel)

request = themis_pb2.ExecuteQueryRequest(
    database='mydb',
    query='FOR doc IN users RETURN doc'
)

response = stub.ExecuteQuery(request)
```

### 3. GraphQL Quickstart

```graphql
query GetUser {
  document(database: "mydb", collection: "users", id: "user_123") {
    id
    data
    createdAt
  }
}
```

### 4. MCP Quickstart

```json
{
  "mcpServers": {
    "themisdb": {
      "command": "themis-mcp-server",
      "args": ["--database", "mydb"],
      "env": {"THEMIS_TOKEN": "your-jwt-token"}
    }
  }
}
```

## 📖 Zusätzliche Dokumentation

| Datei | Beschreibung |
|-------|--------------|
| [json_ingestion_spec.md](json_ingestion_spec.md) | JSON Ingestion Specification |
| [apis_graphql.md](apis_graphql.md) | Erweiterte GraphQL Dokumentation |
| [apis_openapi.md](apis_openapi.md) | OpenAPI/Swagger Spezifikation |
| [MCP_PROTOCOL_SUPPORT.md](MCP_PROTOCOL_SUPPORT.md) | MCP Protokoll Details |
| [HTTP2_HTTP3_PROTOCOL_SUPPORT.md](HTTP2_HTTP3_PROTOCOL_SUPPORT.md) | Moderne HTTP Protokolle |

## 💡 Best Practices

### Authentifizierung
- Verwenden Sie JWT Bearer Tokens
- Erneuern Sie Tokens regelmäßig
- Speichern Sie Tokens sicher (nicht in Git)
- Nutzen Sie HTTPS für alle Requests

### Performance
- Verwenden Sie Batch-Operations für mehrere Dokumente
- Aktivieren Sie Caching wo möglich
- Nutzen Sie Connection Pooling
- Implementieren Sie Pagination für große Ergebnismengen

### Error Handling
- Prüfen Sie HTTP Status Codes
- Implementieren Sie Retry-Logic mit Exponential Backoff
- Loggen Sie API-Fehler für Debugging
- Nutzen Sie Error Codes für spezifische Behandlung

## Verwandte Dokumentation

- [AQL Syntax Guide](../aql/AQL_SYNTAX_GUIDE.md) - Query Language Reference
- [Search Guides](../search/) - Full-Text, Vector, Hybrid Search
- [Authentication & Security](../security/) - RBAC, Encryption, PKI
- [Content Module](../content/README.md) - Content Processing Pipeline
