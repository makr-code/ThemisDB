[docs](../../index.md) > [de](../index.md) > [api](./index.md)

**Stand:** 2026-03-22  
**Version:** 1.8.0  
**Kategorie:** API  
**Validated:** 2026-03-22  
**Status:** current

---

# API Module

Das API-Modul stellt die multi-protokoll-Schnittstelle von ThemisDB bereit: REST/HTTP,
gRPC, WebSocket und GraphQL. Es ist bewusst dünn gehalten — es übersetzt Wire-Protokolle
in interne Request-Objekte und delegiert alle Business-Logik an die `server`-Modul-Schicht.

**Primäre Dokumentation:** [`src/api/README.md`](../../../src/api/README.md)  
**Architektur:** [`src/api/ARCHITECTURE.md`](../../../src/api/ARCHITECTURE.md)  
**Roadmap:** [`src/api/ROADMAP.md`](../../../src/api/ROADMAP.md)  
**Changelog:** [`src/api/CHANGELOG.md`](../../../src/api/CHANGELOG.md)  
**Geplante Erweiterungen (src):** [`src/api/FUTURE_ENHANCEMENTS.md`](../../../src/api/FUTURE_ENHANCEMENTS.md)  
**Geplante Erweiterungen (include):** [`include/api/FUTURE_ENHANCEMENTS.md`](../../../include/api/FUTURE_ENHANCEMENTS.md)  
**Public Headers:** [`include/api/README.md`](../../../include/api/README.md)  
**Security:** [`src/api/SECURITY.md`](../../../src/api/SECURITY.md)  
**Audit:** [`src/api/AUDIT.md`](../../../src/api/AUDIT.md)

---

## Übersicht

| Protokoll | Einstiegspunkt | Implementierungsdatei | Status |
|-----------|---------------|----------------------|--------|
| HTTP/REST | `src/server/http_server.cpp` (11.000+ LOC) | *(nicht in `src/api/`)* | ✅ Production Ready |
| GraphQL | `src/api/graphql.cpp` | Schema, Parser, Executor, Subscription | ✅ Production Ready |
| GraphQL über WebSocket | `src/api/graphql_ws_handler.cpp` | `graphql-transport-ws`-Protokoll | ✅ Production Ready |
| WebSocket CDC-Streaming | `src/api/ws_handler.cpp` | `/v2/changes`, `/v2/cdc/stream` | ✅ Production Ready |
| gRPC | `src/api/grpc_server.cpp` | `GrpcApiServer`, TLS, Service-Registrierung | ✅ Production Ready (Stubs ausstehend) |
| gRPC Service | `src/api/themisdb_grpc_service.cpp` | `ThemisDBService` proto-Bridge | ⚠️ Stubs für ExecuteAQL, StreamAQL, VectorSearch |
| Request Tracing | `src/api/tracing_middleware.cpp` | `X-Correlation-ID`, OTLP-Export | ✅ Production Ready |
| OTLP-Exporter | `src/api/otlp_exporter.cpp` | Async-Queue, libcurl POST, Retry | ✅ Production Ready |
| Geo-Index-Hooks | `src/api/geo_index_hooks.cpp` | GeoJSON-Validierung, Storage-Hooks | ✅ Production Ready |

---

## Source-Code Referenz

### `src/api/` — Implementierungsdateien

| Datei | LOC | Zweck |
|-------|-----|-------|
| `graphql.cpp` | ~1.523 | GraphQL-Parser, Executor, Schema-Resolver, Query-Plan-Cache |
| `graphql_ws_handler.cpp` | ~504 | GraphQL-WebSocket-Subscriptions, CDC-Callback-Safety (`alive_` Flag) |
| `grpc_server.cpp` | ~240 | gRPC-Server-Lifecycle, TLS-Credentials, Completion-Queue |
| `themisdb_grpc_service.cpp` | ~464 | `ThemisDBService` gRPC-Handler (CRUD, Streaming-AQL, Vektor-Suche — teils Stubs) |
| `ws_handler.cpp` | ~187 | WebSocket-Upgrade, CDC-Frame-Dispatcher, Back-Pressure (1.000 Queue-Limit) |
| `tracing_middleware.cpp` | ~142 | `X-Correlation-ID`-Propagation, Thread-local Request-Context, OTLP-Span-Export |
| `otlp_exporter.cpp` | ~412 | OpenTelemetry OTLP/HTTP Async-Queue-Exporter, Background-Flush, Retry (3×) |
| `geo_index_hooks.cpp` | ~575 | GeoJSON-Validierung, idempotente Hook-Registrierung für Hot-Reload |
| `http_server.cpp` | 31 | ❌ **Deprecated Stub** — nicht kompiliert; echte Impl. in `src/server/http_server.cpp` |

### `include/api/` — Public Header-Dateien

| Header | Zweck |
|--------|-------|
| `graphql.h` | `Value`, `Field`, `SelectionSet`, `Parser`, `Executor`, `QueryLimits`, `Schema` |
| `graphql_cache.h` | `QueryPlanCache`, `ResponseCache` mit LRU-Eviction |
| `graphql_metrics.h` | `Metrics`, `QueryTimer` RAII-Guard für Latenz-Tracking |
| `graphql_ws_handler.h` | `GraphQLWsHandler`, Subscription-Cap, CDC-Event-Source |
| `graphql_schema_builder.h` | `IGraphQLSchemaBuilder`, `GraphQLTypeDescriptor`, `SchemaValidationResult` |
| `rate_limiter.h` | `RateLimiter` (Token-Bucket), `OperationRateLimiter` per-Tenant/Client |
| `audit_logger.h` | `AuditLogger` (Circular-Buffer), `AuditLogEntry`, `AuditLogBuilder` |
| `persisted_queries.h` | `PersistedQueryRegistry`, `QueryAllowList`, `QueryHasher` (SHA256) |
| `grpc_server.h` | `GrpcApiServer`, `GrpcServerConfig` |
| `themisdb_grpc_service.h` | `ThemisDBService` gRPC-Handler-Klasse |
| `websocket_handler.h` | `IWebSocketHandler`, `WebSocketSession`, `IWebSocketFrameCallback`, RFC-6455-CloseCode |
| `ws_handler.h` | `WsChangeHandler` Upgrade-Validator, Frame-Dispatcher |
| `grpc_bridge.h` | `IGRPCBridge` (pure-virtual), `ServiceDescriptor`, `GRPCRequest`, `GRPCMetadata` |
| `api_version_router.h` | `IAPIVersionRouter` (pure-virtual), `VersionDescriptor`, `/v1/`/`/v2/`-Routing |
| `correlation_id.h` | `CorrelationId` (16-Byte-UUID), `ICorrelationIDProvider` |
| `tracing_middleware.h` | `TracingMiddleware`, Request-Context-Storage, OTLP-Integration |
| `otlp_exporter.h` | `OtlpExporter`, `OtlpExporterConfig`, `SpanData` |
| `geo_index_hooks.h` | `GeoIndexHooks`, statische Hook-Registrierung |
| `http_handler.h` | `IHttpHandler` (pure-virtual), `HttpRequest`, `HttpResponse`, `Result<T>` |

---

## Integrationspunkte

| Modul | Verbindung |
|-------|-----------|
| `src/auth/` | JWT-Validierung via `jwt_validator.cpp` in **allen** Transports (WebSocket, gRPC, HTTP) |
| `src/server/` | Business-Logik und REST-Routing; `http_server.cpp` registriert `RouteVersionRouter` |
| `src/query/` | AQL-Ausführung wird an `aql_runner.cpp` delegiert (noch ausstehend für gRPC-Stubs) |
| `src/cdc/` | CDC Changefeed für WebSocket-Streaming und GraphQL-Subscriptions |
| `src/index/` | Geo-Index-Hooks in `geo_index_hooks.cpp` schreiben in Storage-Layer |

---

## Bekannte Einschränkungen

- gRPC-RPC-Stubs (`ExecuteAQL`, `StreamAQL`, `VectorSearch`, `FilteredVectorSearch`, `HybridSearch`, `FullTextSearch`) geben aktuell `UNIMPLEMENTED` zurück; `ThemisDBGrpcServiceFactory`-Injektion steht aus.
- `GrpcApiServer::start()` hält `mutex_` während des blockierenden `BuildAndStart()`-Aufrufs (Target: v2.1.0).
- `GrpcApiServer::stop()` ruft `server_->Shutdown()` ohne Deadline auf — kann unbegrenzt blockieren (Target: v2.1.0).
- GraphQL `Parser` unterstützt noch keine Fragments, Directives oder Inline-Fragments (dokumentiert in `graphql.h`).
- `WsChangeHandler::validate()` dekodiert Query-String-Parameter (`from_sequence`, `key_prefix`) nicht URL-konform.

Vollständige Liste: [`docs/de/api/MISSING_IMPLEMENTATIONS.md`](./MISSING_IMPLEMENTATIONS.md)

---

## Tests

50+ Test-Dateien für API-bezogene Funktionalität, darunter:
- `tests/test_api_grpc_server.cpp` — gRPC-Server-Lifecycle (mit `THEMIS_ENABLE_GRPC`)
- `tests/test_api_gateway_enhancements.cpp` — API-Gateway-Integration
- `tests/test_api_interfaces.cpp` — Public-Header-Interfaces
- `tests/test_api_routing.cpp` — Request-Routing
- `tests/test_versioned_api_routing.cpp` — Versioned-API-Routing
- `tests/test_otel_api_tracing.cpp` — OTLP-Tracing

CI-Workflows: `graphql-ws-handler-cdc-callback-safety-ci.yml`, `api-gateway-enhancements-ci.yml`, `request-tracing-correlation-ids-ci.yml`

---

*Primäre Quelldateien: [`docs/de/api/PRIMARY_SOURCES.md`](./PRIMARY_SOURCES.md)*
