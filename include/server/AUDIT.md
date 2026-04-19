<!-- Status: current | validated: 2026-04-19 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Audit Report — Server Module (Public Headers)

**Last Audit:** 2026-04-19
**Auditor:** Copilot
**Status:** ✅ Pass

---

## Summary

| Metric | Result |
|--------|--------|
| Header Files | 119 (in `include/server/`) |
| Implementation | `../../src/server/` |
| Exported Symbols Verified | ✅ |
| Deprecated APIs | 1 (`rate_limiter.h` superseded by `rate_limiter_v2.h`) |
| Security Issues in Headers | None |

---

## Core Infrastructure Headers

| File | Exported Symbols | Notes |
|------|-----------------|-------|
| `http_server.h` | `HttpServer`, `ServerConfig` | Main HTTP listener |
| `http2_session.h` | `Http2Session` | RFC 7540 compliant |
| `http3_session.h` | `Http3Session` | QUIC-based; production-ready |
| `http3_datagram.h` | `Http3Datagram` | RFC 9221 unreliable datagrams |
| `http3_production_config.h` | `Http3ProductionConfig` | Production QUIC tuning |
| `websocket_session.h` | `WebSocketSession` | RFC 6455 frame handling |
| `mqtt_session.h` | `MqttSession` | MQTT 3.1/5.0 bridge |
| `postgres_session.h` | `PostgresSession` | PostgreSQL wire protocol |
| `mcp_server.h` | `McpServer` | Model Context Protocol |
| `rpc_service_impl.h` | `RpcServiceImpl` | gRPC base class |
| `grpc_web_proxy_handler.h` | `GrpcWebProxyHandler` | Browser gRPC-Web proxy |
| `buffer_binary_protocol.h` | `BufferBinaryProtocol` | Binary framing for buffer API |
| `http_type_adapter.h` | `HttpTypeAdapter` | HTTP ↔ internal type conversion |
| `chunked_response_writer.h` | `ChunkedResponseWriter` | Streaming chunked output |
| `sse_connection_manager.h` | `SseConnectionManager` | SSE connection registry |

## API Gateway & Routing Headers

| File | Exported Symbols | Notes |
|------|-----------------|-------|
| `api_gateway.h` | `ApiGateway`, `GatewayConfig` | Central dispatch |
| `distributed_gateway.h` | `DistributedGateway` | Multi-node routing |
| `smart_routing.h` | `SmartRouter`, `RoutingPolicy` | ML-assisted routing |
| `route_version_router.h` | `RouteVersionRouter` | Version multiplexer |
| `openapi_route_registry.h` | `OpenApiRouteRegistry` | OpenAPI 3.x registry |
| `api_version.h` | `ApiVersion` | Version type |
| `api_version_config.h` | `ApiVersionConfig` | Version negotiation |

## Auth & Policy Headers

| File | Exported Symbols | Notes |
|------|-----------------|-------|
| `auth_middleware.h` | `AuthMiddleware`, `AuthContext` | JWT/mTLS |
| `auth_scope_mapper.h` | `AuthScopeMapper` | Scope mapping |
| `oauth2_provider.h` | `OAuth2Provider` | OAuth2 adapter |
| `saml_auth_provider.h` | `SamlAuthProvider` | SAML 2.0 |
| `api_auth_config.h` | `ApiAuthConfig` | Per-route auth config |
| `api_key_mgmt_handler.h` | `ApiKeyMgmtHandler` | API key lifecycle |
| `opa_adapter.h` | `OpaAdapter` | OPA authorization |
| `ranger_adapter.h` | `RangerAdapter` | Ranger policy |
| `policy_engine.h` | `PolicyEngine` | In-process evaluation |
| `policy_api_handler.h` | `PolicyApiHandler` | Policy CRUD |
| `policy_manager_api_handler.h` | `PolicyManagerApiHandler` | Policy manager |
| `policy_template_api_handler.h` | `PolicyTemplateApiHandler` | Policy templates |
| `policy_validation_api_handler.h` | `PolicyValidationApiHandler` | Policy validation |
| `policy_versioning_api_handler.h` | `PolicyVersioningApiHandler` | Policy versioning |

## Rate Limiting Headers

| File | Exported Symbols | Notes |
|------|-----------------|-------|
| `rate_limiter.h` | `IRateLimiter`, `RateLimitResult` | ⚠️ Superseded by v2; kept for ABI compat |
| `rate_limiter_v2.h` | `RateLimiterV2` | Distributed token-bucket; preferred |
| `adaptive_rate_limiter.h` | `AdaptiveRateLimiter` | Load-aware |
| `cost_based_rate_limiter.h` | `CostBasedRateLimiter` | Query-cost-aware |
| `rate_limiting_middleware.h` | `RateLimitingMiddleware` | Middleware wrapper |
| `load_shedder.h` | `LoadShedder`, `ShedPolicy` | Overload protection |
| `request_coalescing.h` | `RequestCoalescing` | Request deduplication |
| `request_validation_middleware.h` | `RequestValidationMiddleware` | Schema validation |
| `response_transformer.h` | `ResponseTransformer` | Response post-processing |
| `cdn_cache_middleware.h` | `CdnCacheMiddleware` | CDN header injection |

## Domain API Handler Headers

| File | Exported Symbols | Notes |
|------|-----------------|-------|
| `query_api_handler.h` | `QueryApiHandler` | AQL query |
| `graph_api_handler.h` | `GraphApiHandler` | Graph traversal |
| `vector_api_handler.h` | `VectorApiHandler` | Vector search |
| `timeseries_api_handler.h` | `TimeseriesApiHandler` | Time-series |
| `spatial_api_handler.h` | `SpatialApiHandler` | Geospatial |
| `transaction_api_handler.h` | `TransactionApiHandler` | ACID transactions |
| `mvcc_api_handler.h` | `MvccApiHandler` | MVCC snapshots |
| `distributed_txn_api_handler.h` | `DistributedTxnApiHandler` | Distributed 2PC |
| `saga_api_handler.h` | `SagaApiHandler` | Saga orchestration |
| `schema_api_handler.h` | `SchemaApiHandler` | Schema DDL |
| `index_api_handler.h` | `IndexApiHandler` | Index management |
| `entity_api_handler.h` | `EntityApiHandler` | Document CRUD |
| `content_api_handler.h` | `ContentApiHandler` | Content management |
| `llm_api_handler.h` | `LlmApiHandler` | LLM inference |
| `lora_api_handler.h` | `LoraApiHandler` | LoRA adapters |
| `prompt_api_handler.h` | `PromptApiHandler` | Prompt templates |
| `prompt_engineering_api_handler.h` | `PromptEngineeringApiHandler` | Prompt workflows |
| `voice_api_handler.h` | `VoiceApiHandler` | Voice/speech |
| `graphql_api_handler.h` | `GraphqlApiHandler` | GraphQL |
| `pitr_api_handler.h` | `PitrApiHandler` | PITR |
| `wal_api_handler.h` | `WalApiHandler` | WAL management |
| `replication_topology_api_handler.h` | `ReplicationTopologyApiHandler` | Replication topology |
| `snapshot_api_handler.h` | `SnapshotApiHandler` | DB snapshots |
| `compliance_reporting_api_handler.h` | `ComplianceReportingApiHandler` | Compliance reports |
| `pii_api_handler.h` | `PiiApiHandler` | PII handling |
| `audit_api_handler.h` | `AuditApiHandler` | Audit log query |
| `monitoring_api_handler.h` | `MonitoringApiHandler` | Metrics |
| `admin_api_handler.h` | `AdminApiHandler` | Admin ops |
| `cache_api_handler.h` | `CacheApiHandler` | Cache management |
| `cache_admin_api_handler.h` | `CacheAdminApiHandler` | Cache administration |
| `import_api_handler.h` | `ImportApiHandler` | Bulk import |
| `export_api_handler.h` | `ExportApiHandler` | Data export |
| `import_wizard_builder.h` | `ImportWizardBuilder` | Import wizard DSL |
| `wasm_handler_registry.h` | `WasmHandlerRegistry` | WASM handlers |
| `serverless_function_api_handler.h` | `ServerlessFunctionApiHandler` | Serverless functions |
| `udf_api_handler.h` | `UdfApiHandler` | UDFs |
| `async_job_api_handler.h` | `AsyncJobApiHandler` | Async job queue |
| `task_scheduler_api_handler.h` | `TaskSchedulerApiHandler` | Task scheduling |
| `branch_api_handler.h` | `BranchApiHandler` | DB branching |
| `diff_api_handler.h` | `DiffApiHandler` | Branch diff |
| `merge_api_handler.h` | `MergeApiHandler` | Branch merge |
| `changefeed_api_handler.h` | `ChangefeedApiHandler` | Change data capture feeds |
| `retention_api_handler.h` | `RetentionApiHandler` | Data retention policies |
| `rope_api_handler.h` | `RopeApiHandler` | Rope data structure API |
| `buffer_api_handler.h` | `BufferApiHandler` | Buffer pool API |
| `keys_api_handler.h` | `KeysApiHandler` | Encryption key management |
| `pki_api_handler.h` | `PkiApiHandler` | PKI certificate management |
| `error_api_handler.h` | `ErrorApiHandler` | Structured error reporting |
| `health_error_service.h` | `HealthErrorService` | Health + error aggregation |
| `hot_reload_api_handler.h` | `HotReloadApiHandler` | Hot config reload |
| `maintenance_api_handler.h` | `MaintenanceApiHandler` | Maintenance mode |
| `profiling_api_handler.h` | `ProfilingApiHandler` | CPU/mem profiling |
| `sharding_metrics_handler.h` | `ShardingMetricsHandler` | Shard-level metrics |
| `service_mesh_api_handler.h` | `ServiceMeshApiHandler` | Service mesh control |
| `geo_topology_api_handler.h` | `GeoTopologyApiHandler` | Geographic topology |
| `classification_api_handler.h` | `ClassificationApiHandler` | Data classification |
| `ethics_api_handler.h` | `EthicsApiHandler` | AI ethics evaluation |
| `feedback_api_handler.h` | `FeedbackApiHandler` | User feedback collection |
| `reports_api_handler.h` | `ReportsApiHandler` | Scheduled reports |
| `review_scheduling_api_handler.h` | `ReviewSchedulingApiHandler` | Review workflows |
| `bpmn_api_handler.h` | `BpmnApiHandler` | BPMN workflow execution |
| `session_api_handler.h` | `SessionApiHandler` | Session CRUD |
| `tenant_manager.h` | `TenantManager`, `TenantConfig` | Multi-tenant management |
| `api_security_audit.h` | `ApiSecurityAudit` | Per-request security audit |
| `update_api_handler.h` | `UpdateApiHandler` | Document update |
| `mqtt_client_service.h` | `MqttClientService` | ✅ Reviewed |
| `shard_repair_api_handler.h` | `ShardRepairApiHandler` | ✅ Reviewed |
| `workload_fingerprint_engine.h` | `WorkloadFingerprintEngine` | ✅ Reviewed |

## gRPC Service Headers

| File | Exported Symbols | Notes |
|------|-----------------|-------|
| `themis_core_grpc_service.h` | `ThemisCoreGrpcService` | Core gRPC service |
| `llm_grpc_service.h` | `LlmGrpcService` | LLM gRPC service |
| `pitr_grpc_service.h` | `PitrGrpcService` | PITR gRPC service |
| `wal_grpc_service.h` | `WalGrpcService` | WAL gRPC streaming |
| `prompt_engineering_grpc_service.h` | `PromptEngineeringGrpcService` | Prompt gRPC service |

---

## Findings

- `rate_limiter.h` is deprecated in favour of `rate_limiter_v2.h`; maintained for ABI
  compatibility until v2.0 removal. No new consumers should include it.
- All handler interfaces follow the `IHandler::handle(Request, Response)` contract.
- No implementation code detected in public headers.
