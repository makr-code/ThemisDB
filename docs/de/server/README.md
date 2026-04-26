# Server Module

**Stand:** 6. April 2026  
**Version:** v1.7.0  
**Status:** `current`  
**Validiert:** 2026-03-10 (Commit `a04b89b`)  
**Kategorie:** 🖥️ Server

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Protokoll-Unterstützung](#protokoll-unterstützung)
- [Source-Code Referenz](#source-code-referenz)
- [API-Endpunkte](#api-endpunkte)
- [Sicherheitsfunktionen](#sicherheitsfunktionen)
- [Performance](#performance)
- [Geplante Erweiterungen](#geplante-erweiterungen)
- [Fehlende Implementierungen](#fehlende-implementierungen)
- [Verwandte Dokumentation](#verwandte-dokumentation)

---

## Übersicht

Das Server-Modul implementiert die vollständige API-Oberfläche von ThemisDB mit Boost.Beast
und Boost.Asio. Es unterstützt HTTP/1.1, HTTP/2, HTTP/3 (QUIC), WebSocket, MQTT, PostgreSQL
Wire-Protokoll und gRPC über 40+ spezialisierte REST-Endpunkte für Multi-Model-Datenbankoperationen,
Governance, LLM-Inferenz, Observability und Administration.

**Umfang (Stand 2026-03-10):**
- 102 Source-Dateien (`src/server/*.cpp`) + 5 RPC-Handler (`src/server/rpc/`)
- 105 Header-Dateien (`include/server/*.h`) + 3 RPC-Header (`include/server/rpc/`)
- ≈ 62.200 LOC in `src/server/` + ≈ 18.500 LOC in `include/server/`
- Primäre Referenz-Dokumentation: [`src/server/README.md`](../../../src/server/README.md) (1342 Zeilen)
- Architektur-Dokumentation: [`src/server/ARCHITECTURE.md`](../../../src/server/ARCHITECTURE.md)
- ROADMAP: [`src/server/ROADMAP.md`](../../../src/server/ROADMAP.md)

---

## Protokoll-Unterstützung

| Protokoll | Port (Standard) | Status | Quelldateien |
|-----------|-----------------|--------|--------------|
| HTTP/1.1 | 8080 | ✅ Produktion | `http_server.cpp` |
| HTTP/2 | 8080 (multiplexed) | ✅ Produktion | `http2_session.cpp` |
| HTTP/3 (QUIC) | 8443 | ✅ Produktion | `http3_session.cpp`, `http3_datagram.cpp` |
| WebSocket | 8080/8443 (Upgrade) | ✅ Produktion | `websocket_session.cpp` |
| MQTT | 1883 | ✅ Produktion | `mqtt_session.cpp` |
| PostgreSQL Wire | 5432 | ✅ Produktion (partiell) | `postgres_session.cpp` |
| gRPC | 9090 | ✅ Produktion | `themis_core_grpc_service.cpp`, `llm_grpc_service.cpp`, `wal_grpc_service.cpp`, `pitr_grpc_service.cpp`, `prompt_engineering_grpc_service.cpp` |
| gRPC-Web | 9091 | ✅ Produktion | `grpc_web_proxy_handler.cpp` |
| MCP (Model Context Protocol) | 8080 (Pfad `/mcp`) | ✅ Produktion | `mcp_server.cpp` |

---

## Source-Code Referenz

### Kern-Komponenten

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| HttpServer | `http_server.h` | `http_server.cpp` | Multi-Protokoll Async-Server (9961 LOC) |
| HTTP2Session | `http2_session.h` | `http2_session.cpp` | HTTP/2 Multiplexing, Server Push |
| HTTP3Session | `http3_session.h` | `http3_session.cpp` | QUIC/HTTP3, Datagramme |
| WebSocketSession | `websocket_session.h` | `websocket_session.cpp` | Binär- und Text-Frames |
| MQTTSession | `mqtt_session.h` | `mqtt_session.cpp` | MQTT Broker-Integration |
| PostgresSession | `postgres_session.h` | `postgres_session.cpp` | PostgreSQL Wire-Protokoll |
| MCPServer | `mcp_server.h` | `mcp_server.cpp` | Model Context Protocol |
| APIGateway | `api_gateway.h` | `api_gateway.cpp` | Routing, Versioning, Load Balancing |

### Authentifizierung & Autorisierung

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| AuthMiddleware | `auth_middleware.h` | `auth_middleware.cpp` | JWT, Kerberos, API-Token, USB-Admin |
| APIAuthConfig | `api_auth_config.h` | `api_auth_config.cpp` | Endpunkt-spezifische Auth-Anforderungen |
| APIKeyMgmtHandler | `api_key_mgmt_handler.h` | `api_key_mgmt_handler.cpp` | API-Schlüsselverwaltung |
| RangerAdapter | `ranger_adapter.h` | `ranger_adapter.cpp` | Apache Ranger Policy-Enforcement |
| OPAAdapter | `opa_adapter.h` | `opa_adapter.cpp` | Open Policy Agent Integration |
| PKIApiHandler | `pki_api_handler.h` | `pki_api_handler.cpp` | PKI/Zertifikat-Verwaltung |

### Rate Limiting & Schutz

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| RateLimiter | `rate_limiter.h` | `rate_limiter.cpp` | Token Bucket, Sliding Window |
| RateLimiterV2 | `rate_limiter_v2.h` | `rate_limiter_v2.cpp` | Erweiterter Rate Limiter (lokal + Redis/distributed) |
| RateLimitingMiddleware | `rate_limiting_middleware.h` | `rate_limiting_middleware.cpp` | Middleware-Integration |
| LoadShedder | `load_shedder.h` | `load_shedder.cpp` | Load Shedding & Circuit Breaking |

### Daten-API-Handler (Auswahl aus 50+ Handlern)

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| EntityApiHandler | `entity_api_handler.h` | `entity_api_handler.cpp` | CRUD für Dokumente/Collections |
| QueryApiHandler | `query_api_handler.h` | `query_api_handler.cpp` | AQL-Query-Ausführung (3526 LOC) |
| GraphApiHandler | `graph_api_handler.h` | `graph_api_handler.cpp` | Graph-Traversal und -Verwaltung |
| VectorApiHandler | `vector_api_handler.h` | `vector_api_handler.cpp` | Vektorsuche und Embedding |
| TimeseriesApiHandler | `timeseries_api_handler.h` | `timeseries_api_handler.cpp` | Zeitreihendaten |
| LLMApiHandler | `llm_api_handler.h` | `llm_api_handler.cpp` | LLM INFER/RAG/EMBED (1639 LOC) |
| LoRAApiHandler | `lora_api_handler.h` | `lora_api_handler.cpp` | LoRA-Adapter-Verwaltung (1320 LOC) |
| GraphQLApiHandler | `graphql_api_handler.h` | `graphql_api_handler.cpp` | GraphQL-Endpunkt |
| ChangefeedApiHandler | `changefeed_api_handler.h` | `changefeed_api_handler.cpp` | CDC/SSE-Subscriptions |
| SSEConnectionManager | `sse_connection_manager.h` | `sse_connection_manager.cpp` | Server-Sent Events |
| AsyncJobApiHandler | `async_job_api_handler.h` | `async_job_api_handler.cpp` | Async Jobs `/v2/jobs` |
| AuditApiHandler | `audit_api_handler.h` | `audit_api_handler.cpp` | Audit-Logs |
| SAGAApiHandler | `saga_api_handler.h` | `saga_api_handler.cpp` | Verteilte SAGA-Transaktionen |
| PIIApiHandler | `pii_api_handler.h` | `pii_api_handler.cpp` | PII-Erkennung/Maskierung |
| KeysApiHandler | `keys_api_handler.h` | `keys_api_handler.cpp` | Schlüsselverwaltung |
| ClassificationApiHandler | `classification_api_handler.h` | `classification_api_handler.cpp` | Datenklassifizierung |
| RetentionApiHandler | `retention_api_handler.h` | `retention_api_handler.cpp` | Aufbewahrungsrichtlinien |
| ReportsApiHandler | `reports_api_handler.h` | `reports_api_handler.cpp` | Berichtserstellung |
| MonitoringApiHandler | `monitoring_api_handler.h` | `monitoring_api_handler.cpp` | Monitoring/Metriken (1768 LOC) |
| ContentApiHandler | `content_api_handler.h` | `content_api_handler.cpp` | Inhaltsverarbeitung |
| VoiceApiHandler | `voice_api_handler.h` | `voice_api_handler.cpp` | Sprach-API (1572 LOC) |
| SpatialApiHandler | `spatial_api_handler.h` | `spatial_api_handler.cpp` | Geo/Spatial-Abfragen |
| SchemaApiHandler | `schema_api_handler.h` | `schema_api_handler.cpp` | Schema-Verwaltung (1398 LOC) |
| ServerlessFunctionApiHandler | `serverless_function_api_handler.h` | `serverless_function_api_handler.cpp` | Serverless-Funktionen |
| ServiceMeshApiHandler | `service_mesh_api_handler.h` | `service_mesh_api_handler.cpp` | Service-Mesh (Envoy xDS) |
| GrpcWebProxyHandler | `grpc_web_proxy_handler.h` | `grpc_web_proxy_handler.cpp` | gRPC-Web-Proxy |
| CDNCacheMiddleware | `cdn_cache_middleware.h` | `cdn_cache_middleware.cpp` | Edge-Caching |
| OpenAPIRouteRegistry | `openapi_route_registry.h` | `openapi_route_registry.cpp` | OpenAPI 3.1 Auto-Generierung |
| RequestValidationMiddleware | `request_validation_middleware.h` | `request_validation_middleware.cpp` | JSON-Schema-Validierung |

### OpenAPI & Versionierung

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| OpenAPIRouteRegistry | `openapi_route_registry.h` | `openapi_route_registry.cpp` | OpenAPI 3.1 aus Handler-Annotationen |
| APIVersion | `api_version.h` | `api_version.cpp` | Versioning-Logik (`/v1/`, `/v2/`) |
| APIVersionConfig | `api_version_config.h` | *(Header-only)* | Versionierungskonfiguration |

---

## API-Endpunkte

| Pfad-Präfix | Handler | Beschreibung |
|-------------|---------|--------------|
| `GET/POST /api/v1/collections/*` | EntityApiHandler | CRUD Dokumente/Collections |
| `POST /api/v1/query` | QueryApiHandler | AQL-Query-Ausführung |
| `POST /api/v1/graph/*` | GraphApiHandler | Graph-Traversal |
| `POST /api/v1/vector/*` | VectorApiHandler | Vektorsuche |
| `GET/POST /api/v1/timeseries/*` | TimeseriesApiHandler | Zeitreihendaten |
| `POST /api/v1/llm/*` | LLMApiHandler | LLM INFER, RAG, EMBED |
| `GET/POST /api/v1/lora/*` | LoRAApiHandler | LoRA-Adapter |
| `POST /graphql` | GraphQLApiHandler | GraphQL |
| `GET /api/v1/changefeeds/*` | ChangefeedApiHandler | CDC-Subscriptions (SSE) |
| `POST/GET/DELETE /v2/jobs[/{id}]` | AsyncJobApiHandler | Async Jobs |
| `GET/POST /api/v1/audit/*` | AuditApiHandler | Audit-Logs |
| `POST /api/v1/saga/*` | SAGAApiHandler | SAGA-Transaktionen |
| `GET/POST /api/v1/pii/*` | PIIApiHandler | PII-Erkennung |
| `GET/POST /api/v1/keys/*` | KeysApiHandler | Schlüsselverwaltung |
| `GET/POST /api/v1/classification/*` | ClassificationApiHandler | Datenklassifizierung |
| `GET/POST /api/v1/retention/*` | RetentionApiHandler | Aufbewahrungsrichtlinien |
| `GET/POST /api/v1/reports/*` | ReportsApiHandler | Berichte |
| `GET /metrics` | MonitoringApiHandler | Prometheus-Metriken |
| `GET/POST /api/v1/content/*` | ContentApiHandler | Inhaltsverarbeitung |
| `GET/POST /api/v1/voice/*` | VoiceApiHandler | Sprach-API |
| `GET/POST /api/v1/spatial/*` | SpatialApiHandler | Geo/Spatial |
| `GET/POST /api/v1/schema/*` | SchemaApiHandler | Schema-Verwaltung |
| `POST/GET /api/v1/functions/*` | ServerlessFunctionApiHandler | Serverless |
| `GET/POST /api/v1/service-mesh/*` | ServiceMeshApiHandler | Service-Mesh |
| `GET /openapi.json` | OpenAPIRouteRegistry | OpenAPI 3.1 Spec |
| `GET/POST /admin/*` | AdminApiHandler | Admin-Endpunkte (USB/Kerberos) |
| `/mcp` | MCPServer | Model Context Protocol |

---

## Sicherheitsfunktionen

### Authentifizierung

```cpp
// Vier unterstützte Auth-Methoden (auth_middleware.cpp):
// 1. JWT Bearer Token
// 2. Kerberos Service Ticket
// 3. API Token (Header: X-API-Key)
// 4. USB Admin Token (Admin-Endpunkte)
class AuthMiddleware {
    bool validateJWT(const std::string& token);       // HMAC-SHA256 / RSA-256
    bool validateKerberos(const std::string& ticket);
    bool validateAPIToken(const std::string& key);
    bool validateUSBAdminToken(const std::string& token);
};
```

### Rate Limiting

```cpp
// Token Bucket (rate_limiter.cpp, rate_limiter_v2.cpp)

// Lokal (Einzelknoten):
TokenBucketRateLimiter::Config cfg;
cfg.capacity    = 1000;
cfg.refill_rate = 100;
cfg.backend     = TokenBucketRateLimiter::Backend::LOCAL;
TokenBucketRateLimiter limiter(cfg);
limiter.tryAcquire();  // → true / false

// Verteilt via Redis (cluster-weit):
cfg.backend       = TokenBucketRateLimiter::Backend::REDIS;
cfg.redis.host    = "redis.internal";
cfg.redis.port    = 6379;
cfg.bucket_id     = "api:v1";
TokenBucketRateLimiter redis_limiter(cfg);
// Automatischer Fallback auf lokales Bucket bei Redis-Ausfall
bool healthy = redis_limiter.isRedisHealthy();
```

### Apache Ranger & OPA

```cpp
// ranger_adapter.cpp + opa_adapter.cpp
class RangerAdapter {
    bool authorize(user, resource, action);
    std::vector<Policy> getPolicies(resource);
};
```

---

## Performance

| Metrik | Zielwert | Status |
|--------|----------|--------|
| Durchsatz | 50.000–200.000 req/s | ✅ Dokumentiert in ROADMAP |
| p50 Latenz | < 5 ms | ✅ Dokumentiert |
| p99 Latenz | < 50 ms | ✅ Dokumentiert |
| TLS 1.3 Handshake | ≤ 2 ms | ✅ Dokumentiert |
| Graceful Shutdown | ≤ 30 s | ✅ Implementiert |

Benchmarks: [`benchmarks/bench_api_endpoints.cpp`](../../../benchmarks/bench_api_endpoints.cpp) (14 Mikro-Benchmarks)

---

## Geplante Erweiterungen

| Feature | Target | Status |
|---------|--------|--------|
| OAuth2/OIDC (Authorization Code + PKCE) | v1.6.0 | ⏳ Geplant (kein Code vorhanden) |
| Verteiltes Rate Limiting via Redis | v1.6.0 | ✅ Implementiert (`Backend::REDIS` in `rate_limiter_v2.h`, EVALSHA-Lua-Skript, lokaler Fallback) |
| Verteiltes API Gateway (Raft) | v1.7.0 | ⏳ Geplant (kein Code vorhanden) |
| gRPC-Web TypeScript Client-Generierung | v1.7.0 | ⏳ Geplant |
| SAML 2.0 SP-Support | v1.7.0 | ✅ Implementiert (`saml_auth_provider.cpp`, 37 Unit-Tests) |
| WebAssembly API Handler (WASI) | v1.8.0 | ⏳ Geplant (kein Code vorhanden) |

Details: [`src/server/FUTURE_ENHANCEMENTS.md`](../../../src/server/FUTURE_ENHANCEMENTS.md)

---

## Fehlende Implementierungen

Vollständiger Report: [`missing-implementations.md`](./missing-implementations.md)

Zusammenfassung der wichtigsten Befunde:

| # | Claim | Beobachtung | Schwere |
|---|-------|-------------|---------|
| 1 | `rate_limiter_v2` Redis-Backend (ROADMAP § Distributed Rate Limiting) | `rate_limiter_v2.cpp` vorhanden, aber kein `Backend::REDIS`; nur lokales Token-Bucket | Mittel |
| 2 | OAuth2/OIDC-Provider (`server/oauth2_provider.cpp`) | Keine Datei vorhanden | Hoch |
| 3 | SAML 2.0 SP (`server/saml_auth_provider.cpp`) | ✅ Implementiert (v1.7.0): `saml_auth_provider.cpp`, `saml_auth_provider.h`, 37 Unit-Tests | Gelöst |
| 4 | Distributed API Gateway (`server/distributed_gateway.cpp`) | Keine Datei vorhanden | Niedrig |
| 5 | WebAssembly Handler Registry (`server/wasm_handler_registry.cpp`) | ✅ Implementiert (v2.1.0) | Niedrig |
| 6 | PostgreSQL Wire-Protokoll: erweiterte PG-Features | `postgres_session.cpp` vorhanden (1929 LOC), aber ROADMAP warnt: „partial compatibility" | Info |

---

## Verwandte Dokumentation

### Primary (Entwickler-Docs)
- [`src/server/README.md`](../../../src/server/README.md) — vollständige Komponentenbeschreibung (1344 Zeilen)
- [`include/server/README.md`](../../../include/server/README.md) — Header-Interface-Dokumentation (819 Zeilen)
- [`src/server/ARCHITECTURE.md`](../../../src/server/ARCHITECTURE.md) — Architekturleitfaden
- [`src/server/ROADMAP.md`](../../../src/server/ROADMAP.md) — Feature-Roadmap und Phasenstatus
- [`src/server/FUTURE_ENHANCEMENTS.md`](../../../src/server/FUTURE_ENHANCEMENTS.md) — Geplante Erweiterungen mit Referenzen
- [`src/server/rpc/README.md`](../../../src/server/rpc/README.md) — RPC-Transfer-Handler

### Dieses Modul (Secondary/German docs)
- [inventory.md](./inventory.md) — Primär-Inventar aller Dateien
- [missing-implementations.md](./missing-implementations.md) — Reality-Check Befunde (6 Findings)
- [missing-implementations.json](./missing-implementations.json) — Maschinenlesbar

### Secondary (Nutzerdocs)
- [Deployment Guide](../guides/guides_deployment.md)
- [TLS Setup](../guides/guides_tls_setup.md)
- [RBAC / Authorization](../guides/guides_rbac.md)
- [Enterprise Features](../enterprise/README.md)
- [APIs: OpenAPI](../apis/apis_openapi.md)
- [APIs: GraphQL](../apis/apis_graphql.md)
