# Server Module – Fehlende Implementierungen

**Stand:** 2026-03-10  
**Modul:** `src/server/` / `include/server/`  
**Geprüft anhand:** Commit `a04b89b` (Branch `copilot/update-server-module-docs`)  
**Methodik:** Source-Code-Scan gegen Behauptungen in `src/server/ROADMAP.md`, `src/server/FUTURE_ENHANCEMENTS.md` und `include/server/README.md`

---

## Zusammenfassung

| # | Schwere | Feature | Erwartet | Beobachtet |
|---|---------|---------|----------|------------|
| 1 | 🟠 Mittel | Distributed Rate Limiting – Redis-Backend | `rate_limiter_v2.h/cpp` mit `Backend::REDIS` | `rate_limiter_v2.cpp` vorhanden (239 LOC), aber kein Redis-Pfad |
| 2 | ✅ Gelöst | OAuth2/OIDC-Provider | `server/oauth2_provider.cpp`, `include/server/oauth2_provider.h` | Implementiert (v1.6.0, 30 Unit-Tests) |
| 3 | ✅ Gelöst | SAML 2.0 SP | `server/saml_auth_provider.cpp`, `include/server/saml_auth_provider.h` | Implementiert (v1.7.0) |
| 4 | ✅ Gelöst | Distributed API Gateway | `server/distributed_gateway.cpp`, `include/server/distributed_gateway.h` | Implementiert in PR: `DistributedGateway` mit Raft-Config-Sync, ConsistentHashRing, Failover |
| 5 | 🟡 Niedrig | WebAssembly Handler Registry | `server/wasm_handler_registry.cpp`, `include/server/wasm_handler_registry.h` | Keine dieser Dateien existiert |
| 6 | ℹ️ Info | PostgreSQL Wire: Advanced Features | Vollständige PG-Kompatibilität | `postgres_session.cpp` (1929 LOC) – ROADMAP warnt explizit: „partial compatibility" |

---

## Details

### Finding 1 – Distributed Rate Limiting (Redis-Backend)

**Claim-Quelle:** `src/server/ROADMAP.md` → „Planned Features / Short-term" → „Distributed rate limiting via Redis backend"  
**Erwartete Implementierung:**  
- `server/rate_limiter_v2.cpp` mit `Backend::REDIS` Strategie  
- Atomares EVALSHA in Redis für cluster-weites Token Bucket  
- Fallback auf lokales Bucket bei Redis-Ausfall  

**Beobachtet:**  
- `src/server/rate_limiter_v2.cpp` existiert (239 LOC, Stand 2026-03-09)  
- `include/server/rate_limiter_v2.h` existiert  
- Grep auf `REDIS`, `redis`, `evalsha`, `Backend::REDIS` → **kein Treffer**  
- Nur lokales Token-Bucket implementiert  

**Evidence (geprüfte Pfade):**  
```
src/server/rate_limiter_v2.cpp   – vorhanden, 239 LOC, kein Redis-Pfad
include/server/rate_limiter_v2.h – vorhanden, kein Backend::REDIS Enum
```

**Issue-Titelvorschlag:** `feat(server): implement Redis backend for RateLimiterV2 (distributed token bucket)`  
**Label-Vorschläge:** `area/server`, `type/feature`, `priority/p1`, `effort/medium`

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
- Tests in `tests/test_distributed_gateway.cpp` (DistributedGatewayFocusedTests)  

**Evidence (geprüfte Pfade):**  
```
src/server/distributed_gateway.cpp      – vorhanden (DistributedGateway, ConsistentHashRing)
include/server/distributed_gateway.h    – vorhanden
tests/test_distributed_gateway.cpp      – vorhanden (focused test suite)
```

---

### Finding 5 – WebAssembly Handler Registry

**Claim-Quelle:** `src/server/ROADMAP.md` → „Planned Features / Long-term (Target: v1.8.0)"  
**Erwartete Implementierung:**  
- `src/server/wasm_handler_registry.cpp`  
- `include/server/wasm_handler_registry.h`  
- WASI-Sandbox, CPU-Zeitlimit (500 ms), Speicher-Cap (64 MB)  
- Upload: `POST /api/v1/functions/{id}/wasm`  

**Beobachtet:**  
- Keine dieser Dateien existiert  
- `serverless_function_api_handler.cpp` (vorhandenes Serverless) arbeitet mit eingebettetem C++ Code, nicht mit WASM  

**Evidence (geprüfte Pfade):**  
```
src/server/wasm_handler_registry.cpp    – nicht vorhanden
include/server/wasm_handler_registry.h  – nicht vorhanden
src/server/serverless_function_api_handler.cpp – vorhanden (non-WASM Serverless)
```

**Issue-Titelvorschlag:** `feat(server): WebAssembly API handler sandbox via WASI (v1.8.0)`  
**Label-Vorschläge:** `area/server`, `type/feature`, `priority/p3`, `effort/x-large`

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
| Rate Limiting (lokal) | `src/server/rate_limiter.cpp`, `rate_limiter_v2.cpp`, `rate_limiting_middleware.cpp` |
| API-Versionierung | `src/server/api_version.cpp` |
| Response-Streaming | `src/server/chunked_response_writer.cpp` |

---

## Verwandte Dokumentation

- [README.md](./README.md) — Modulübersicht
- [`src/server/ROADMAP.md`](../../../src/server/ROADMAP.md) — Feature-Roadmap
- [`src/server/FUTURE_ENHANCEMENTS.md`](../../../src/server/FUTURE_ENHANCEMENTS.md) — Geplante Erweiterungen
- [missing-implementations.json](./missing-implementations.json) — Maschinenlesbare Version
