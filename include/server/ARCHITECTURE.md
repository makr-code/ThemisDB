> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/server/ARCHITECTURE.md -->

# Server Module — Public Header Architecture

**Module Path:** `include/server/`
**Implementation:** `../../src/server/`
**Canonical architecture doc:** [`../../src/server/ARCHITECTURE.md`](../../src/server/ARCHITECTURE.md)

---

## 1. Overview

`include/server/` defines the **public C++ contract** for ThemisDB's multi-protocol server layer. With 124 headers, this is the largest public surface in `include/`. Headers cover the HTTP/2+HTTP/3/gRPC/WebSocket/MQTT/Postgres server, all REST API handlers, middleware, rate limiting, policy enforcement, session management, and server-side monitoring.

For full pipeline details — request routing, middleware chain, gRPC reflection, HTTP/3 QUIC, WebSocket back-pressure — see:
→ [`../../src/server/ARCHITECTURE.md`](../../src/server/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Core Server

| Header | Public Type | Purpose |
|--------|------------|---------|
| `http_server.h` | `HttpServer` | Root HTTP/2 + HTTP/3 server lifecycle |
| `http2_session.h` | `HTTP2Session` | HTTP/2 per-connection session |
| `http3_session.h` | `HTTP3Session` | HTTP/3/QUIC per-connection session |
| `http3_production_config.h` | `HTTP3ProductionConfig` | QUIC production tuning parameters |
| `http3_datagram.h` | `HTTP3Datagram` | QUIC datagram framing |
| `websocket_session.h` | `WebSocketSession` | WebSocket session (`active_` is `atomic<bool>`) |
| `postgres_session.h` | `PostgresSession` | Postgres wire protocol session |
| `rpc_service_impl.h` | `RPCServiceImpl` | gRPC/RPC service multiplexer |
| `mcp_server.h` | `MCPServer` | Model Context Protocol server |

### 2.2 API Handlers (REST)

| Header | Public Type | Area |
|--------|------------|------|
| `query_api_handler.h` | `QueryApiHandler` | AQL / SQL query execution |
| `entity_api_handler.h` | `EntityApiHandler` | CRUD for entities |
| `graph_api_handler.h` | `GraphApiHandler` | Graph traversal and management |
| `vector_api_handler.h` | `VectorApiHandler` | Vector search and management |
| `index_api_handler.h` | `IndexApiHandler` | Index lifecycle |
| `schema_api_handler.h` | `SchemaApiHandler` | Schema DDL |
| `transaction_api_handler.h` | `TransactionApiHandler` | Local transaction REST API |
| `distributed_txn_api_handler.h` | `DistributedTxnApiHandler` | Distributed transaction REST API |
| `saga_api_handler.h` | `SagaApiHandler` | Saga lifecycle API |
| `cache_api_handler.h` | `CacheApiHandler` | Cache management |
| `cache_admin_api_handler.h` | `CacheAdminApiHandler` | Cache administration |
| `monitoring_api_handler.h` | `MonitoringApiHandler` | /stats / /metrics endpoints |
| `admin_api_handler.h` | `AdminApiHandler` | Admin operations |
| `audit_api_handler.h` | `AuditApiHandler` | Audit log API |
| `llm_api_handler.h` | `LLMApiHandler` | LLM inference REST API |
| `lora_api_handler.h` | `LoRAApiHandler` | LoRA adapter management |
| `voice_api_handler.h` | `VoiceApiHandler` | Voice/speech API |
| `prompt_api_handler.h` | `PromptApiHandler` | Prompt management |
| `prompt_engineering_api_handler.h` | `PromptEngineeringApiHandler` | Prompt engineering API |
| `feedback_api_handler.h` | `FeedbackApiHandler` | ML feedback collection |
| `geo_topology_api_handler.h` | `GeoTopologyApiHandler` | Geo/topology management |
| `spatial_api_handler.h` | `SpatialApiHandler` | Geospatial queries |
| `timeseries_api_handler.h` | `TimeseriesApiHandler` | Timeseries data API |
| `changefeed_api_handler.h` | `ChangefeedApiHandler` | CDC / changefeed streaming |
| `continuous_query_api_handler.h` | `ContinuousQueryApiHandler` | Continuous query API |
| `replication_topology_api_handler.h` | `ReplicationTopologyApiHandler` | Replication topology management |
| `snapshot_api_handler.h` | `SnapshotApiHandler` | Snapshot lifecycle |
| `pitr_api_handler.h` | `PITRApiHandler` | Point-in-time recovery API |
| `maintenance_api_handler.h` | `MaintenanceApiHandler` | Maintenance operations |
| `wal_api_handler.h` | `WALApiHandler` | WAL inspection and management |
| `branch_api_handler.h` | `BranchApiHandler` | Named branch API |
| `merge_api_handler.h` | `MergeApiHandler` | Branch merge API |
| `diff_api_handler.h` | `DiffApiHandler` | Branch diff API |
| `shard_repair_api_handler.h` | `ShardRepairApiHandler` | Shard repair and rebalance |
| `sharding_metrics_handler.h` | `ShardingMetricsHandler` | Sharding metrics |
| `keys_api_handler.h` | `KeysApiHandler` | Key management |
| `pki_api_handler.h` | `PKIApiHandler` | PKI certificate management |
| `retention_api_handler.h` | `RetentionApiHandler` | Data retention policy |
| `buffer_api_handler.h` | `BufferApiHandler` | Write buffer management |
| `async_job_api_handler.h` | `AsyncJobApiHandler` | Async job tracking |
| `export_api_handler.h` | `ExportApiHandler` | Data export |
| `import_api_handler.h` | `ImportApiHandler` | Data import |
| `udf_api_handler.h` | `UDFApiHandler` | User-defined function management |
| `classification_api_handler.h` | `ClassificationApiHandler` | ML classification API |
| `ethics_api_handler.h` | `EthicsApiHandler` | AI ethics API |
| `pii_api_handler.h` | `PIIApiHandler` | PII detection and masking |
| `content_api_handler.h` | `ContentApiHandler` | Content management |
| `reports_api_handler.h` | `ReportsApiHandler` | Reporting API |
| `profiling_api_handler.h` | `ProfilingApiHandler` | Runtime profiling API |
| `hot_reload_api_handler.h` | `HotReloadApiHandler` | Config hot-reload API |
| `session_api_handler.h` | `SessionApiHandler` | Session management |
| `rope_api_handler.h` | `RopeApiHandler` | Rope data structure API |
| `bpmn_api_handler.h` | `BPMNApiHandler` | BPMN workflow API |
| `error_api_handler.h` | `ErrorApiHandler` | Structured error responses |
| `service_mesh_api_handler.h` | `ServiceMeshApiHandler` | Service mesh management |
| `task_scheduler_api_handler.h` | `TaskSchedulerApiHandler` | Task scheduler API |
| `serverless_function_api_handler.h` | `ServerlessFunctionApiHandler` | Serverless function invocation |
| `review_scheduling_api_handler.h` | `ReviewSchedulingApiHandler` | Review scheduling |
| `compliance_reporting_api_handler.h` | `ComplianceReportingApiHandler` | Compliance report generation |
| `graphql_api_handler.h` | `GraphQLApiHandler` | GraphQL endpoint |

### 2.3 gRPC Services

| Header | Public Type | Purpose |
|--------|------------|---------|
| `llm_grpc_service.h` | `LLMGrpcService` | LLM inference gRPC service |
| `pitr_grpc_service.h` | `PITRGrpcService` | PITR gRPC service |
| `wal_grpc_service.h` | `WALGrpcService` | WAL gRPC service |
| `prompt_engineering_grpc_service.h` | `PromptEngineeringGrpcService` | Prompt engineering gRPC |
| `themis_core_grpc_service.h` | `ThemisCoreGrpcService` | Core gRPC multiplexer |
| `grpc_web_proxy_handler.h` | `GRPCWebProxyHandler` | gRPC-Web proxy |

### 2.4 Middleware

| Header | Public Type | Purpose |
|--------|------------|---------|
| `auth_middleware.h` | `AuthMiddleware` | JWT/API-key authentication |
| `auth_scope_mapper.h` | `AuthScopeMapper` | Scope → permission mapping |
| `rate_limiter.h` / `rate_limiter_v2.h` | `RateLimiter`, `RateLimiterV2` | Token-bucket rate limiting |
| `adaptive_rate_limiter.h` | `AdaptiveRateLimiter` | Workload-adaptive rate limiting |
| `cost_based_rate_limiter.h` | `CostBasedRateLimiter` | Query-cost-based rate limiting |
| `rate_limiting_middleware.h` | `RateLimitingMiddleware` | Middleware wrapper |
| `load_shedder.h` | `LoadShedder` | Request shedding under overload |
| `request_coalescing.h` | `RequestCoalescing` | Duplicate-request coalescing |
| `request_validation_middleware.h` | `RequestValidationMiddleware` | Schema-level request validation |
| `response_transformer.h` | `ResponseTransformer` | Response format transformation |
| `cdn_cache_middleware.h` | `CDNCacheMiddleware` | CDN cache header injection |
| `chunked_response_writer.h` | `ChunkedResponseWriter` | HTTP chunked transfer encoding |

### 2.5 Policy and Governance

| Header | Public Type | Purpose |
|--------|------------|---------|
| `policy_engine.h` | `PolicyEngine` | OPA-backed policy evaluation |
| `policy_manager_api_handler.h` | `PolicyManagerApiHandler` | Policy CRUD |
| `policy_template_api_handler.h` | `PolicyTemplateApiHandler` | Policy template management |
| `policy_validation_api_handler.h` | `PolicyValidationApiHandler` | Policy dry-run |
| `policy_versioning_api_handler.h` | `PolicyVersioningApiHandler` | Policy version history |
| `opa_adapter.h` | `OPAAdapter` | OPA HTTP client adapter |
| `ranger_adapter.h` | `RangerAdapter` | Apache Ranger adapter |

### 2.6 Routing and Gateway

| Header | Public Type | Purpose |
|--------|------------|---------|
| `api_gateway.h` | `APIGateway` | Multi-protocol gateway router |
| `distributed_gateway.h` | `DistributedGateway` | Multi-shard/multi-region gateway |
| `route_version_router.h` | `RouteVersionRouter` | API version-based routing |
| `api_version.h` / `api_version_config.h` | `APIVersion`, `APIVersionConfig` | API versioning types |
| `openapi_route_registry.h` | `OpenAPIRouteRegistry` | OpenAPI route registration |
| `smart_routing.h` | `SmartRouting` | Latency/cost-aware routing |

### 2.7 Authentication and Security

| Header | Public Type | Purpose |
|--------|------------|---------|
| `api_auth_config.h` | `APIAuthConfig` | Auth configuration struct |
| `api_key_mgmt_handler.h` | `APIKeyMgmtHandler` | API key lifecycle |
| `api_security_audit.h` | `APISecurityAudit` | Security audit integration |
| `saml_auth_provider.h` | `SAMLAuthProvider` | SAML 2.0 auth provider |
| `tenant_manager.h` | `TenantManager` | Multi-tenancy management |

### 2.8 Auxiliary

| Header | Public Type | Purpose |
|--------|------------|---------|
| `sse_connection_manager.h` | `SSEConnectionManager` | Server-Sent Events connection pool |
| `mqtt_client_service.h` | `MQTTClientService` | MQTT broker client |
| `buffer_binary_protocol.h` | `BufferBinaryProtocol` | Binary framing for buffer API |
| `import_wizard_builder.h` | `ImportWizardBuilder` | Guided import builder |
| `http_type_adapter.h` | `HTTPTypeAdapter` | HTTP ↔ internal type conversion |
| `workload_fingerprint_engine.h` | `WorkloadFingerprintEngine` | Workload classification |
| `server_activation_profile.h` | `ServerActivationProfile` | Feature-flag activation profile |
| `wasm_handler_registry.h` | `WASMHandlerRegistry` | WASM-based handler plugins |
| `health_error_service.h` | `HealthErrorService` | Structured health error responses |

---

## 3. Namespace Layout

| Namespace | Scope |
|-----------|-------|
| `themis::server` | Core server, session, and gateway types |
| `themis::server::api` | REST API handler types |
| `themis::server::grpc` | gRPC service types |
| `themis::server::middleware` | Middleware pipeline types |
| `themis::server::policy` | Policy engine and adapter types |
