# ThemisDB Server Module

**Status:** `current` | **Validated:** 2026-03-10 (Commit `a04b89b`) | **Version:** v1.7.0

## Module Purpose

The Server module provides ThemisDB's complete API surface, network protocol implementations, and client-facing services. Built on Boost.Beast and Boost.Asio, it handles HTTP/1.1, HTTP/2, HTTP/3, WebSocket, MQTT, PostgreSQL wire protocol, and gRPC, exposing a comprehensive REST API with 40+ specialized endpoints for multi-model data operations, governance, and observability.

## Relevant Interfaces

| Interface / File | Role |
|-----------------|------|
| `server.cpp` | Main server entry point and lifecycle management |
| `api_handler.cpp` | HTTP request routing and handler dispatch |
| `llm_api_handler.cpp` | LLM inference API handler (INFER, RAG, EMBED) |
| `rpc/` | RPC handler infrastructure for gRPC services |
| `middleware/` | Auth, logging, and rate limiting middleware |

## Scope

**In Scope:**
- HTTP/1.1, HTTP/2, HTTP/3 server implementation with TLS 1.3
- RESTful API with 40+ specialized handlers
- WebSocket support for real-time notifications
- MQTT broker integration for IoT use cases
- PostgreSQL wire protocol for SQL compatibility
- gRPC services for high-performance RPC
- API Gateway with routing, versioning, and load balancing
- Authentication & authorization (JWT, Kerberos, API tokens, USB admin auth)
- Rate limiting (token bucket, sliding window, distributed)
- Load shedding and circuit breaking
- Server-Sent Events (SSE) for changefeeds
- Multi-tenancy with tenant isolation
- Policy enforcement engine (Apache Ranger integration)
- Request/response transformation and validation
- Metrics, tracing, and audit logging
- Model Context Protocol (MCP) server for AI integrations

**Out of Scope:**
- Data storage and persistence (handled by storage module)
- Query parsing and execution (handled by query module)
- Index management (handled by index module)
- Client SDK implementations (handled by sdks/ directory)

## Key Components

### HTTP Server (Core)

#### HTTPServer
**Location:** `http_server.cpp`, `../include/server/http_server.h`

Main HTTP/HTTPS server built on Boost.Beast with async I/O and multi-threading.

**Features:**
- **Multi-Protocol Support**: HTTP/1.1, HTTP/2, HTTP/3
- **TLS 1.3**: Secure connections with modern cipher suites
- **Connection Pooling**: Efficient resource management
- **Async I/O**: Non-blocking operations for high concurrency
- **Keep-Alive**: Persistent connections for reduced latency
- **Compression**: Gzip, Brotli, Zstd response compression
- **CORS**: Cross-origin resource sharing configuration
- **Static File Serving**: Documentation and web UI hosting
- **Graceful Shutdown**: Clean connection termination

**Configuration:**
```cpp
HTTPServer::Config config;
config.host = "0.0.0.0";
config.port = 8080;
config.https_port = 8443;
config.num_threads = std::thread::hardware_concurrency();
config.max_connections = 10000;
config.read_timeout_ms = 30000;
config.write_timeout_ms = 30000;
config.enable_compression = true;
config.enable_cors = true;
config.tls_cert_path = "/etc/themis/certs/server.crt";
config.tls_key_path = "/etc/themis/certs/server.key";

HTTPServer server(storage, config);
server.start();
```

**Thread Safety:**
- Multiple worker threads handle concurrent requests
- Thread-local storage for per-request context
- Lock-free request routing via API Gateway

**Performance Characteristics:**
- **Throughput**: 50K-200K req/sec (depends on handler complexity)
- **Latency**: p50 <5ms, p99 <50ms (network + handler time)
- **Connections**: 10K+ concurrent connections supported

---

#### HTTP/2 Session
**Location:** `http2_session.cpp`, `../include/server/http2_session.h`

HTTP/2 support with multiplexing, server push, and header compression.

**Features:**
- Stream multiplexing (multiple requests per connection)
- Header compression (HPACK)
- Server push for proactive resource delivery
- Flow control and priority
- Binary framing for efficiency

**Use Cases:**
- Modern web browsers
- High-latency networks (reduces handshakes)
- Server-initiated data push

---

#### HTTP/3 Session
**Location:** `http3_session.cpp`, `../include/server/http3_session.h`

HTTP/3 support over QUIC protocol for improved performance.

**Features:**
- UDP-based transport (QUIC)
- 0-RTT connection establishment
- Built-in TLS 1.3
- Loss recovery without head-of-line blocking
- Connection migration (IP address changes)

**Benefits:**
- 30-50% latency reduction vs HTTP/2
- Better mobile network performance
- Faster page loads

---

### API Gateway

#### APIGateway
**Location:** `api_gateway.cpp`, `../include/server/api_gateway.h`

Unified entry point for all API requests with routing, versioning, and federation.

**Features:**
- **Request Routing**: Route to local handlers or remote shards
- **Load Balancing**: Distribute load across backend nodes
- **API Versioning**: Support multiple API versions (v1, v2, v3)
- **Query Federation**: Scatter-gather for distributed queries
- **Circuit Breaking**: Fail fast for unhealthy backends
- **Request Transformation**: Rewrite requests/responses
- **Metrics**: Per-endpoint latency and error rates
- **Sharding**: Consistent hashing for data locality

**Architecture:**
```
Client → API Gateway → [Auth] → [Rate Limit] → [Load Shedding]
                          ↓
                    Route Decision
                          ↓
        ┌─────────────────┴─────────────────┐
        ↓                                     ↓
  Local Handler                         Shard Router
  (Single Node)                        (Distributed)
        ↓                                     ↓
  Response Aggregation ←─────────────────────┘
        ↓
    Client
```

**Routing Strategies:**
- **LOCAL**: Execute on current node
- **SHARD**: Route to specific shard by key
- **SCATTER_GATHER**: Query all shards and merge results
- **FEDERATION**: Federated query across multiple shards

**Example:**
```cpp
APIGateway::Config config;
config.enable_sharding = true;
config.enable_circuit_breaker = true;
config.enable_query_federation = true;

APIGateway gateway(config);
gateway.registerHandler("/api/v1/entities", entity_handler);
gateway.registerHandler("/api/v1/query", query_handler);
gateway.start();
```

---

### Authentication & Authorization

#### AuthMiddleware
**Location:** `auth_middleware.cpp`, `../include/server/auth_middleware.h`

Comprehensive authentication and authorization with multiple auth methods.

**Supported Auth Methods:**
- **JWT Tokens**: Keycloak, Auth0, custom JWT providers
- **API Tokens**: Static bearer tokens for service accounts
- **Kerberos/GSSAPI**: Enterprise SSO integration
- **USB Admin Auth**: Hardware token for admin operations
- **Certificate Auth**: mTLS client certificates

**Features:**
- Scope-based access control
- Tenant isolation from JWT claims
- Token rotation and revocation
- JWKS (JSON Web Key Set) integration
- Group/role-based authorization

**Configuration:**
```cpp
AuthMiddleware::JWTConfig jwt_config;
jwt_config.jwks_url = "https://auth.example.com/.well-known/jwks.json";
jwt_config.expected_issuer = "https://auth.example.com";
jwt_config.expected_audience = "themisdb-api";
jwt_config.scope_claim = "roles";
jwt_config.tenant_claim = "tenant_id";

auth_middleware.enableJWT(jwt_config);

// Add static API tokens
AuthMiddleware::TokenConfig token_config;
token_config.token = "sk-prod-abc123...";
token_config.user_id = "service-account-1";
token_config.tenant_id = "tenant-001";
token_config.scopes = {"read:entities", "write:entities"};
auth_middleware.addToken(token_config);
```

**Authorization Flow:**
```
1. Extract token from Authorization header
2. Validate token (JWT signature or static token lookup)
3. Extract user_id, tenant_id, scopes
4. Check required scopes for endpoint
5. Inject AuthContext into request
6. Log auth decision to audit log
```

**Scopes:**
- `read:entities`, `write:entities`
- `read:query`, `write:query`
- `admin:all` (superuser)
- `tenant:manage` (tenant management)
- Custom scopes per API endpoint

---

#### OAuth2Provider
**Location:** `oauth2_provider.cpp`, `../include/server/oauth2_provider.h`

Server-layer OAuth2/OIDC provider implementing the full Authorization Code Grant
with PKCE (RFC 7636 / RFC 6749) and JWT token introspection (RFC 7662).  Bridges
the auth-layer `OIDCProvider` and `OAuthPKCEFlow` to HTTP endpoints.

**Endpoints:**
- `GET  /api/v1/auth/oauth2/authorize` – Generate PKCE challenge + authorization URL
- `GET  /api/v1/auth/oauth2/callback` – Handle IdP redirect, exchange code for tokens
- `POST /api/v1/auth/oauth2/token` – Explicit code exchange for server-side clients
- `POST /api/v1/auth/oauth2/refresh` – Refresh token rotation (RFC 6749 §6)
- `POST /api/v1/auth/token/introspect` – Local JWT validation (RFC 7662)
- `POST /api/v1/auth/oauth2/logout` – Best-effort session termination

**Integration:**
```cpp
#include "server/oauth2_provider.h"

// Configure the provider
OAuth2Provider::Config cfg;
cfg.oidc.issuer_url           = "https://keycloak.example.com/realms/production";
cfg.oidc.client_id            = "themisdb";
cfg.oidc.client_secret        = "";            // empty for public (PKCE) clients
cfg.oidc.scopes               = {"openid", "email", "groups"};
cfg.oidc.expected_audience    = "themisdb";
cfg.redirect_uri              = "https://myapp.example.com/auth/callback";
cfg.state_ttl                 = std::chrono::seconds{600}; // 10-minute CSRF window

// Optional: wrap the IdP access_token in an internal session token
cfg.token_factory = [](const std::string& access_token) -> std::string {
    return createInternalSession(access_token);
};

OAuth2Provider provider(cfg);

// ── Browser-redirect flow ──────────────────────────────────────────────────
// Step 1: client calls GET /api/v1/auth/oauth2/authorize
auto auth = provider.handleAuthorize();
// → { "authorization_url": "https://keycloak.../auth?code_challenge=...",
//     "state":             "a3f8c1...",
//     "code_verifier":     "<store client-side, never send to IdP>" }

// Step 2: IdP redirects to redirect_uri?code=AUTH_CODE&state=a3f8c1...
//         Client calls GET /api/v1/auth/oauth2/callback?code=...&state=...
auto tokens = provider.handleCallback(auth_code, state);
// → { "access_token": "...", "token_type": "Bearer",
//     "expires_in": 3600, "refresh_token": "...", "id_token": "..." }

// ── Refresh ───────────────────────────────────────────────────────────────
auto new_tokens = provider.handleRefresh(old_refresh_token);

// ── Introspect ────────────────────────────────────────────────────────────
auto info = provider.handleIntrospect(bearer_token);
// → { "active": true, "sub": "user@example.com", "exp": 1712345678,
//     "iss": "https://keycloak...", "aud": "themisdb", "groups": [...] }
// → { "active": false }   — on expired / invalid token
```

**Error responses** always contain a `status_code` field:
- `400` – missing/invalid parameters (code, state, verifier, refresh_token, token)
- `400` – PKCE verifier mismatch or unknown/expired state
- `401` – IdP-rejected token or invalid/expired access token
- `500` – internal or IdP connectivity error

---

#### SamlAuthProvider
**Location:** `saml_auth_provider.cpp`, `../include/server/saml_auth_provider.h`

Server-layer SAML 2.0 Service Provider for enterprise SSO.  Handles SP-initiated
login, Assertion Consumer Service (ACS), Single Logout (SLO), and SP metadata.

**Endpoints:**
- `GET  /api/v1/auth/saml/login` – SP-initiated SSO redirect
- `POST /api/v1/auth/saml/acs` – Assertion Consumer Service
- `POST /api/v1/auth/saml/slo` – Single Logout
- `GET  /api/v1/auth/saml/metadata` – SP SAML metadata XML

**Integration:**
```cpp
#include "server/saml_auth_provider.h"

SamlAuthProvider::Config cfg;
cfg.saml.sp_entity_id      = "https://myapp.example.com/saml/metadata";
cfg.saml.sp_acs_url        = "https://myapp.example.com/saml/acs";
cfg.saml.idp_sso_url       = "https://idp.example.com/sso";
cfg.saml.idp_entity_id     = "https://idp.example.com/metadata";
cfg.saml.idp_certificate_pem = IDP_CERT;
cfg.idp_slo_url            = "https://idp.example.com/slo";

SamlAuthProvider saml(cfg);

// Login redirect
auto login  = saml.handleLogin();     // → { "redirect_url": "...", "request_id": "..." }

// ACS callback
auto result = saml.handleAcs(saml_response_b64);
// → { "token": "...", "user_id": "...", "email": "...", "attributes": {...} }
```

---

#### PolicyEngine
**Location:** `policy_engine.cpp`, `../include/server/policy_engine.h`

Fine-grained policy enforcement for data access control.

**Features:**
- Attribute-Based Access Control (ABAC)
- Row-level security (filter query results)
- Column-level security (mask sensitive fields)
- Time-based access restrictions
- IP whitelist/blacklist
- Data classification enforcement

**Policy Language:**
```json
{
  "policy_id": "pii-access-policy",
  "effect": "ALLOW",
  "principal": "group:data-scientists",
  "resource": "table:users:*",
  "actions": ["read"],
  "conditions": {
    "ip_whitelist": ["10.0.0.0/8"],
    "time_range": {"start": "09:00", "end": "17:00"},
    "data_classification": {"exclude": ["PII_HIGH"]}
  }
}
```

**Example:**
```cpp
PolicyEngine engine;
engine.loadPolicy(policy_json);

// Check access
PolicyEngine::EvalContext ctx;
ctx.user_id = "alice";
ctx.groups = {"data-scientists"};
ctx.resource = "table:users:123";
ctx.action = "read";
ctx.ip_address = "10.1.2.3";

auto decision = engine.evaluate(ctx);
if (decision.allowed) {
    // Apply column masking if needed
    auto masked_result = decision.applyTransforms(raw_result);
    return masked_result;
}
```

---

#### RangerAdapter
**Location:** `ranger_adapter.cpp`, `../include/server/ranger_adapter.h`

Integration with Apache Ranger for enterprise policy management.

**Features:**
- Sync policies from Ranger admin server
- Delegate authorization decisions to Ranger
- Audit logging to Ranger audit store
- Tag-based policies
- Policy caching for performance

---

### Rate Limiting & Load Management

#### RateLimiter (v1)
**Location:** `rate_limiter.cpp`, `../include/server/rate_limiter.h`

Token bucket rate limiting for API endpoints.

**Algorithm:** Token Bucket
- Bucket capacity = burst size
- Refill rate = sustained rate
- Each request consumes 1 token

**Configuration:**
```cpp
RateLimitConfig config;
config.bucket_capacity = 1000;      // 1000 burst
config.refill_rate = 100.0 / 60.0;  // 100 req/min
config.per_ip_enabled = true;
config.per_user_enabled = true;
config.whitelist_ips = {"10.0.0.0/8", "192.168.1.0/24"};

RateLimiter limiter(config);

if (!limiter.checkLimit(client_ip, user_id)) {
    return HTTP_429_TOO_MANY_REQUESTS;
}
```

**Features:**
- Per-IP rate limiting
- Per-user rate limiting
- Custom limits for specific users
- IP whitelist
- Retry-After header calculation

---

#### RateLimiterV2
**Location:** `rate_limiter_v2.cpp`, `../include/server/rate_limiter_v2.h`

Advanced rate limiting with priority lanes and optional Redis backend for cluster-wide distributed limiting.

**Key classes:** `TokenBucketRateLimiter`, `PerClientRateLimiter`, `RedisRateLimiterConfig`

**Algorithm:** Token bucket (local in-process or Redis-backed)

**Backends:**
- `Backend::LOCAL` – In-process token bucket (default, backward-compatible)
- `Backend::REDIS` – Cluster-wide atomic token bucket via Redis `EVALSHA`; automatic fallback to local on Redis error

**Features:**
- Priority lanes: HIGH / NORMAL / LOW
- Per-client rate limiting via `PerClientRateLimiter`
- Distributed rate limiting across cluster nodes (Redis `Backend::REDIS`)
- Graceful fallback to local bucket on Redis unavailability
- Health observability via `isRedisHealthy()`
- Metrics: `getTotalRequests()`, `getTotalRejections()`

---

#### LoadShedder
**Location:** `load_shedder.cpp`, `../include/server/load_shedder.h`

Adaptive request rejection under overload conditions.

**Metrics Monitored:**
- CPU usage (threshold: 95%)
- Memory usage (threshold: 90%)
- Request queue depth (threshold: 1000)
- Disk I/O wait time

**Priority Classes:**
- **HIGH**: Admin operations, health checks (never shed)
- **NORMAL**: Regular CRUD operations (shed at 90% capacity)
- **LOW**: Analytics queries, batch jobs (shed at 70% capacity)

**Example:**
```cpp
LoadShedder::Config config;
config.cpu_threshold = 0.95;
config.memory_threshold = 0.90;
config.queue_depth_threshold = 1000;

LoadShedder shedder(config);

// Update metrics periodically
shedder.updateLoad(cpu_usage, memory_usage, queue_depth);

// Check before processing request
if (shedder.shouldReject(LoadShedder::Priority::NORMAL)) {
    return HTTP_503_SERVICE_UNAVAILABLE;
}
```

**Rejection Strategy:**
- At 70% load: Shed LOW priority
- At 90% load: Shed LOW + NORMAL priority
- At 95% load: Shed everything except HIGH priority

---

### API Handlers (40+ Endpoints)

All API handlers follow a consistent interface pattern:

```cpp
class IAPIHandler {
public:
    virtual http::response<http::string_body> handle(
        const http::request<http::string_body>& req,
        const AuthMiddleware::AuthContext& auth_ctx
    ) = 0;
};
```

#### Entity API Handler
**Location:** `entity_api_handler.cpp`, `../include/server/entity_api_handler.h`

CRUD operations for entities (relational and document models).

**Endpoints:**
- `POST /api/v1/entities` - Create entity
- `GET /api/v1/entities/{id}` - Read entity
- `PUT /api/v1/entities/{id}` - Update entity
- `DELETE /api/v1/entities/{id}` - Delete entity
- `GET /api/v1/entities` - List/search entities
- `POST /api/v1/entities/batch` - Batch operations

**Features:**
- Field-level encryption
- PII detection and masking
- Schema validation
- Optimistic locking (ETags)
- Partial updates (PATCH semantics)

---

#### Query API Handler
**Location:** `query_api_handler.cpp`, `../include/server/query_api_handler.h`

Query execution for AQL (Artemis Query Language) and SQL.

**Endpoints:**
- `POST /api/v1/query` - Execute query
- `POST /api/v1/query/explain` - Explain query plan
- `POST /api/v1/query/prepare` - Prepare statement
- `POST /api/v1/query/execute-prepared` - Execute prepared
- `GET /api/v1/query/cursor/{cursor_id}` - Fetch more results

**Features:**
- AQL and SQL support
- Query planning and optimization
- Cursor-based pagination
- Query timeout and cancellation
- Result streaming
- Query caching

---

#### Vector API Handler
**Location:** `vector_api_handler.cpp`, `../include/server/vector_api_handler.h`

Vector similarity search and embedding operations.

**Endpoints:**
- `POST /api/v1/vectors/search` - Similarity search
- `POST /api/v1/vectors/insert` - Insert vectors
- `PUT /api/v1/vectors/{id}` - Update vector
- `DELETE /api/v1/vectors/{id}` - Delete vector
- `GET /api/v1/vectors/indexes` - List vector indexes

**Features:**
- HNSW and IVF index support
- Cosine, L2, dot product distance metrics
- Batch insert for efficiency
- Filter combined with vector search
- Approximate nearest neighbors (ANN)

---

#### Graph API Handler
**Location:** `graph_api_handler.cpp`, `../include/server/graph_api_handler.h`

Graph database operations (nodes, edges, traversals).

**Endpoints:**
- `POST /api/v1/graph/nodes` - Create node
- `POST /api/v1/graph/edges` - Create edge
- `POST /api/v1/graph/traverse` - Graph traversal
- `GET /api/v1/graph/shortest-path` - Shortest path
- `POST /api/v1/graph/pattern-match` - Pattern matching

**Features:**
- Property graph model
- BFS/DFS traversals
- Dijkstra shortest path
- PageRank, centrality calculations
- Cypher-like query support

---

#### Timeseries API Handler
**Location:** `timeseries_api_handler.cpp`, `../include/server/timeseries_api_handler.h`

Time-series data ingestion and querying.

**Endpoints:**
- `POST /api/v1/timeseries/write` - Write time-series data
- `GET /api/v1/timeseries/query` - Query time-series
- `POST /api/v1/timeseries/aggregate` - Aggregations
- `GET /api/v1/timeseries/continuous-aggs` - List continuous aggregates
- `POST /api/v1/timeseries/retention` - Set retention policy

**Features:**
- High-throughput ingestion (100K+ points/sec)
- Downsampling and aggregation
- Continuous aggregates (materialized views)
- Compression (Gorilla, Delta-of-delta)
- Retention policies

---

#### LLM API Handler
**Location:** `llm_api_handler.cpp`, `../include/server/llm_api_handler.h`

LLM integration for embeddings, completions, and RAG.

**Endpoints:**
- `POST /api/v1/llm/embeddings` - Generate embeddings
- `POST /api/v1/llm/completions` - Text completion
- `POST /api/v1/llm/chat` - Chat completion
- `POST /api/v1/llm/rag` - Retrieval-Augmented Generation

**Features:**
- llama.cpp integration
- LoRA adapter management
- Streaming responses
- Token counting and limits
- Context caching

---

#### LoRA API Handler
**Location:** `lora_api_handler.cpp`, `../include/server/lora_api_handler.h`

LoRA (Low-Rank Adaptation) model management.

**Endpoints:**
- `POST /api/v1/lora/upload` - Upload LoRA adapter
- `GET /api/v1/lora/list` - List adapters
- `POST /api/v1/lora/apply` - Apply adapter to model
- `DELETE /api/v1/lora/{id}` - Delete adapter

---

#### Changefeed API Handler
**Location:** `changefeed_api_handler.cpp`, `../include/server/changefeed_api_handler.h`

Change Data Capture (CDC) for real-time data streaming.

**Endpoints:**
- `POST /api/v1/changefeeds/create` - Create changefeed
- `GET /api/v1/changefeeds/{id}` - Get changefeed status
- `DELETE /api/v1/changefeeds/{id}` - Stop changefeed
- `GET /api/v1/changefeeds/{id}/stream` - Stream changes (SSE)

**Features:**
- Real-time change streaming
- Filtering by table/collection
- Checkpointing for recovery
- Multiple output formats (JSON, Avro)

---

#### Admin API Handler
**Location:** `admin_api_handler.cpp`, `../include/server/admin_api_handler.h`

Administrative operations and system management.

**Endpoints:**
- `GET /api/v1/admin/health` - Health check
- `GET /api/v1/admin/metrics` - Prometheus metrics
- `POST /api/v1/admin/shutdown` - Graceful shutdown
- `POST /api/v1/admin/backup` - Trigger backup
- `GET /api/v1/admin/config` - Get configuration

---

#### Audit API Handler
**Location:** `audit_api_handler.cpp`, `../include/server/audit_api_handler.h`

Audit log query and management.

**Endpoints:**
- `GET /api/v1/audit/logs` - Query audit logs
- `GET /api/v1/audit/summary` - Audit summary
- `POST /api/v1/audit/export` - Export audit logs

---

#### Monitoring API Handler
**Location:** `monitoring_api_handler.cpp`, `../include/server/monitoring_api_handler.h`

System monitoring and observability.

**Endpoints:**
- `GET /api/v1/monitoring/metrics` - System metrics
- `GET /api/v1/monitoring/traces` - Distributed traces
- `GET /api/v1/monitoring/logs` - Query logs
- `GET /api/v1/monitoring/dashboards` - List dashboards

---

### Additional API Handlers

- **Schema API**: Schema management and evolution
- **Transaction API**: Multi-statement transactions
- **Snapshot API**: Point-in-time snapshots
- **Branch API**: Data versioning and branching
- **Merge API**: Branch merging with conflict resolution
- **Diff API**: Data diff between versions
- **PITR API**: Point-in-time recovery
- **WAL API**: Write-ahead log management
- **Index API**: Index creation and management
- **Cache API**: Cache management and statistics
- **Retention API**: Data retention policies
- **PII API**: PII detection and handling
- **Classification API**: Data classification
- **Keys API**: Encryption key management
- **PKI API**: PKI certificate management
- **Policy API**: Policy CRUD operations
- **Policy Manager API**: Policy lifecycle management
- **Policy Template API**: Policy templates
- **Policy Validation API**: Policy validation
- **Policy Versioning API**: Policy versioning
- **Export API**: Data export (CSV, JSON, Parquet)
- **Spatial API**: Geospatial operations
- **Content API**: Content management (files, documents)
- **Feedback API**: User feedback collection
- **Reports API**: Report generation
- **Ethics API**: AI ethics and bias detection
- **Compliance API**: Compliance reporting
- **BPMN API**: Business process management
- **SAGA API**: Distributed saga orchestration
- **Buffer API**: Buffer management
- **Rope API**: Rope data structure operations
- **Hot Reload API**: Configuration hot reload
- **Profiling API**: Performance profiling
- **Prompt API**: Prompt management for LLMs
- **Voice API**: Voice interface integration
- **Update API**: Software updates
- **Sharding Metrics API**: Sharding health metrics
- **Review Scheduling API**: Data review scheduling

---

### Protocol Support

#### WebSocket Session
**Location:** `websocket_session.cpp`, `../include/server/websocket_session.h`

WebSocket support for bi-directional real-time communication.

**Features:**
- Full-duplex communication
- Message framing (text/binary)
- Heartbeat/ping-pong
- Compression (permessage-deflate)

**Use Cases:**
- Real-time dashboards
- Change notifications
- Chat applications
- Live query results

---

#### MQTT Session
**Location:** `mqtt_session.cpp`, `../include/server/mqtt_session.h`

MQTT broker integration for IoT devices.

**Features:**
- MQTT 3.1.1 and 5.0 support
- Publish/subscribe
- QoS 0, 1, 2
- Retained messages
- Last Will and Testament

**Use Cases:**
- IoT sensor data ingestion
- Device command and control
- Edge computing integration

---

#### PostgreSQL Session
**Location:** `postgres_session.cpp`, `../include/server/postgres_session.h`

PostgreSQL wire protocol for SQL client compatibility.

**Features:**
- Binary and text format support
- Prepared statements
- Transaction control
- Extended query protocol
- COPY protocol for bulk loading

**Compatible Clients:**
- psql command-line tool
- pgAdmin
- DBeaver
- Any PostgreSQL driver (JDBC, libpq, etc.)

**Example:**
```bash
psql -h localhost -p 5432 -U themis -d default
themis=> SELECT * FROM users WHERE age > 30;
```

---

#### gRPC Services
**Location:** `themis_core_grpc_service.cpp`, `llm_grpc_service.cpp`, `pitr_grpc_service.cpp`, `wal_grpc_service.cpp`

High-performance RPC services using gRPC.

**Services:**
- **ThemisCoreGRPC**: Core database operations
- **LLMServiceGRPC**: LLM operations
- **PITRServiceGRPC**: Point-in-time recovery
- **WALServiceGRPC**: WAL streaming for replication

**Features:**
- HTTP/2 transport
- Protocol Buffers serialization
- Bi-directional streaming
- Built-in auth and TLS
- Load balancing support

---

### Specialized Components

#### SSE Connection Manager
**Location:** `sse_connection_manager.cpp`, `../include/server/sse_connection_manager.h`

Server-Sent Events for real-time data push.

**Features:**
- Long-lived HTTP connections
- Server → client event streaming
- Automatic reconnection
- Event ID tracking for replay
- Connection pooling and lifecycle management

**Use Cases:**
- Changefeed streaming
- Real-time notifications
- Progress updates for long-running operations

**Example:**
```cpp
SSEConnectionManager sse_manager;

// Client connects
auto connection_id = sse_manager.createConnection(request);

// Server pushes events
sse_manager.sendEvent(connection_id, "data-changed",
                      R"({"table":"users","op":"insert"})");

// Client disconnects
sse_manager.closeConnection(connection_id);
```

---

#### Tenant Manager
**Location:** `tenant_manager.cpp`, `../include/server/tenant_manager.h`

Multi-tenancy with data isolation and resource quotas.

**Features:**
- Tenant provisioning and deprovisioning
- Data isolation (physical or logical)
- Resource quotas (storage, CPU, connections)
- Tenant-specific configuration
- Cross-tenant data access prevention

**Configuration:**
```cpp
TenantManager::Config config;
config.isolation_mode = TenantManager::IsolationMode::LOGICAL;
config.default_storage_quota_gb = 100;
config.default_connection_quota = 1000;

TenantManager tenant_mgr(config);

// Create tenant
tenant_mgr.createTenant("tenant-001", {
    .storage_quota_gb = 500,
    .connection_quota = 5000,
    .enabled = true
});

// Check tenant quota
if (!tenant_mgr.checkQuota("tenant-001", quota_type)) {
    return HTTP_429_QUOTA_EXCEEDED;
}
```

---

#### MCP Server
**Location:** `mcp_server.cpp`, `../include/server/mcp_server.h`

Model Context Protocol server for AI/LLM integrations.

**Features:**
- Expose database as MCP resources
- LLM tool calling
- Context management
- Sampling and embeddings

**Use Cases:**
- AI agents with database access
- RAG (Retrieval-Augmented Generation)
- LLM-powered applications

---

## Architecture

### Layered Architecture

```
┌──────────────────────────────────────────────────────────────────┐
│                      Client Layer                                 │
│  (HTTP, WebSocket, MQTT, PostgreSQL, gRPC clients)              │
└──────────────────────────────────────────────────────────────────┘
                              ↓
┌──────────────────────────────────────────────────────────────────┐
│                      Protocol Layer                               │
│  HTTP/1.1/2/3, WebSocket, MQTT, PostgreSQL, gRPC                │
│  (http_server, websocket_session, mqtt_session, postgres_session)│
└──────────────────────────────────────────────────────────────────┘
                              ↓
┌──────────────────────────────────────────────────────────────────┐
│                       API Gateway                                 │
│  Routing, Load Balancing, Circuit Breaking, Versioning          │
│  (api_gateway)                                                   │
└──────────────────────────────────────────────────────────────────┘
                              ↓
┌──────────────────────────────────────────────────────────────────┐
│                   Middleware Pipeline                             │
│  Auth → Rate Limiting → Load Shedding → Policy Enforcement      │
│  (auth_middleware, rate_limiter, load_shedder, policy_engine)  │
└──────────────────────────────────────────────────────────────────┘
                              ↓
┌──────────────────────────────────────────────────────────────────┐
│                      API Handlers                                 │
│  40+ specialized handlers (entity, query, vector, graph, etc.)  │
└──────────────────────────────────────────────────────────────────┘
                              ↓
┌──────────────────────────────────────────────────────────────────┐
│                    Core Services                                  │
│  Storage, Query, Index, Transaction, Security                   │
└──────────────────────────────────────────────────────────────────┘
```

### Request Flow

**Typical Request:**
```
1. Client sends HTTP request
2. HTTPServer accepts connection (async I/O)
3. Parse request (method, path, headers, body)
4. Route to APIGateway
5. APIGateway determines handler (local vs shard)
6. Middleware pipeline:
   a. Auth: Validate JWT/token → Extract user_id, tenant_id
   b. Rate Limiting: Check token bucket → 429 if exceeded
   c. Load Shedding: Check system load → 503 if overloaded
   d. Policy Enforcement: Check ABAC policies → 403 if denied
7. Call API Handler (e.g., EntityAPIHandler)
8. Handler calls storage/query/index modules
9. Response transformation and serialization
10. Compression (if enabled and Accept-Encoding present)
11. Send response to client
12. Log metrics (latency, status code)
13. Audit log (if configured)
```

**WebSocket Connection:**
```
1. Client initiates WebSocket handshake
2. HTTPServer upgrades connection
3. WebSocketSession manages bi-directional messages
4. Messages routed to appropriate handlers
5. Server can push messages asynchronously
6. Connection kept alive with heartbeats
7. Graceful close on client disconnect
```

### Thread Safety Model

**Connection Handling:**
- One I/O thread per CPU core (Boost.Asio thread pool)
- Each connection handled asynchronously (non-blocking)
- Thread-local storage for per-request context
- Lock-free request routing via API Gateway

**Shared State:**
- AuthMiddleware: Thread-safe token validation (read-optimized)
- RateLimiter: Atomic token bucket updates
- PolicyEngine: Immutable policy cache (lock-free reads)
- TenantManager: Thread-safe tenant registry

**Handler Execution:**
- Handlers are stateless and thread-safe
- Shared storage/index access via thread-safe interfaces
- No mutable global state in handlers

---

## Integration Points

### With Storage Module
All API handlers interact with storage:
```cpp
// EntityAPIHandler
auto result = storage_engine_->get(key);
```

**Used For:**
- CRUD operations on entities
- Query execution
- Transaction coordination

---

### With Query Module
Query API handler delegates to query engine:
```cpp
// QueryAPIHandler
QueryEngine query_engine(storage, index_manager);
auto result = query_engine.execute(aql_query);
```

**Used For:**
- AQL/SQL parsing and execution
- Query optimization
- Result set generation

---

### With Index Module
Vector, Graph, Spatial APIs use specialized indexes:
```cpp
// VectorAPIHandler
auto neighbors = vector_index_->knnSearch(embedding, k);
```

**Used For:**
- Vector similarity search
- Graph traversals
- Geospatial queries

---

### With Core Module
All handlers use ConcernsContext for observability:
```cpp
EntityAPIHandler(storage, concerns_context);
// Automatic logging, tracing, metrics
```

**Used For:**
- Request logging
- Distributed tracing
- Metrics collection
- Result caching

---

### With Security Module
Auth and encryption integration:
```cpp
// Field-level encryption via AuthContext
auto decrypted = encryption_->decrypt(encrypted_field, auth_ctx.tenant_id);
```

**Used For:**
- JWT validation
- Field-level encryption/decryption
- Audit logging

---

## API/Usage Examples

### Basic HTTP Server Setup

```cpp
#include "server/http_server.h"
#include "storage/storage_engine.h"

// Create storage
auto storage = std::make_shared<StorageEngine>(...);

// Configure server
HTTPServer::Config config;
config.host = "0.0.0.0";
config.port = 8080;
config.num_threads = 8;

// Create server
HTTPServer server(storage, config);

// Start server
server.start();

// Server is now accepting requests on port 8080
```

---

### API Handler Registration

```cpp
#include "server/http_server.h"
#include "server/entity_api_handler.h"

auto entity_handler = std::make_shared<EntityAPIHandler>(storage, concerns_ctx);

server.registerHandler("/api/v1/entities", entity_handler);
server.registerHandler("/api/v1/entities/{id}", entity_handler);
```

---

### Authentication Configuration

```cpp
#include "server/auth_middleware.h"

AuthMiddleware auth;

// JWT configuration
AuthMiddleware::JWTConfig jwt_config;
jwt_config.jwks_url = "https://auth.example.com/.well-known/jwks.json";
jwt_config.expected_issuer = "https://auth.example.com";
jwt_config.scope_claim = "roles";
auth.enableJWT(jwt_config);

// Static API tokens
AuthMiddleware::TokenConfig token;
token.token = "sk-prod-abc123";
token.user_id = "api-service";
token.scopes = {"read:entities", "write:entities"};
auth.addToken(token);

// Use in request pipeline
auto auth_result = auth.authenticate(request);
if (!auth_result.authorized) {
    return HTTP_401_UNAUTHORIZED;
}
```

---

### Rate Limiting

```cpp
#include "server/rate_limiter.h"

RateLimitConfig config;
config.bucket_capacity = 1000;
config.refill_rate = 100.0 / 60.0;  // 100 req/min

RateLimiter limiter(config);

// Check rate limit before processing request
if (!limiter.checkLimit(client_ip, user_id)) {
    auto retry_after = limiter.getRetryAfterMs(client_ip);
    return http::response<http::string_body>{
        http::status::too_many_requests,
        request.version()
    }
    .set(http::field::retry_after, std::to_string(retry_after / 1000))
    .body("Rate limit exceeded");
}
```

---

### WebSocket Integration

```cpp
#include "server/websocket_session.h"

WebSocketSession session(tcp_stream);

// Upgrade HTTP connection to WebSocket
session.upgrade(http_request);

// Receive messages
session.async_read([](const std::string& message) {
    // Handle incoming message
    process_message(message);
});

// Send messages
session.send("Hello from server!");

// Close connection
session.close();
```

---

### SSE for Changefeeds

```cpp
#include "server/sse_connection_manager.h"
#include "server/changefeed_api_handler.h"

SSEConnectionManager sse_manager;

// Client subscribes to changefeed via SSE
auto connection_id = sse_manager.createConnection(request);

// Changefeed pushes updates
changefeed.subscribe([&](const Change& change) {
    auto event_data = change.toJSON();
    sse_manager.sendEvent(connection_id, "data-changed", event_data);
});

// Client disconnects
sse_manager.closeConnection(connection_id);
```

---

## Dependencies

### Internal Dependencies
- **themis/base/interfaces**: Interface definitions
- **core/concerns**: Logging, tracing, metrics
- **storage**: Data persistence
- **query**: Query execution
- **index**: Index management
- **security**: Authentication, encryption
- **utils**: Utilities (JSON, validation, etc.)

### External Dependencies
- **Boost.Beast** (required): HTTP/WebSocket server
- **Boost.Asio** (required): Async I/O
- **OpenSSL** (required): TLS/SSL support
- **jwt-cpp** (optional): JWT validation
- **nlohmann/json** (required): JSON parsing
- **Protocol Buffers** (optional): gRPC support
- **gRPC** (optional): RPC services
- **MQTT-CPP** (optional): MQTT broker
- **libpq** (optional): PostgreSQL protocol

### Build Configuration
```cmake
# Link server module
target_link_libraries(my_app themis-server)

# Dependencies
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)

# Optional features
option(THEMIS_ENABLE_GRPC "Enable gRPC services" ON)
option(THEMIS_ENABLE_WEBSOCKET "Enable WebSocket" ON)
option(THEMIS_ENABLE_MQTT "Enable MQTT" ON)
option(THEMIS_ENABLE_POSTGRES_PROTOCOL "Enable PostgreSQL wire protocol" ON)
```

---

## Performance Characteristics

### Throughput
- **Simple GET requests**: 100K-200K req/sec (cache hits)
- **CRUD operations**: 50K-100K req/sec (storage operations)
- **Complex queries**: 5K-20K req/sec (depends on query complexity)
- **WebSocket messages**: 50K-100K msg/sec
- **SSE connections**: 10K+ concurrent streams

### Latency (p50 / p99)
- **Auth middleware**: <100μs / <500μs
- **Rate limiter**: <50μs / <200μs
- **Entity CRUD**: <5ms / <50ms (network + storage)
- **Query execution**: <10ms / <100ms (simple queries)
- **Vector search**: <10ms / <50ms (HNSW index)

### Resource Usage
- **Memory**: ~2-4GB base + ~10KB per connection
- **CPU**: 1 core per 10K req/sec (avg)
- **Network**: 1Gbps handles ~50K req/sec (avg 2KB response)

### Tuning Recommendations
- Increase `num_threads` for CPU-bound handlers
- Enable HTTP/2 for latency-sensitive clients
- Use connection pooling for gRPC services
- Enable response compression for large payloads
- Tune rate limits based on load testing

---

## Known Limitations

1. **Single-Node Bottleneck**
   - All requests route through single API Gateway
   - Horizontal scaling requires external load balancer
   - No built-in sharding (use API Gateway sharding feature)

2. **WebSocket Scalability**
   - Each WebSocket holds a connection (memory overhead)
   - Recommend <10K concurrent WebSockets per node
   - Use SSE for one-way communication (lower overhead)

3. **Authentication Overhead**
   - JWT validation adds 100-500μs latency per request
   - JWKS fetching can cause intermittent spikes
   - Recommendation: Use API tokens for service-to-service

4. **Rate Limiting Granularity**
   - Per-IP rate limiting can affect NAT'd clients
   - Distributed rate limiting available via `TokenBucketRateLimiter` with `Backend::REDIS` (`rate_limiter_v2.h`)
   - Enable by setting `cfg.backend = TokenBucketRateLimiter::Backend::REDIS` with appropriate `cfg.redis` connection config

5. **Policy Engine Performance**
   - Complex ABAC policies can add latency
   - Recommendation: Cache policy decisions
   - Optimize policy evaluation order

6. **Long-Running Queries**
   - No automatic query timeout (client-side only)
   - Can block worker threads
   - Recommendation: Use async query execution

7. **HTTP/3 Support**
   - Experimental in v1.5.x
   - Not all clients support HTTP/3
   - Fallback to HTTP/2 if QUIC fails

---

## Status

**Production Ready** (as of v1.5.0)

✅ **Stable Features:**
- HTTP/1.1 and HTTP/2 servers
- REST API (40+ handlers)
- JWT and API token authentication
- Rate limiting (token bucket)
- Load shedding
- WebSocket support
- PostgreSQL wire protocol
- gRPC services
- SSE connection management
- Multi-tenancy
- Policy enforcement
- Audit logging

⚠️ **Beta Features:**
- HTTP/3 support
- MQTT broker integration
- Distributed rate limiting (v2)
- Query federation
- MCP server

🔬 **Experimental:**
- WebAssembly handlers
- GraphQL API
- OpenAPI auto-generation
- API analytics dashboard

---

## Related Documentation

- [API Gateway Architecture](../../docs/server/api_gateway_architecture.md)
- [Authentication Guide](../../docs/server/authentication.md)
- [Rate Limiting Strategies](../../docs/server/rate_limiting.md)
- [Multi-Tenancy Setup](../../docs/server/multi_tenancy.md)
- [WebSocket Integration](../../docs/server/websocket.md)
- [PostgreSQL Protocol](../../docs/server/postgres_protocol.md)
- [gRPC Services](../../docs/server/grpc_services.md)
- [HTTP Server Refactoring Plan](../../HTTP_SERVER_REFACTORING_ACTION_PLAN.md)
- [Core Module](../core/README.md) - Cross-cutting concerns
- [Storage Module](../storage/README.md) - Data persistence
- [Query Module](../../docs/query/README.md) - Query execution

---

## Contributing

When contributing to the server module:

1. Follow REST API best practices
2. Add auth checks to all endpoints
3. Implement rate limiting for public endpoints
4. Add integration tests for new handlers
5. Update OpenAPI spec for new endpoints
6. Consider backward compatibility for API changes

For detailed contribution guidelines, see [CONTRIBUTING.md](../../CONTRIBUTING.md).

---

## See Also

- [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md) - Planned server improvements
- [Core Module](../core/README.md) - Dependency injection and concerns
- [Storage Module](../storage/README.md) - Data persistence layer
- [docs/de/server/README.md](../../docs/de/server/README.md) - German secondary documentation (Übersicht, Komponenten, Protokolle)
- [docs/de/server/missing-implementations.md](../../docs/de/server/missing-implementations.md) - Reality-check findings (March 2026)

## Scientific References

1. Belshe, M., Peon, R., & Thomson, M. (2015). **Hypertext Transfer Protocol Version 2 (HTTP/2)**. RFC 7540. IETF. https://doi.org/10.17487/RFC7540

2. Fette, I., & Melnikov, A. (2011). **The WebSocket Protocol**. RFC 6455. IETF. https://doi.org/10.17487/RFC6455

3. gRPC Authors. (2023). **gRPC: A High-Performance, Open-Source Universal RPC Framework**. CNCF Project. https://grpc.io/

4. Postel, J. (1980). **Transmission Control Protocol**. RFC 793. IETF. https://doi.org/10.17487/RFC0793

5. Rescorla, E. (2018). **The Transport Layer Security (TLS) Protocol Version 1.3**. RFC 8446. IETF. https://doi.org/10.17487/RFC8446

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.
