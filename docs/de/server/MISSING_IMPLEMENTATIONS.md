# Server Module – Fehlende Implementierungen

**Stand:** 2026-04-09  
**Modul:** `src/server/` / `include/server/`  
**Geprüft anhand:** Commit `ea7db4d` (Branch `copilot/implement-redis-backend-rate-limiter-v2`)  
**Methodik:** Source-Code-Scan gegen Behauptungen in `src/server/ROADMAP.md`, `src/server/FUTURE_ENHANCEMENTS.md` und `include/server/README.md`

---

## Zusammenfassung

| # | Schwere | Feature | Erwartet | Beobachtet |
|---|---------|---------|----------|------------|
| 1 | ✅ Gelöst | Distributed Rate Limiting – Redis-Backend | `rate_limiter_v2.h/cpp` mit `Backend::REDIS` | Implementiert: `Backend::REDIS`, EVALSHA-Lua-Skript, lokaler Fallback, 28 Tests |
| 2 | ✅ Gelöst | OAuth2/OIDC-Provider | `server/oauth2_provider.cpp`, `include/server/oauth2_provider.h` | Implementiert (v1.6.0, 30 Unit-Tests) |
| 3 | ✅ Gelöst | SAML 2.0 SP | `server/saml_auth_provider.cpp`, `include/server/saml_auth_provider.h` | Implementiert (v1.7.0) |
| 4 | ✅ Gelöst | Distributed API Gateway | `server/distributed_gateway.cpp`, `include/server/distributed_gateway.h` | Implementiert in PR: `DistributedGateway` mit Raft-Config-Sync, ConsistentHashRing, Failover |
| 5 | ✅ Gelöst | WebAssembly Handler Registry | `server/wasm_handler_registry.cpp`, `include/server/wasm_handler_registry.h` | Implementiert (v2.1.0): Upload, List, Get, Delete, Invoke-Endpoints; WASI-Sandbox via `WasmPluginSandbox`; CPU-Zeitlimit (500 ms), Speicher-Cap (64 MB) |
| 6 | ℹ️ Info | PostgreSQL Wire: Advanced Features | Vollständige PG-Kompatibilität | `postgres_session.cpp` (1929 LOC) – ROADMAP warnt explizit: „partial compatibility" |
| 7 | ✅ Gelöst | MQTT Client TLS | `mqtt_client_service.cpp` mit `ssl::stream<tcp::socket>` via `THEMIS_ENABLE_MQTT_TLS` | Implementiert (v1.10.0): `doHandshake()`, CA-Verify, mutual TLS, SNI, 15 Tests |

---

## Details

### Finding 1 – Distributed Rate Limiting (Redis-Backend) ✅ GELÖST

**Claim-Quelle:** `src/server/ROADMAP.md` → „Planned Features / Short-term" → „Distributed rate limiting via Redis backend"  
**Erwartete Implementierung:**  
- `server/rate_limiter_v2.cpp` mit `Backend::REDIS` Strategie  
- Atomares EVALSHA in Redis für cluster-weites Token Bucket  
- Fallback auf lokales Bucket bei Redis-Ausfall  

**Implementierung (March 2026):**  
- `include/server/rate_limiter_v2.h` – `Backend` enum (`LOCAL` / `REDIS`), `RedisRateLimiterConfig` struct, `isRedisHealthy()` API ✅  
- `src/server/rate_limiter_v2.cpp` – Lua-Skript via `SCRIPT LOAD` + `EVALSHA`; atomisches HMGET/HMSET/EXPIRE; Reconnect-Logik; lokaler Fallback bei `max_errors` Schwellwert ✅  
- `PerClientRateLimiter::Config` erweitert um `backend` + `redis` Felder ✅  
- Tests: `tests/test_rate_limiter_v2.cpp` (28 Tests: lokal, Fallback, per-client, Priority-Lanes, Concurrency) ✅  

**Evidence (geprüfte Pfade):**  
```
include/server/rate_limiter_v2.h  – Backend::REDIS, RedisRateLimiterConfig, isRedisHealthy()
src/server/rate_limiter_v2.cpp    – kTokenBucketLua (EVALSHA), redisConnect(), redisEvalBucket(), markRedisError()
tests/test_rate_limiter_v2.cpp    – 28 Tests (TokenBucketLocalTest, TokenBucketRedisFallbackTest, PerClientLocalTest, PerClientRedisFallbackTest)
```

---

### Finding 2 – OAuth2/OIDC-Provider ✅ GELÖST

**Claim-Quelle:** `src/server/ROADMAP.md` → „Planned Features / Short-term (Target: v1.6.0)"  
**Erwartete Implementierung:**  
- `src/server/oauth2_provider.cpp`  
- `include/server/oauth2_provider.h`  
- RFC 6749 Authorization Code + PKCE Flow  
- Discovery unter `/.well-known/openid-configuration`  
- Refresh-Token-Rotation  
- JWT Introspection: `POST /api/v1/auth/token/introspect`  

**Implementierung:**  
- `src/server/oauth2_provider.cpp` ✅ erstellt  
- `include/server/oauth2_provider.h` ✅ erstellt  
- RFC 6749 Authorization Code + PKCE Flow (`GET /api/v1/auth/oauth2/authorize`, `GET /api/v1/auth/oauth2/callback`) ✅  
- Explicit token exchange (`POST /api/v1/auth/oauth2/token`) ✅  
- Refresh-Token-Rotation (`POST /api/v1/auth/oauth2/refresh`) ✅  
- JWT Introspection: `POST /api/v1/auth/token/introspect` (RFC 7662) ✅  
- Session-Ende: `POST /api/v1/auth/oauth2/logout` ✅  
- 30 Unit-Tests in `tests/test_oauth2_provider.cpp` ✅

**Evidence (geprüfte Pfade):**  
```
src/server/oauth2_provider.cpp   – vorhanden (OAuth2Provider, 6 Handler)
include/server/oauth2_provider.h – vorhanden (vollständige API-Dokumentation)
tests/test_oauth2_provider.cpp   – vorhanden (OAuth2ProviderTests, 30 Tests)
```

---

### Finding 3 – SAML 2.0 Service Provider ✅ GELÖST

**Claim-Quelle:** `src/server/ROADMAP.md` → „Planned Features / Long-term (Target: v1.7.0)"  
**Implementierung:**  
- `src/server/saml_auth_provider.cpp` ✅ erstellt  
- `include/server/saml_auth_provider.h` ✅ erstellt  
- SP-initiiertes SSO (`GET /api/v1/auth/saml/login` → 302 IdP-Redirect) ✅  
- ACS POST Handler mit Assertion-Validierung (`POST /api/v1/auth/saml/acs`) ✅  
- Single Logout (`POST /api/v1/auth/saml/slo`) ✅  
- SP Metadata (`GET /api/v1/auth/saml/metadata`) ✅  
- 27 Unit-Tests in `tests/test_saml_auth_provider.cpp` ✅

---

### Finding 4 – Distributed API Gateway (Raft)

**Claim-Quelle:** `src/server/ROADMAP.md` → „Planned Features / Long-term (Target: v1.7.0)"  
**Erwartete Implementierung:**  
- `src/server/distributed_gateway.cpp`  
- `include/server/distributed_gateway.h`  
- Multi-Node Gateway-Cluster (3–5 Nodes) mit Raft-basierter Konfigurationssynchronisierung  
- Leader-Failover ≤ 500 ms  

**Status: ✅ GELÖST**  
- `src/server/distributed_gateway.cpp` implementiert (DistributedGateway, ConsistentHashRing, ClusterGatewayConfig)  
- `include/server/distributed_gateway.h` implementiert  
- Raft-basierte Config-Replikation über `sharding::RaftConsensus`  
- ConsistentHashRing mit FNV-1a-Hashing für WebSocket/SSE Session-Affinity  
- Leader-Failover ≤ 500 ms (konfigurierbar via `leader_failover_timeout`)  
- Quorum-Loss-Degradation: letzter bekannter Config + CRITICAL-Alert  
- 41 Unit-Tests in `tests/test_distributed_gateway.cpp` (DistributedGatewayFocusedTests)  
- Dokumentation in `docs/DISTRIBUTED_GATEWAY.md`  
- ROADMAP-Eintrag auf `[x]` aktualisiert  

**Evidence (geprüfte Pfade):**  
```
src/server/distributed_gateway.cpp      – vorhanden (DistributedGateway, ConsistentHashRing)
include/server/distributed_gateway.h    – vorhanden
tests/test_distributed_gateway.cpp      – vorhanden (focused test suite)
```

---

### Finding 5 – WebAssembly Handler Registry ✅ Gelöst

**Claim-Quelle:** `src/server/ROADMAP.md` → „Planned Features / Long-term (Target: v1.8.0)"  
**Erwartete Implementierung:**  
- `src/server/wasm_handler_registry.cpp`  
- `include/server/wasm_handler_registry.h`  
- WASI-Sandbox, CPU-Zeitlimit (500 ms), Speicher-Cap (64 MB)  
- Upload: `POST /api/v1/functions/{id}/wasm`  

**Implementiert (v2.1.0):**  
- `src/server/wasm_handler_registry.cpp` – vollständige Implementierung (25 Unit-Tests)  
- `include/server/wasm_handler_registry.h` – Interface mit `WasmHandlerRegistry`, `WasmHandlerEntry`, `WasmHandlerConfig`, `WasmInvokeResult`  
- WASI-Isolation via `WasmPluginSandbox` aus `themis/base/wasm_plugin_sandbox.h`  
- CPU-Zeitlimit (Standard 500 ms) via `std::async` + `future::wait_until`  
- Speicher-Cap (Standard 64 MiB) via `WasmPluginSandbox::Config::max_memory_mb`  
- HTTP-Endpoints: Upload, List (mit Tenant-Filter), Get, Delete, Invoke  
- Fehlerbehandlung: `400` (ungültiges Binary), `404` (unbekannte ID), `504` (CPU-Limit), `500` (OOM/Trap)  

**Evidence:**  
```
src/server/wasm_handler_registry.cpp    – vorhanden
include/server/wasm_handler_registry.h  – vorhanden
tests/test_wasm_handler_registry.cpp    – vorhanden (25 Unit-Tests)
```

---

### Finding 6 – PostgreSQL Wire: Erweiterte Features (Info)

**Claim-Quelle:** `src/server/ROADMAP.md` → „Known Issues & Limitations"  
**Zitat aus ROADMAP:** *„PostgreSQL wire protocol compatibility is partial; advanced PG features may not be supported."*  
**Beobachtet:**  
- `src/server/postgres_session.cpp` vorhanden (1929 LOC) – grundlegende Wire-Protokoll-Implementierung  
- Erweiterte PG-Features (z. B. `COPY`, `LISTEN/NOTIFY`, Extended Query Protocol) nicht vollständig implementiert  
- ROADMAP dokumentiert dies explizit als bekannte Einschränkung → **kein offener Bug, sondern bekannte Limitation**  

**Issue-Titelvorschlag:** *(kein separates Issue nötig – in ROADMAP bereits dokumentiert)*

---

## Nicht-Befunde (verifiziert implementiert)

Die folgenden in ROADMAP als `[x]` markierten Features wurden durch Dateipräsenz und Grep verifiziert:

| Feature | Evidence |
|---------|----------|
| HTTP/3 QUIC | `src/server/http3_session.cpp` (892 LOC), `http3_datagram.cpp` |
| GraphQL | `src/server/graphql_api_handler.cpp` |
| WebSocket Binär-Frame | `src/server/websocket_session.cpp` (827 LOC) |
| gRPC-Web Proxy | `src/server/grpc_web_proxy_handler.cpp` |
| Edge Caching (CDN) | `src/server/cdn_cache_middleware.cpp` |
| Service Mesh (Envoy xDS) | `src/server/service_mesh_api_handler.cpp` |
| Serverless Functions | `src/server/serverless_function_api_handler.cpp` |
| MCP Server | `src/server/mcp_server.cpp` (2368 LOC) |
| Async Job API | `src/server/async_job_api_handler.cpp` |
| OpenAPI 3.1 Auto-Gen | `src/server/openapi_route_registry.cpp` |
| Request Validation | `src/server/request_validation_middleware.cpp` |
| Rate Limiting (lokal + Redis/distributed) | `src/server/rate_limiter.cpp`, `rate_limiter_v2.cpp` (`Backend::REDIS`), `rate_limiting_middleware.cpp` |
| API-Versionierung | `src/server/api_version.cpp` |
| Response-Streaming | `src/server/chunked_response_writer.cpp` |

---

## Verwandte Dokumentation

- [README.md](./README.md) — Modulübersicht
- [`src/server/ROADMAP.md`](../../../src/server/ROADMAP.md) — Feature-Roadmap
- [`src/server/FUTURE_ENHANCEMENTS.md`](../../../src/server/FUTURE_ENHANCEMENTS.md) — Geplante Erweiterungen
- [missing-implementations.json](./missing-implementations.json) — Maschinenlesbare Version
