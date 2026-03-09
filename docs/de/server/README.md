# Server Module

**Stand:** 22. Dezember 2025  
**Version:** v1.3.0  
**Kategorie:** 🖥️ Server

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Source-Code Referenz](#source-code-referenz)
- [Implementierung](#implementierung)

---

## Übersicht

Das Server-Modul implementiert den HTTP/REST API Server für ThemisDB mit Boost.Beast und Boost.Asio.

## Source-Code Referenz

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| HttpServer | `http_server.h` | `http_server.cpp` | Async HTTP Server |
| AuthMiddleware | `auth_middleware.h` | `auth_middleware.cpp` | JWT Authentication |
| RateLimiter | `rate_limiter.h` | `rate_limiter.cpp` | Request Throttling |
| RangerAdapter | `ranger_adapter.h` | `ranger_adapter.cpp` | Apache Ranger |
| SSEConnectionManager | `sse_connection_manager.h` | `sse_connection_manager.cpp` | Server-Sent Events |
| AuditApiHandler | `audit_api_handler.h` | `audit_api_handler.cpp` | Audit API |
| SAGAApiHandler | `saga_api_handler.h` | `saga_api_handler.cpp` | SAGA Transactions |
| PIIApiHandler | `pii_api_handler.h` | `pii_api_handler.cpp` | PII Detection |
| KeysApiHandler | `keys_api_handler.h` | `keys_api_handler.cpp` | Key Management |
| ClassificationApiHandler | `classification_api_handler.h` | `classification_api_handler.cpp` | Data Classification |
| RetentionApiHandler | `retention_api_handler.h` | `retention_api_handler.cpp` | Data Retention |
| ReportsApiHandler | `reports_api_handler.h` | `reports_api_handler.cpp` | Report Generation |

**Gesamt:** 20 Header, 20 Source-Dateien, ~18,000 LOC

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
