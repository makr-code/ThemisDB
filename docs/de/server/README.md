# Server Module

**Stand:** 9. März 2026
**Version:** 1.x
**Kategorie:** 🖥️ Server
**Validated:** 2026-03-09
**Status:** current

---

**Primäre Dokumentation:** [`src/server/README.md`](../../../src/server/README.md)
**Roadmap:** [`src/server/ROADMAP.md`](../../../src/server/ROADMAP.md)
**Fehlende Implementierungen:** [`missing-implementations.md`](./missing-implementations.md)

---

## Übersicht

Das Server-Modul stellt ThemisDBs vollständige API-Oberfläche, Netzwerkprotokoll-Implementierungen
und client-seitige Dienste bereit. Aufgebaut auf Boost.Beast und Boost.Asio unterstützt es
HTTP/1.1, HTTP/2, HTTP/3, WebSocket, MQTT, PostgreSQL-Wire-Protokoll und gRPC mit über 100
spezialisierten REST-Endpunkten.

## Source-Code Referenz

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| HTTPServer | `http_server.h` | `http_server.cpp` | HTTP/HTTPS Async-I/O-Server (Boost.Beast) |
| HTTP2Session | `http2_session.h` | `http2_session.cpp` | HTTP/2 Multiplexing, Server Push, HPACK |
| HTTP3Session | `http3_session.h` | `http3_session.cpp` | HTTP/3 über QUIC |
| HTTP3Datagram | `http3_datagram.h` | `http3_datagram.cpp` | HTTP/3-Datagram-Unterstützung |
| APIGateway | `api_gateway.h` | `api_gateway.cpp` | Routing, Versionierung, Federation |
| OpenAPIRouteRegistry | `openapi_route_registry.h` | `openapi_route_registry.cpp` | OpenAPI 3.1 Spec-Autogenerierung |
| AuthMiddleware | `auth_middleware.h` | `auth_middleware.cpp` | JWT, Kerberos, API-Token, USB-Admin-Auth |
| RateLimiter | `rate_limiter.h` | `rate_limiter.cpp` | Token-Bucket Rate Limiting |
| RateLimiterV2 | `rate_limiter_v2.h` | `rate_limiter_v2.cpp` | Sliding-Window / Distributed Rate Limiting |
| RateLimitingMiddleware | `rate_limiting_middleware.h` | `rate_limiting_middleware.cpp` | Rate-Limiting-Middleware |
| LoadShedder | `load_shedder.h` | `load_shedder.cpp` | Load Shedding und Circuit Breaking |
| RequestValidationMiddleware | `request_validation_middleware.h` | `request_validation_middleware.cpp` | JSON-Schema-Validierung |
| CDNCacheMiddleware | `cdn_cache_middleware.h` | `cdn_cache_middleware.cpp` | CDN/Edge Cache-Control |
| TenantManager | `tenant_manager.h` | `tenant_manager.cpp` | Mandantenisolierung |
| PolicyEngine | `policy_engine.h` | `policy_engine.cpp` | Policy-Durchsetzung |
| OPAAdapter | `opa_adapter.h` | `opa_adapter.cpp` | Open Policy Agent Integration |
| RangerAdapter | `ranger_adapter.h` | `ranger_adapter.cpp` | Apache Ranger Integration |
| WebSocketSession | `websocket_session.h` | `websocket_session.cpp` | WebSocket-Sitzungen |
| MQTTSession | `mqtt_session.h` | `mqtt_session.cpp` | MQTT-Broker-Integration |
| PostgresSession | `postgres_session.h` | `postgres_session.cpp` | PostgreSQL-Wire-Protokoll |
| SSEConnectionManager | `sse_connection_manager.h` | `sse_connection_manager.cpp` | Server-Sent Events |
| MCPServer | `mcp_server.h` | `mcp_server.cpp` | Model Context Protocol |
| ChunkedResponseWriter | `chunked_response_writer.h` | `chunked_response_writer.cpp` | Streaming-Antworten |
| EntityAPIHandler | `entity_api_handler.h` | `entity_api_handler.cpp` | Entitäts-CRUD |
| QueryAPIHandler | `query_api_handler.h` | `query_api_handler.cpp` | AQL/SQL-Abfragen |
| VectorAPIHandler | `vector_api_handler.h` | `vector_api_handler.cpp` | Vektor-Einbettungen |
| GraphAPIHandler | `graph_api_handler.h` | `graph_api_handler.cpp` | Graph-Traversal |
| TimeseriesAPIHandler | `timeseries_api_handler.h` | `timeseries_api_handler.cpp` | Zeitreihen |
| SpatialAPIHandler | `spatial_api_handler.h` | `spatial_api_handler.cpp` | Geo/Spatial |
| LLMAPIHandler | `llm_api_handler.h` | `llm_api_handler.cpp` | LLM INFER/RAG/EMBED |
| AsyncJobAPIHandler | `async_job_api_handler.h` | `async_job_api_handler.cpp` | Asynchrone Jobs |
| AuditAPIHandler | `audit_api_handler.h` | `audit_api_handler.cpp` | Audit-Logs |
| MonitoringAPIHandler | `monitoring_api_handler.h` | `monitoring_api_handler.cpp` | Monitoring/Metriken |
| GraphQLAPIHandler | `graphql_api_handler.h` | `graphql_api_handler.cpp` | GraphQL (in Entwicklung) |
| ComplianceReportingAPIHandler | `compliance_reporting_api_handler.h` | `compliance_reporting_api_handler.cpp` | Compliance-Reporting |
| ServerlessFunctionAPIHandler | `serverless_function_api_handler.h` | `serverless_function_api_handler.cpp` | Serverless-Functions |
| PIIAPIHandler | `pii_api_handler.h` | `pii_api_handler.cpp` | PII-Verwaltung |
| RetentionAPIHandler | `retention_api_handler.h` | `retention_api_handler.cpp` | Datenaufbewahrung |

**Gesamt:** ~100 Header, ~100 Source-Dateien in `src/server/` und `include/server/`

## HttpServer

```cpp
class HttpServer {
    // Async HTTP/REST API Server mit Boost.Beast
    
    struct Config {
        std::string host = "0.0.0.0";
        uint16_t port = 8080;
        size_t num_threads = 4;
        bool enable_ssl = false;
        std::string cert_path;
        std::string key_path;
    };
    
    // Lifecycle
    void start();
    void stop();
    
    // Route Registration
    void registerHandler(method, path, handler);
};
```

## API Handler

Alle API-Handler folgen einem einheitlichen Pattern:

```cpp
class AuditApiHandler {
    // GET /api/audit/events - Liste Audit-Events
    // GET /api/audit/events/{id} - Einzelnes Event
    // POST /api/audit/query - Audit-Suche
    
    http::response<http::string_body> handleRequest(
        const http::request<http::string_body>& req);
};
```

## Haupt-API-Endpoints

| Endpoint | Handler | Beschreibung |
|----------|---------|--------------|
| `/api/audit/*` | AuditApiHandler | Audit Logs |
| `/api/saga/*` | SAGAApiHandler | Distributed Transactions |
| `/api/pii/*` | PIIApiHandler | PII Detection/Masking |
| `/api/keys/*` | KeysApiHandler | Key Management |
| `/api/classification/*` | ClassificationApiHandler | Data Classification |
| `/api/retention/*` | RetentionApiHandler | Retention Policies |
| `/api/reports/*` | ReportsApiHandler | Report Generation |

## Security Features

### AuthMiddleware
```cpp
class AuthMiddleware {
    bool validateJWT(const std::string& token);
    std::optional<Claims> extractClaims(const std::string& token);
};
```

### RateLimiter
```cpp
class RateLimiter {
    bool allowRequest(const std::string& client_id);
    void setLimit(int requests_per_second);
};
```

### Apache Ranger Integration
```cpp
class RangerAdapter {
    bool authorize(user, resource, action);
    std::vector<Policy> getPolicies(resource);
};
```

## Verwandte Dokumentation

- [APIs: OpenAPI](../apis/apis_openapi.md) - OpenAPI Spec
- [APIs: GraphQL](../apis/apis_graphql.md) - GraphQL Schema
- [Security: RBAC](../security/security_rbac.md) - Access Control
| [server_security.md](./server_security.md) | TLS, Auth, Rate Limiting | 📋 TODO |
| [server_performance.md](./server_performance.md) | Benchmarks & Tuning | 📋 TODO |

## Verwandte Dokumentation

- [Deployment Guide](../guides/guides_deployment.md)
- [TLS Setup](../guides/guides_tls_setup.md)
- [RBAC](../guides/guides_rbac.md)
- [Enterprise Features](../enterprise/README.md)
