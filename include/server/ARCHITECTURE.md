<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/server/ -->

# Server Module — Public Header Architecture

**Version:** 1.0
**Last Updated:** 2026-04-06
**Header Path:** `include/server/`
**Implementation:** `../../src/server/`

---

## Overview

The Server module exposes 116 public headers covering the full request-handling stack of
ThemisDB: HTTP/2 and HTTP/3 sessions, WebSocket and MQTT sessions, gRPC and gRPC-Web services,
an API gateway, authentication middleware, rate limiting, smart routing, tenant management, and
~80 domain-specific API handlers. All production logic resides in `../../src/server/`.

---

## Design Principles

1. **Protocol Agnosticism** — every domain handler is decoupled from transport; session headers
   (`http2_session.h`, `http3_session.h`, `websocket_session.h`, `mqtt_session.h`,
   `postgres_session.h`) own protocol framing, handlers own business logic.
2. **Middleware Chain** — `auth_middleware.h`, `rate_limiting_middleware.h`,
   `request_validation_middleware.h`, and `cdn_cache_middleware.h` compose into a typed
   pipeline; each middleware is independently testable.
3. **Gateway-First Routing** — `api_gateway.h` and `distributed_gateway.h` are the single
   authoritative entry points; `smart_routing.h` and `route_version_router.h` handle version
   multiplexing.
4. **Multi-Tenant Isolation** — `tenant_manager.h` enforces tenant boundaries at the gateway
   layer before handlers are invoked.
5. **Observability Built-In** — `monitoring_api_handler.h`, `sharding_metrics_handler.h`, and
   `health_error_service.h` expose metrics and health endpoints as first-class handlers.
6. **gRPC-Web TypeScript Support** — `grpc_web_proxy_handler.h` bridges browser clients;
   TypeScript stubs generated via `scripts/gen_grpc_web_ts.py` (`@themisdb/client-grpc-web`
   v1.7.0).

---

## Subsystem Map

### Core Server Infrastructure

| Header | Classes/Interfaces | Purpose |
|--------|-------------------|---------|
| `http_server.h` | `HttpServer`, `ServerConfig` | Main HTTP/1.1 + HTTP/2 listener |
| `http2_session.h` | `Http2Session` | HTTP/2 stream multiplexing |
| `http3_session.h` | `Http3Session` | HTTP/3 (QUIC) session management |
| `http3_datagram.h` | `Http3Datagram` | HTTP/3 unreliable datagram support |
| `http3_production_config.h` | `Http3ProductionConfig` | QUIC/HTTP3 production tuning |
| `websocket_session.h` | `WebSocketSession` | WebSocket upgrade and frame handling |
| `mqtt_session.h` | `MqttSession` | MQTT 3.1/5.0 session broker bridge |
| `postgres_session.h` | `PostgresSession` | PostgreSQL wire-protocol session adapter |
| `mcp_server.h` | `McpServer` | Model Context Protocol server |
| `rpc_service_impl.h` | `RpcServiceImpl` | Generic gRPC service base implementation |
| `grpc_web_proxy_handler.h` | `GrpcWebProxyHandler` | gRPC-Web browser proxy (TypeScript client: `@themisdb/client-grpc-web` v1.7.0) |

### API Gateway & Routing

| Header | Classes/Interfaces | Purpose |
|--------|-------------------|---------|
| `api_gateway.h` | `ApiGateway`, `GatewayConfig` | Central request dispatch |
| `distributed_gateway.h` | `DistributedGateway` | Multi-node gateway with consensus routing |
| `smart_routing.h` | `SmartRouter`, `RoutingPolicy` | ML-assisted adaptive routing |
| `route_version_router.h` | `RouteVersionRouter` | API version multiplexer |
| `openapi_route_registry.h` | `OpenApiRouteRegistry` | OpenAPI 3.x route registration |
| `api_version.h` | `ApiVersion` | Version type and comparison helpers |
| `api_version_config.h` | `ApiVersionConfig` | Version negotiation configuration |

### Authentication & Authorization

| Header | Classes/Interfaces | Purpose |
|--------|-------------------|---------|
| `auth_middleware.h` | `AuthMiddleware`, `AuthContext` | JWT/mTLS authentication middleware |
| `auth_scope_mapper.h` | `AuthScopeMapper` | OAuth2 scope-to-permission mapping |
| `oauth2_provider.h` | `OAuth2Provider` | OAuth2 authorization server adapter |
| `saml_auth_provider.h` | `SamlAuthProvider` | SAML 2.0 SSO provider |
| `api_auth_config.h` | `ApiAuthConfig` | Per-route authentication configuration |
| `api_key_mgmt_handler.h` | `ApiKeyMgmtHandler` | API key lifecycle management |
| `opa_adapter.h` | `OpaAdapter` | Open Policy Agent authorization adapter |
| `ranger_adapter.h` | `RangerAdapter` | Apache Ranger policy adapter |
| `policy_engine.h` | `PolicyEngine` | In-process policy evaluation engine |

### Rate Limiting & Traffic Management

| Header | Classes/Interfaces | Purpose |
|--------|-------------------|---------|
| `rate_limiter.h` | `IRateLimiter`, `RateLimitResult` | Core rate limiter interface |
| `rate_limiter_v2.h` | `RateLimiterV2` | Distributed token-bucket rate limiter |
| `adaptive_rate_limiter.h` | `AdaptiveRateLimiter` | Load-aware adaptive rate limiting |
| `cost_based_rate_limiter.h` | `CostBasedRateLimiter` | Query-cost-aware rate limiter |
| `rate_limiting_middleware.h` | `RateLimitingMiddleware` | Middleware wrapper for rate limiters |
| `load_shedder.h` | `LoadShedder`, `ShedPolicy` | Overload protection via load shedding |
| `request_coalescing.h` | `RequestCoalescing` | Identical-request deduplication |
| `request_validation_middleware.h` | `RequestValidationMiddleware` | Schema-validated request guard |
| `response_transformer.h` | `ResponseTransformer` | Response post-processing pipeline |
| `chunked_response_writer.h` | `ChunkedResponseWriter` | Streaming chunked response output |
| `cdn_cache_middleware.h` | `CdnCacheMiddleware` | CDN cache-control header injection |
| `sse_connection_manager.h` | `SseConnectionManager` | Server-Sent Events connection registry |

### Tenant & Session Management

| Header | Classes/Interfaces | Purpose |
|--------|-------------------|---------|
| `tenant_manager.h` | `TenantManager`, `TenantConfig` | Multi-tenant lifecycle and isolation |
| `session_api_handler.h` | `SessionApiHandler` | User session CRUD handler |

### Domain API Handlers (selected)

| Header | Handler | Domain |
|--------|---------|--------|
| `query_api_handler.h` | `QueryApiHandler` | AQL query execution |
| `graph_api_handler.h` | `GraphApiHandler` | Graph traversal API |
| `vector_api_handler.h` | `VectorApiHandler` | Vector similarity search |
| `timeseries_api_handler.h` | `TimeseriesApiHandler` | Time-series data ingest/query |
| `spatial_api_handler.h` | `SpatialApiHandler` | Geospatial query API |
| `transaction_api_handler.h` | `TransactionApiHandler` | ACID transaction control |
| `mvcc_api_handler.h` | `MvccApiHandler` | MVCC snapshot management |
| `distributed_txn_api_handler.h` | `DistributedTxnApiHandler` | Distributed 2PC transactions |
| `saga_api_handler.h` | `SagaApiHandler` | Saga pattern orchestration |
| `schema_api_handler.h` | `SchemaApiHandler` | Schema definition and migration |
| `index_api_handler.h` | `IndexApiHandler` | Index management |
| `entity_api_handler.h` | `EntityApiHandler` | Document/entity CRUD |
| `llm_api_handler.h` | `LlmApiHandler` | LLM inference API |
| `lora_api_handler.h` | `LoraApiHandler` | LoRA adapter management |
| `prompt_api_handler.h` | `PromptApiHandler` | Prompt template management |
| `prompt_engineering_api_handler.h` | `PromptEngineeringApiHandler` | Prompt engineering workflows |
| `voice_api_handler.h` | `VoiceApiHandler` | Voice/speech processing API |
| `graphql_api_handler.h` | `GraphqlApiHandler` | GraphQL endpoint handler |
| `pitr_api_handler.h` | `PitrApiHandler` | Point-in-time recovery |
| `wal_api_handler.h` | `WalApiHandler` | WAL management API |
| `replication_topology_api_handler.h` | `ReplicationTopologyApiHandler` | Replication topology control |
| `snapshot_api_handler.h` | `SnapshotApiHandler` | Database snapshot management |
| `compliance_reporting_api_handler.h` | `ComplianceReportingApiHandler` | Regulatory compliance reports |
| `pii_api_handler.h` | `PiiApiHandler` | PII detection and redaction |
| `audit_api_handler.h` | `AuditApiHandler` | Audit log query API |
| `monitoring_api_handler.h` | `MonitoringApiHandler` | Metrics and observability |
| `admin_api_handler.h` | `AdminApiHandler` | Administrative operations |
| `cache_api_handler.h` | `CacheApiHandler` | Cache management |
| `import_api_handler.h` | `ImportApiHandler` | Bulk data import |
| `export_api_handler.h` | `ExportApiHandler` | Data export |
| `wasm_handler_registry.h` | `WasmHandlerRegistry` | WASM extension handler registry |
| `serverless_function_api_handler.h` | `ServerlessFunctionApiHandler` | Serverless function invocation |
| `udf_api_handler.h` | `UdfApiHandler` | User-defined function management |
| *(planned)* `workload_fingerprint_engine.h` | `WorkloadFingerprintEngine`, `WorkloadFingerprint` | Layer 8: per-tenant workload fingerprinting for cross-shard comparison (IMPL-B8) |

### gRPC Services

| Header | Service | Purpose |
|--------|---------|---------|
| `themis_core_grpc_service.h` | `ThemisCoreGrpcService` | Core DB operations over gRPC |
| `llm_grpc_service.h` | `LlmGrpcService` | LLM inference over gRPC |
| `pitr_grpc_service.h` | `PitrGrpcService` | PITR over gRPC |
| `wal_grpc_service.h` | `WalGrpcService` | WAL streaming over gRPC |
| `prompt_engineering_grpc_service.h` | `PromptEngineeringGrpcService` | Prompt workflows over gRPC |

---

> Implementation details in `../../src/server/`. No business logic in `include/`.
