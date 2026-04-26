# Server Module — Primär-Inventar
<!-- status: current | validated: 2026-04-06 | commit: 22764eeee -->

**Modul:** `server`  
**Stand:** 2026-03-10  
**Quelle:** Reality-Check gegen Sourcecode-Stand Commit `22764eeee`

---

## Primäre Dokumentationsdateien

### `src/server/`

| Datei | Typ | Status |
|-------|-----|--------|
| `src/server/README.md` | Modul-Übersicht, Komponenten, Konfiguration, Beispiele (1344 LOC) | ✅ aktuell |
| `src/server/ROADMAP.md` | Feature-Status (Phases 1–4 abgeschlossen), geplante Features | ✅ aktuell (korrigiert 2026-03-10) |
| `src/server/ARCHITECTURE.md` | Architekturprinzipien, Komponentenübersicht, Request-Flow | ✅ aktuell |
| `src/server/FUTURE_ENHANCEMENTS.md` | Geplante Erweiterungen mit 21 IEEE/IETF/ACM-Referenzen | ✅ aktuell |

### `include/server/`

| Datei | Typ | Status |
|-------|-----|--------|
| `include/server/README.md` | Public API Referenz (Headers, Klassen, Verwendung) (819 LOC) | ✅ aktuell |
| `include/server/FUTURE_ENHANCEMENTS.md` | API-level Erweiterungen, Design Constraints | ✅ aktuell |

### `src/server/rpc/`

| Datei | Typ | Status |
|-------|-----|--------|
| `src/server/rpc/README.md` | RPC Transfer Handler (Snapshot, Blob, Differential Update) | ✅ aktuell |

---

## Implementierungsdateien (Source – Auswahl nach Kategorien)

**Gesamt: 102 `.cpp` in `src/server/` + 4 in `src/server/rpc/`**

### Kern-Server (Protokoll-Implementierungen)

| Datei | Klasse / Beschreibung | LOC |
|-------|----------------------|-----|
| `src/server/http_server.cpp` | `HTTPServer` – Multi-Protokoll Async-Server | 9961 |
| `src/server/http2_session.cpp` | HTTP/2 Multiplexing, Server Push | 390 |
| `src/server/http3_session.cpp` | QUIC/HTTP3 | 892 |
| `src/server/http3_datagram.cpp` | HTTP/3 Datagramme | 264 |
| `src/server/websocket_session.cpp` | WebSocket Binär/Text-Frames | 827 |
| `src/server/mqtt_session.cpp` | MQTT Broker-Integration | 801 |
| `src/server/postgres_session.cpp` | PostgreSQL Wire-Protokoll (partiell) | 1929 |
| `src/server/mcp_server.cpp` | Model Context Protocol (3 Stubs: SQL/Cypher) | 2368 |

### API-Gateway, Auth, Rate Limiting

| Datei | Klasse / Beschreibung | LOC |
|-------|----------------------|-----|
| `src/server/api_gateway.cpp` | Routing, Versioning, Load Balancing | 975 |
| `src/server/auth_middleware.cpp` | JWT, Kerberos, API-Token, USB-Admin | 437 |
| `src/server/api_auth_config.cpp` | Endpunkt-spezifische Auth-Anforderungen | 166 |
| `src/server/api_key_mgmt_handler.cpp` | API-Schlüsselverwaltung | 263 |
| `src/server/rate_limiter.cpp` | Token Bucket, Sliding Window | 290 |
| `src/server/rate_limiter_v2.cpp` | Erweiterter Rate Limiter (lokal, kein Redis) | 239 |
| `src/server/rate_limiting_middleware.cpp` | Rate-Limit-Middleware | 179 |
| `src/server/load_shedder.cpp` | Load Shedding & Circuit Breaking | 187 |
| `src/server/ranger_adapter.cpp` | Apache Ranger Policy-Enforcement | 219 |
| `src/server/opa_adapter.cpp` | Open Policy Agent Integration | 187 |
| `src/server/tenant_manager.cpp` | Multi-Tenancy, Custom-Domain-Routing | 262 |

### Daten-API-Handler (50+ Handler, Auswahl nach LOC)

| Datei | Klasse / Beschreibung | LOC |
|-------|----------------------|-----|
| `src/server/query_api_handler.cpp` | AQL-Query-Ausführung | 3526 |
| `src/server/llm_api_handler.cpp` | LLM INFER/RAG/EMBED | 1639 |
| `src/server/voice_api_handler.cpp` | Sprach-API | 1572 |
| `src/server/schema_api_handler.cpp` | Schema-Verwaltung | 1398 |
| `src/server/lora_api_handler.cpp` | LoRA-Adapter | 1320 |
| `src/server/entity_api_handler.cpp` | CRUD Dokumente/Collections | 1227 |
| `src/server/changefeed_api_handler.cpp` | CDC/SSE-Subscriptions | 1118 |
| `src/server/graph_api_handler.cpp` | Graph-Traversal | 1098 |
| `src/server/task_scheduler_api_handler.cpp` | Task Scheduler | 984 |
| `src/server/monitoring_api_handler.cpp` | Monitoring/Metriken | 1768 |
| `src/server/rope_api_handler.cpp` | ROPE | 909 |
| `src/server/serverless_function_api_handler.cpp` | Serverless Functions | 528 |
| `src/server/graphql_api_handler.cpp` | GraphQL | 206 |
| `src/server/grpc_web_proxy_handler.cpp` | gRPC-Web Proxy | 336 |
| `src/server/cdn_cache_middleware.cpp` | Edge-Caching (CDN) | 260 |
| `src/server/service_mesh_api_handler.cpp` | Service-Mesh (Envoy xDS) | 177 |

### OpenAPI, Middleware, Sonstiges

| Datei | Klasse / Beschreibung |
|-------|-----------------------|
| `src/server/openapi_route_registry.cpp` | OpenAPI 3.1 Auto-Generierung |
| `src/server/request_validation_middleware.cpp` | JSON-Schema Request-Validierung |
| `src/server/chunked_response_writer.cpp` | Response-Streaming (chunked) |
| `src/server/api_version.cpp` | Versionierungs-Logik (`/v1/`, `/v2/`) |
| `src/server/async_job_api_handler.cpp` | Async Jobs `/v2/jobs` |
| `src/server/sse_connection_manager.cpp` | Server-Sent Events |

### RPC Transfer Handler (`src/server/rpc/`)

| Datei | Klasse / Beschreibung |
|-------|-----------------------|
| `src/server/rpc/rpc_service_impl.cpp` | gRPC Service-Implementierung |
| `src/server/rpc/snapshot_transfer_handler.cpp` | MVCC-aware Snapshot Transfer |
| `src/server/rpc/blob_transfer_handler.cpp` | Large Binary Transfer (LoRA) |
| `src/server/rpc/differential_update_engine.cpp` | Hash-basiertes Differenz-Update |

---

## Header-Dateien (Public API)

**Gesamt: 105 `.h` in `include/server/` + 3 in `include/server/rpc/`**

Wichtigste Headers (Auswahl):

| Header | Klasse | Beschreibung |
|--------|--------|--------------|
| `include/server/http_server.h` | `HTTPServer` | Multi-Protokoll-Server, Config, Session |
| `include/server/auth_middleware.h` | `AuthMiddleware` | JWT/Kerberos/API-Token/USB-Auth |
| `include/server/rate_limiter.h` | `RateLimiter` | Token Bucket, Sliding Window |
| `include/server/rate_limiter_v2.h` | `RateLimiterV2` | Erweiterter Limiter (lokal; kein Redis) |
| `include/server/api_gateway.h` | `APIGateway` | Routing, Versioning |
| `include/server/mcp_server.h` | `MCPServer` | Model Context Protocol |
| `include/server/openapi_route_registry.h` | `OpenAPIRouteRegistry` | OpenAPI 3.1 |
| `include/server/api_version_config.h` | `APIVersionConfig` | Versionierungskonfiguration |

---

## Sekundäre Dokumentationsdateien

| Datei | Beschreibung |
|-------|-------------|
| `docs/de/server/README.md` | Deutsche Überblicks-Dokumentation (Protokolle, Komponenten, Endpunkte, Security, Performance) |
| `docs/de/server/missing-implementations.md` | Reality-Check Report (6 Findings, Stand 2026-03-10) |
| `docs/de/server/missing-implementations.json` | Maschinenlesbare Version (6 Findings) |
| `docs/de/server/inventory.md` | Dieses Dokument |

---

## Weiterführende Dokumentation

- [src/server/README.md](../../../src/server/README.md) — Vollständige Modulbeschreibung
- [src/server/ROADMAP.md](../../../src/server/ROADMAP.md) — Feature-Status und Planung
- [src/server/ARCHITECTURE.md](../../../src/server/ARCHITECTURE.md) — Architekturleitfaden
- [include/server/README.md](../../../include/server/README.md) — Public API Referenz
- [src/server/FUTURE_ENHANCEMENTS.md](../../../src/server/FUTURE_ENHANCEMENTS.md) — Geplante Erweiterungen + IEEE-Referenzen
- [docs/de/server/README.md](README.md) — Sekundäre Dokumentation (Deutsch)
- [docs/de/server/missing-implementations.md](missing-implementations.md) — Befund-Report
