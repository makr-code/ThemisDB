# API Documentation

**Stand:** 5. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** API

---

## Übersicht

ThemisDB bietet REST APIs für Datenbank-Operationen, Content-Management und LLM-Integration.

## API Endpoints

| Endpoint | Handler | Beschreibung |
|----------|---------|--------------|
| `/api/entities` | EntityHandler | CRUD Operations |
| `/api/query` | QueryHandler | AQL Query Execution |
| `/api/graph` | GraphHandler | Graph Traversal |
| `/api/vector` | VectorHandler | Vector Search |
| `/api/content` | ContentHandler | Content Upload/Download |
| `/api/audit` | AuditApiHandler | Audit Logs |
| `/api/saga` | SAGAApiHandler | Distributed Transactions |
| `/api/pii` | PIIApiHandler | PII Detection/Masking |
| `/api/keys` | KeysApiHandler | Key Management |
| `/api/classification` | ClassificationApiHandler | Data Classification |
| `/api/retention` | RetentionApiHandler | Retention Policies |
| `/api/reports` | ReportsApiHandler | Report Generation |

## Source-Code Referenz

Die API-Handler sind implementiert in:
- `include/server/http_server.h` - HTTP Server
- `src/server/*_api_handler.cpp` - API Handler Implementierungen

## Dokumentation in diesem Ordner

| Datei | Beschreibung |
|-------|--------------|
| [STREAMING_JSONL_TRAINING.md](STREAMING_JSONL_TRAINING.md) | Streaming JSONL API für LLM Training |
| [VCC_CLARA_EXPORT_API.md](VCC_CLARA_EXPORT_API.md) | VCC Clara Export API |

## Verwandte Dokumentation

- [OpenAPI Spec](../openapi.yaml) - OpenAPI 3.0 Specification
- [Server Module](../server/README.md) - Server Implementation
- [AQL Documentation](../aql/README.md) - Query Language
