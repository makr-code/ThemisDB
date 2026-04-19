<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Server Module (Public Headers)

All notable public API changes. Format: [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
Implementation changelog: `../../src/server/CHANGELOG.md`.

## [Unreleased]

## [1.5.0] — 2026-03-12
### Added
- `grpc_web_proxy_handler.h` — gRPC-Web browser proxy; TypeScript client stubs generated via
  `scripts/gen_grpc_web_ts.py` (package `@themisdb/client-grpc-web` v1.7.0)
- `http3_datagram.h` — HTTP/3 unreliable datagram support (RFC 9221)
- `http3_production_config.h` — production QUIC/HTTP3 tuning interface
- `cost_based_rate_limiter.h` — query-cost-aware rate limiter interface
- `adaptive_rate_limiter.h` — load-aware adaptive rate limiter
- `load_shedder.h` — overload protection via configurable shed policies
- `request_coalescing.h` — identical-request deduplication interface
- `sse_connection_manager.h` — Server-Sent Events connection registry
- `distributed_gateway.h` — multi-node gateway with consensus routing
- `lora_api_handler.h` — LoRA adapter management API handler
- `voice_api_handler.h` — voice/speech processing API handler
- `ethics_api_handler.h` — AI ethics evaluation handler
- `bpmn_api_handler.h` — BPMN workflow execution handler
- `wasm_handler_registry.h` — WASM extension handler registry

### Changed
- `api_gateway.h`: `GatewayConfig` now accepts `TenantManager` reference for tenant-aware
  dispatch
- `auth_middleware.h`: `AuthContext` extended with `device_id` and `trust_level` fields
- `rate_limiter_v2.h`: `RateLimiterV2` now supports sliding-window and token-bucket modes

### Deprecated
- `rate_limiter.h` (`IRateLimiter`) — superseded by `rate_limiter_v2.h`; will be removed in
  v2.0.0

## [1.4.0] — 2026-01-15
### Added
- `prompt_engineering_api_handler.h`, `prompt_engineering_grpc_service.h` — prompt engineering
  workflows
- `service_mesh_api_handler.h` — service mesh topology control
- `sharding_metrics_handler.h` — per-shard metrics handler
- `openapi_route_registry.h` — OpenAPI 3.x route registration
- `route_version_router.h` — version multiplexing router
- `cdn_cache_middleware.h` — CDN cache-control header injection middleware
- `buffer_binary_protocol.h` — binary framing for the buffer API
- `import_wizard_builder.h` — import wizard builder DSL
- `rope_api_handler.h` — rope data structure API handler

## [1.3.0] — 2025-09-01
### Added
- `http3_session.h` — HTTP/3 (QUIC) session interface
- `mqtt_session.h` — MQTT 3.1/5.0 session bridge
- `postgres_session.h` — PostgreSQL wire-protocol session adapter
- `mcp_server.h` — Model Context Protocol server
- `wal_grpc_service.h`, `wal_api_handler.h` — WAL streaming and management
- `pitr_grpc_service.h`, `pitr_api_handler.h` — point-in-time recovery
- `replication_topology_api_handler.h` — replication topology control
- `snapshot_api_handler.h` — database snapshot management

## [1.2.0] — 2025-05-01
### Added
- `llm_api_handler.h`, `llm_grpc_service.h` — LLM inference over REST and gRPC
- `vector_api_handler.h` — vector similarity search API
- `spatial_api_handler.h` — geospatial query API
- `timeseries_api_handler.h` — time-series data ingest and query
- `saga_api_handler.h` — saga pattern orchestration
- `distributed_txn_api_handler.h` — distributed two-phase commit

## [1.0.0] — 2024-01-01
### Added
- Initial public header set: `http_server.h`, `http2_session.h`, `websocket_session.h`,
  `api_gateway.h`, `auth_middleware.h`, `rate_limiter.h`, `tenant_manager.h`,
  `smart_routing.h`, `rpc_service_impl.h`, `query_api_handler.h`, `graph_api_handler.h`,
  `schema_api_handler.h`, `entity_api_handler.h`, `transaction_api_handler.h`,
  `admin_api_handler.h`, `monitoring_api_handler.h`, `health_error_service.h`
