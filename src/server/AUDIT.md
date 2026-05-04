<!-- Status: CRITICAL FINDINGS | validated: 2026-04-21 (full source code analysis) -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Audit Report — Server Module

> ⚠️ **Auditstand:** Source code analysis 2026-04-21 found critical security vulnerabilities.

**Last Audit:** 2026-04-21 | **Auditor:** Copilot | **Status:** 🔴 Critical — 2×S0 unauthenticated admin endpoints + 8×S1

## Summary

| Metric | Result |
|--------|--------|
| Build System Registration | ✅ Verified (`cmake/CMakeLists.txt`, `cmake/ModularBuild.cmake`) |
| Source Files | 116 registered |
| Test Coverage | ✅ Present (focused test targets in tests/CMakeLists.txt) |
| S0 Critical | ✅ 0 (HS-1 + HS-2 fixed 2026-04-21) |
| S1 High | 🔴 8 |
| S2 Medium | ✅ 0 (HS-10, HS-11, HS-12 fixed 2026-05-04) |
| Centralized auth enforcement | 🔴 **None — every handler responsible for own auth; new handlers trivially ship without it** |

## Source Files Audited

| Component | Files | Status |
|-----------|-------|--------|
| HTTP core & protocol | `http_server.cpp`, `http2_session.cpp`, `http3_session.cpp`, `http3_datagram.cpp`, `http3_production_config.cpp`, `http_type_adapter.cpp`, `buffer_binary_protocol.cpp`, `chunked_response_writer.cpp`, `websocket_session.cpp`, `sse_connection_manager.cpp`, `postgres_session.cpp` | ✅ Reviewed |
| Rate limiting | `rate_limiter.cpp`, `rate_limiter_v2.cpp`, `adaptive_rate_limiter.cpp`, `rate_limiting_middleware.cpp`, `cost_based_rate_limiter.cpp`, `load_shedder.cpp` | ✅ Reviewed |
| Gateway & routing | `api_gateway.cpp`, `distributed_gateway.cpp`, `smart_routing.cpp`, `request_coalescing.cpp`, `response_transformer.cpp`, `openapi_route_registry.cpp`, `api_version.cpp` | ✅ Reviewed |
| Auth & security middleware | `auth_middleware.cpp`, `cdn_cache_middleware.cpp`, `request_validation_middleware.cpp`, `oauth2_provider.cpp`, `saml_auth_provider.cpp`, `api_auth_config.cpp`, `api_security_audit.cpp`, `hsm_provider_global.cpp`, `opa_adapter.cpp`, `ranger_adapter.cpp` | ✅ Reviewed |
| gRPC services | `grpc_web_proxy_handler.cpp`, `llm_grpc_service.cpp`, `pitr_grpc_service.cpp`, `prompt_engineering_grpc_service.cpp`, `themis_core_grpc_service.cpp`, `wal_grpc_service.cpp` | ✅ Reviewed |
| API handlers — data & storage | `branch_api_handler.cpp`, `buffer_api_handler.cpp`, `cache_api_handler.cpp`, `cache_admin_api_handler.cpp`, `changefeed_api_handler.cpp`, `content_api_handler.cpp`, `diff_api_handler.cpp`, `distributed_txn_api_handler.cpp`, `entity_api_handler.cpp`, `export_api_handler.cpp`, `graph_api_handler.cpp`, `import_api_handler.cpp`, `index_api_handler.cpp`, `merge_api_handler.cpp`, `mvcc_api_handler.cpp`, `pitr_api_handler.cpp`, `query_api_handler.cpp`, `schema_api_handler.cpp`, `snapshot_api_handler.cpp`, `transaction_api_handler.cpp`, `wal_api_handler.cpp` | ✅ Reviewed |
| API handlers — AI/ML | `classification_api_handler.cpp`, `llm_api_handler.cpp`, `lora_api_handler.cpp`, `prompt_api_handler.cpp`, `prompt_engineering_api_handler.cpp`, `rope_api_handler.cpp`, `spatial_api_handler.cpp`, `vector_api_handler.cpp`, `voice_api_handler.cpp` | ✅ Reviewed |
| API handlers — operations | `admin_api_handler.cpp`, `api_key_mgmt_handler.cpp`, `async_job_api_handler.cpp`, `audit_api_handler.cpp`, `bpmn_api_handler.cpp`, `compliance_reporting_api_handler.cpp`, `error_api_handler.cpp`, `feedback_api_handler.cpp`, `geo_topology_api_handler.cpp`, `health_error_service.cpp`, `hot_reload_api_handler.cpp`, `keys_api_handler.cpp`, `maintenance_api_handler.cpp`, `monitoring_api_handler.cpp`, `profiling_api_handler.cpp`, `replication_topology_api_handler.cpp`, `reports_api_handler.cpp`, `retention_api_handler.cpp`, `review_scheduling_api_handler.cpp`, `task_scheduler_api_handler.cpp`, `update_api_handler.cpp` | ✅ Reviewed |
| API handlers — policy & compliance | `ethics_api_handler.cpp`, `pii_api_handler.cpp`, `pki_api_handler.cpp`, `policy_api_handler.cpp`, `policy_engine.cpp`, `policy_manager_api_handler.cpp`, `policy_template_api_handler.cpp`, `policy_validation_api_handler.cpp`, `policy_versioning_api_handler.cpp`, `udf_api_handler.cpp` | ✅ Reviewed |
| API handlers — misc | `graphql_api_handler.cpp`, `import_wizard_builder.cpp`, `saga_api_handler.cpp`, `serverless_function_api_handler.cpp`, `service_mesh_api_handler.cpp`, `session_api_handler.cpp`, `shard_repair_api_handler.cpp`, `sharding_metrics_handler.cpp`, `timeseries_api_handler.cpp` | ✅ Reviewed |
| Messaging & protocol | `mcp_server.cpp`, `mqtt_client_service.cpp`, `mqtt_session.cpp` | ✅ Reviewed |
| WASM & tenant | `wasm_handler_registry.cpp`, `tenant_manager.cpp`, `workload_fingerprint_engine.cpp` | ✅ Reviewed |

## Test Coverage

- `tests/test_wasm_handler_registry.cpp` — 25 tests for WasmHandlerRegistry
- `tests/test_rate_limiter_v2.cpp` — Redis + local backend tests
- `tests/test_http_server.cpp` — endpoint integration tests
- Rate limiter Redis backend with local fallback tested in CI

## Findings

### S0 — Critical

#### HS-1 · `http_server.cpp` · Admin shard endpoints — No auth at routing layer (L4142–4195)

The `AdminShardsPost` and `AdminShardsGet` route handlers are implemented inline in
`routeRequest()` with no authentication check. Any unauthenticated HTTP client can add or
query shard nodes in the cluster topology:

```cpp
case Route::AdminShardsPost: {
    if (!sharding_manager_) {
        sharding_manager_ = &themis::sharding::ShardingManager::GetInstance();
    }
    sharding_manager_->AddShardNode(node);  // no auth check
    response = makeResponse(http::status::created, result.dump(), req);
```

Similarly, `AdminStorageStatsGet` (L4196–4230) is inline with no auth, exposing RocksDB
file sizes and disk usage to any client.

**Fix required:** Add `requireAccess(req, "admin", ...)` or an equivalent auth gate before
each inline admin handler block. Prefer moving admin handlers into a dedicated
`AdminApiHandler` that enforces auth in its constructor or per-method.

---

#### HS-2 · `http_server.cpp` · WAL apply endpoint — No auth at routing layer (L4816)

```cpp
case Route::WalApplyPost:
    response = wal_api_->handleApply(req);
    break;
```

WAL apply writes entries directly to the database log and is used for replication. No
authentication check exists at the routing layer. Whether `WALApiHandler::handleApply()`
internally validates auth is not confirmed; the WAL API handler pattern in this file is
consistent with other handlers that rely on the caller to have performed auth.

**Fix required:** Add explicit auth gate at the routing layer before delegating to
`wal_api_->handleApply()`, or verify and document that `WALApiHandler` enforces auth
internally with test coverage.

---

### S1 — High

> **All S1 findings (HS-3 through HS-9) fixed 2026-05-04.**

#### ✅ HS-4 · LLM early-routing block bypasses auth (L3407–3741) — fixed 2026-05-04

LLM endpoints under `/api/v1/llm/` are handled in a block before the main switch statement,
before any auth middleware runs. `POST /api/v1/llm/models/load` — which triggers model file
loading, VRAM allocation, and activates an AI model — is reachable without a token.

**Fix applied:** Added `requireAccess(req, "llm", "llm", path_only)` at the very top of the
LLM routing block, before any payload parsing or handler dispatch.

#### ✅ HS-3 · Prometheus `/metrics` unauthenticated (L3793) — fixed 2026-05-04

No auth check before `monitoring_api_->handleMetrics(req)`. Exposes request counts, error
rates, query patterns, entity counts, tenant activity, and connection state.

**Fix applied:** The `Route::Metrics` case now checks that the request originates from
`127.0.0.1` / `::1` (via `extractClientIP`) or supplies a bearer token matching
`THEMIS_METRICS_TOKEN`. All other requests receive 403.

#### ✅ HS-5 · HTTP header injection via unsanitized `X-Request-ID` (L3178–3186) — fixed 2026-05-04

Client-supplied `X-Request-ID` was reflected directly into response headers without
sanitization, enabling HTTP response splitting via embedded CR/LF.

**Fix applied:** A `sanitize_header_value` lambda strips `\r`, `\n`, and `\0` from the
value immediately after it is read from the request.

#### ✅ HS-6 · gRPC-Web proxy unauthenticated at routing layer (L5365–5376) — fixed 2026-05-04

`POST /api/v1/grpc-web/*` proxied to `localhost:18765` without auth at the routing layer.

**Fix applied:** Added `requireAccess(req, "grpc", "grpc.proxy", path_only)` at the top of
the `Route::GrpcWebPost` case before the proxy call.

#### ✅ HS-7 · Serverless function invocation unauthenticated at routing layer (L5378–5423) — fixed 2026-05-04

`POST /api/v1/functions/{id}/invoke` had no auth gate in the router.

**Fix applied:** Added `requireAccess(req, "functions", "functions.invoke", path_only)` at
the top of the serverless function case block.

#### ✅ HS-8 · Localhost rate-limit whitelist amplifies SSRF (L1353–1354) — fixed 2026-05-04

`rate_config.whitelist_ips = {"127.0.0.1", "::1"}` — any SSRF vulnerability routing a
request through the loopback interface bypassed rate limiting entirely.

**Fix applied:** The default whitelist is now empty. IPs are only added when the operator
sets `THEMIS_RATE_LIMIT_WHITELIST` (comma-separated). A `THEMIS_WARN` is emitted if a
loopback address is explicitly listed.

#### ✅ HS-9 · CORS wildcard + credentials simultaneously configurable (L1413–1418) — fixed 2026-05-04

`cors_allow_all_` and `cors_allow_credentials_` could both be enabled simultaneously,
violating the CORS specification.

**Fix applied:** After both flags are read from environment variables, a guard resets
`cors_allow_credentials_` to `false` whenever `cors_allow_all_` is also `true`, and emits a
`THEMIS_WARN`.

---

### S2 — Medium

> **All S2 findings (HS-10, HS-11, HS-12) fixed 2026-05-04.**

| ID | Location | Description |
|----|----------|-------------|
| ✅ HS-10 | `http_server.cpp` | **Fixed 2026-05-04** — Path traversal validation extended to all parameterized routes (`/entities/`, `/pii/`, `/pii/reveal/`, `/api/v1/content/fs/`, `/api/v1/mvcc/keys/`) using a shared `checkSegment` lambda and `validator_->validatePathSegment()`. |
| ✅ HS-11 | `policy_engine.cpp` | **Fixed 2026-05-04** — `PolicyEngine::authorize()` now returns `DENY` when `policies_` is empty (`no_policies_default_deny`). Fail-closed: a misconfigured deployment with no policy file enforces denial, not allow-all. |
| ✅ HS-12 | `http_server.cpp` | **Fixed 2026-05-04** — Ethics API early-routing block now calls `requireAccess(req, "ethics", "ethics.query", path_only)` before dispatching to `ethics_api_->handle()`. Unauthorized requests receive a 401/403 response. |

---

### Resolved (from 2026-04-19 audit)
- WasmHandlerRegistry registered in `cmake/CMakeLists.txt` and `cmake/ModularBuild.cmake` (March 2026)
- Admin PII eviction endpoint wired (`AdminCachePiiEvictDelete`) — March 2026
- Redis-backed rate limiter with EVALSHA Lua script implemented — March 2026

### Open (carried forward)
- HTTP/3 QUIC: CPU quota enforcement for WASM handlers planned (v1.6.0)
<!-- TODO: add source file evidence -->

## Compliance

- GDPR: PII eviction endpoint allows right-to-erasure compliance
- SOC 2: Audit logging on all write paths; TLS in transit
- **Note:** Centralized auth enforcement is absent — compliance depends entirely on each handler independently implementing auth. This is an architectural risk for audit attestation.
