# ThemisDB Server Module Headers

## Module Purpose

This directory contains public interfaces and declarations for ThemisDB's Server module. These headers define the API surface for HTTP/WebSocket/gRPC/MQTT/PostgreSQL protocol implementations, authentication, rate limiting, policy enforcement, and 40+ specialized API handlers.

## Scope

**In Scope:**
- Protocol server interfaces (HTTP, WebSocket, MQTT, PostgreSQL, gRPC)
- API handler interfaces for all endpoints
- Authentication and authorization interfaces
- Rate limiting and load shedding interfaces
- Policy enforcement interfaces
- Connection management interfaces
- API Gateway and routing interfaces

**Out of Scope:**
- Implementation details (see `../../src/server/`)
- Internal helper functions and utilities
- Protocol-specific implementation classes

## Key Components

### Core Server Interfaces

#### http_server.h
**Main HTTP/HTTPS server interface**

Exposes the primary server class with configuration options for HTTP/1.1, HTTP/2, and HTTP/3 protocols.

**Key Classes:**
- `HTTPServer` - Main server class
- `HTTPServer::Config` - Server configuration
- `HTTPSession` - Per-connection session handler

**Features:**
- TLS 1.3 support
- Multi-threading with Boost.Asio
- Connection pooling
- Graceful shutdown
- Compression (gzip, brotli, zstd)

---

#### http2_session.h
**HTTP/2 protocol support**

Implements HTTP/2 multiplexing, server push, and header compression (HPACK).

**Key Classes:**
- `HTTP2Session` - HTTP/2 session management
- `HTTP2Stream` - Individual request/response stream

---

#### http3_session.h
**HTTP/3 over QUIC protocol**

Implements HTTP/3 with QUIC transport for improved performance and 0-RTT connection establishment.

**Key Classes:**
- `HTTP3Session` - HTTP/3 session over QUIC
- `QUICConnection` - QUIC connection management

---

### API Gateway

#### api_gateway.h
**Unified API Gateway interface**

Central entry point for request routing, load balancing, and federation.

**Key Classes:**
- `APIGateway` - Main gateway class
- `APIGateway::Config` - Gateway configuration
- `RouteTarget` - Routing target enumeration

**Features:**
- Request routing (local, shard, scatter-gather, federation)
- API versioning
- Circuit breaking
- Load balancing
- Query federation across shards

---

#### api_version.h
**API versioning support**

API version negotiation and compatibility checking.

**Key Classes:**
- `APIVersion` - Version representation
- `VersionMatcher` - Version compatibility logic

---

### Authentication & Authorization

#### auth_middleware.h
**Authentication middleware interface**

Comprehensive authentication supporting JWT, API tokens, Kerberos, and USB admin auth.

**Key Classes:**
- `AuthMiddleware` - Main authentication class
- `AuthMiddleware::AuthContext` - Authenticated user context
- `AuthMiddleware::AuthResult` - Authentication result
- `AuthMiddleware::JWTConfig` - JWT configuration
- `AuthMiddleware::TokenConfig` - API token configuration

**Supported Auth Methods:**
- JWT (JSON Web Tokens) with JWKS
- Static API tokens
- Kerberos/GSSAPI (enterprise SSO)
- USB admin authentication (hardware tokens)
- mTLS (client certificates)

**Features:**
- Scope-based access control
- Tenant isolation
- Token rotation
- Group/role authorization

---

#### policy_engine.h
**Fine-grained policy enforcement**

Attribute-Based Access Control (ABAC) for data access policies.

**Key Classes:**
- `PolicyEngine` - Policy evaluation engine
- `PolicyEngine::EvalContext` - Evaluation context
- `PolicyEngine::Decision` - Policy decision with transformations

**Features:**
- Row-level security
- Column-level security (field masking)
- Time-based access restrictions
- IP whitelist/blacklist
- Data classification enforcement

---

#### ranger_adapter.h
**Apache Ranger integration**

Interface to Apache Ranger for enterprise policy management.

**Key Classes:**
- `RangerAdapter` - Ranger API integration
- `RangerPolicy` - Policy representation

**Features:**
- Policy synchronization from Ranger
- Delegated authorization decisions
- Audit logging to Ranger
- Tag-based policies

---

### Rate Limiting & Load Management

#### rate_limiter.h
**Token bucket rate limiting (v1)**

Token bucket algorithm for API rate limiting.

**Key Classes:**
- `RateLimiter` - Main rate limiter
- `RateLimitConfig` - Configuration
- `TokenBucket` - Token bucket implementation

**Features:**
- Per-IP rate limiting
- Per-user rate limiting
- Custom limits per user/IP
- IP whitelist
- Retry-After calculation

---

#### rate_limiter_v2.h
**Advanced rate limiting (v2)**

Enhanced rate limiting with distributed coordination and multiple algorithms.

**Key Classes:**
- `RateLimiterV2` - Enhanced rate limiter
- `RateLimiterV2::Config` - Configuration with backend selection

**Algorithms:**
- Token bucket
- Sliding window counter
- Leaky bucket

**Backends:**
- In-memory (single node)
- Redis (distributed)

---

#### load_shedder.h
**Adaptive load shedding**

Reject low-priority requests under overload conditions.

**Key Classes:**
- `LoadShedder` - Load shedding engine
- `LoadShedder::Config` - Thresholds and configuration
- `LoadShedder::Priority` - Request priority levels (HIGH, NORMAL, LOW)

**Monitored Metrics:**
- CPU usage
- Memory usage
- Request queue depth

---

### API Handlers (40+ Endpoints)

All handlers implement a consistent interface for request processing:

#### entity_api_handler.h
**Entity CRUD operations**

REST API for entities (relational and document models).

**Endpoints:**
- Create, read, update, delete entities
- Batch operations
- List/search with filtering

**Features:**
- Field-level encryption
- PII detection and masking
- Schema validation
- Optimistic locking (ETags)
- Partial updates

---

#### query_api_handler.h
**Query execution**

Execute AQL (Artemis Query Language) and SQL queries.

**Endpoints:**
- Execute query
- Explain query plan
- Prepare statement
- Execute prepared statement
- Cursor-based pagination

**Features:**
- AQL and SQL support
- Query optimization
- Result streaming
- Query caching
- Timeout and cancellation

---

#### vector_api_handler.h
**Vector similarity search**

Vector operations and nearest neighbor search.

**Endpoints:**
- Similarity search (ANN)
- Insert/update/delete vectors
- Index management

**Features:**
- HNSW and IVF indexes
- Multiple distance metrics (cosine, L2, dot product)
- Combined filtering and vector search
- Batch operations

---

#### graph_api_handler.h
**Graph database operations**

Property graph model with nodes, edges, and traversals.

**Endpoints:**
- Create nodes/edges
- Graph traversals (BFS, DFS)
- Shortest path algorithms
- Pattern matching

**Features:**
- Property graph model
- Dijkstra shortest path
- PageRank and centrality
- Cypher-like queries

---

#### timeseries_api_handler.h
**Time-series data**

High-throughput time-series ingestion and querying.

**Endpoints:**
- Write time-series data
- Query with time ranges
- Aggregations and downsampling
- Continuous aggregates
- Retention policies

**Features:**
- 100K+ points/sec ingestion
- Compression (Gorilla, delta-of-delta)
- Continuous aggregates (materialized views)
- Automatic retention

---

#### llm_api_handler.h
**LLM integration**

LLM operations including embeddings, completions, and RAG.

**Endpoints:**
- Generate embeddings
- Text/chat completion
- Retrieval-Augmented Generation (RAG)

**Features:**
- llama.cpp integration
- Streaming responses
- Token limits
- Context caching

---

#### lora_api_handler.h
**LoRA model management**

LoRA (Low-Rank Adaptation) adapter management for LLMs.

**Endpoints:**
- Upload LoRA adapter
- List adapters
- Apply adapter to model
- Delete adapter

---

#### changefeed_api_handler.h
**Change Data Capture (CDC)**

Real-time change streaming for data synchronization.

**Endpoints:**
- Create/delete changefeed
- Stream changes (SSE)
- Changefeed status

**Features:**
- Real-time change streaming
- Filtering by table/collection
- Checkpointing
- Multiple output formats

---

### Additional API Handler Headers

- **admin_api_handler.h** - Administrative operations
- **audit_api_handler.h** - Audit log queries
- **monitoring_api_handler.h** - System monitoring
- **schema_api_handler.h** - Schema management
- **transaction_api_handler.h** - Transaction control
- **snapshot_api_handler.h** - Snapshots and PITR
- **branch_api_handler.h** - Data versioning
- **merge_api_handler.h** - Branch merging
- **diff_api_handler.h** - Data diff operations
- **pitr_api_handler.h** - Point-in-time recovery
- **wal_api_handler.h** - WAL management
- **index_api_handler.h** - Index operations
- **cache_api_handler.h** - Cache management
- **retention_api_handler.h** - Retention policies
- **pii_api_handler.h** - PII handling
- **classification_api_handler.h** - Data classification
- **keys_api_handler.h** - Key management
- **pki_api_handler.h** - PKI operations
- **policy_api_handler.h** - Policy CRUD
- **policy_manager_api_handler.h** - Policy lifecycle
- **policy_template_api_handler.h** - Policy templates
- **policy_validation_api_handler.h** - Policy validation
- **policy_versioning_api_handler.h** - Policy versioning
- **export_api_handler.h** - Data export
- **spatial_api_handler.h** - Geospatial operations
- **content_api_handler.h** - Content management
- **feedback_api_handler.h** - Feedback collection
- **reports_api_handler.h** - Report generation
- **ethics_api_handler.h** - AI ethics
- **compliance_reporting_api_handler.h** - Compliance
- **bpmn_api_handler.h** - BPMN workflows
- **saga_api_handler.h** - Distributed sagas
- **buffer_api_handler.h** - Buffer management
- **rope_api_handler.h** - Rope operations
- **hot_reload_api_handler.h** - Hot reload
- **profiling_api_handler.h** - Performance profiling
- **prompt_api_handler.h** - Prompt management
- **voice_api_handler.h** - Voice interface
- **update_api_handler.h** - Software updates
- **sharding_metrics_handler.h** - Sharding metrics
- **review_scheduling_api_handler.h** - Review scheduling

---

### Protocol Support

#### websocket_session.h
**WebSocket protocol**

Full-duplex WebSocket communication.

**Key Classes:**
- `WebSocketSession` - WebSocket session
- `WebSocketMessage` - Message abstraction

**Features:**
- Text and binary frames
- Compression (permessage-deflate)
- Heartbeat/ping-pong

---

#### mqtt_session.h
**MQTT broker interface**

MQTT protocol for IoT devices.

**Key Classes:**
- `MQTTSession` - MQTT session
- `MQTTMessage` - MQTT message

**Features:**
- MQTT 3.1.1 and 5.0
- QoS 0, 1, 2
- Retained messages
- Last Will and Testament

---

#### postgres_session.h
**PostgreSQL wire protocol**

PostgreSQL-compatible interface for SQL clients.

**Key Classes:**
- `PostgresSession` - PostgreSQL session
- `PostgresProtocol` - Protocol implementation

**Features:**
- Simple and extended query protocols
- Prepared statements
- COPY protocol
- Binary/text formats

---

#### gRPC Service Headers

- **themis_core_grpc_service.h** - Core database gRPC
- **llm_grpc_service.h** - LLM gRPC service
- **pitr_grpc_service.h** - PITR gRPC service
- **wal_grpc_service.h** - WAL streaming gRPC

---

### Specialized Components

#### sse_connection_manager.h
**Server-Sent Events management**

Manage long-lived SSE connections for real-time data push.

**Key Classes:**
- `SSEConnectionManager` - SSE connection lifecycle
- `SSEConnection` - Individual SSE connection

**Features:**
- Connection pooling
- Event buffering and replay
- Automatic reconnection support

---

#### tenant_manager.h
**Multi-tenancy**

Tenant isolation and resource quotas.

**Key Classes:**
- `TenantManager` - Tenant lifecycle
- `TenantManager::Config` - Isolation configuration
- `Tenant` - Tenant representation

**Features:**
- Logical/physical isolation
- Storage quotas
- Connection limits
- Tenant-specific configuration

---

#### mcp_server.h
**Model Context Protocol server**

MCP server for AI/LLM integrations.

**Key Classes:**
- `MCPServer` - MCP protocol implementation
- `MCPResource` - Exposed resource

**Features:**
- Tool calling for LLMs
- Context management
- Database resource exposure

---

### Helper Interfaces

#### health_error_service.h
**Health checks and error reporting**

**Key Classes:**
- `HealthErrorService` - Health check aggregation
- `HealthStatus` - Health status representation

---

#### http_type_adapter.h
**HTTP type conversions**

Adapters between Boost.Beast types and internal types.

---

## Architecture Patterns

### Interface Design Principles

1. **Header-Only Where Possible**
   - Template classes defined in headers
   - Small utility functions inline
   - Reduces link dependencies

2. **Forward Declarations**
   - Minimize include chains
   - Reduce compilation time
   - Clear dependency boundaries

3. **Consistent Naming**
   - Classes: PascalCase (e.g., `HTTPServer`)
   - Methods: camelCase (e.g., `registerHandler`)
   - Constants: UPPER_SNAKE_CASE (e.g., `MAX_CONNECTIONS`)

4. **Configuration Structs**
   - Nested `Config` structs for options
   - Sensible defaults
   - Builder pattern where appropriate

5. **Result Types**
   - Use `Result<T, Error>` for error handling
   - Avoid exceptions in hot paths
   - Optional<T> for nullable values

---

## Usage Examples

### Including Server Headers

```cpp
// Include main server
#include "server/http_server.h"

// Include specific protocol
#include "server/websocket_session.h"
#include "server/postgres_session.h"

// Include API handlers
#include "server/entity_api_handler.h"
#include "server/query_api_handler.h"

// Include middleware
#include "server/auth_middleware.h"
#include "server/rate_limiter.h"
#include "server/load_shedder.h"
```

---

### Creating a Custom API Handler

```cpp
#include "server/api_handler_interface.h"

class CustomAPIHandler : public IAPIHandler {
public:
    CustomAPIHandler(std::shared_ptr<StorageEngine> storage,
                     std::shared_ptr<ConcernsContext> concerns)
        : storage_(storage), concerns_(concerns) {}
    
    http::response<http::string_body> handle(
        const http::request<http::string_body>& req,
        const AuthMiddleware::AuthContext& auth_ctx) override {
        
        concerns_->logger()->info("Processing custom request for user: {}", 
                                  auth_ctx.user_id);
        
        // Custom logic here
        
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::content_type, "application/json");
        res.body() = R"({"status":"ok"})";
        res.prepare_payload();
        return res;
    }
    
    std::string getPath() const override { return "/api/v1/custom"; }
    
    std::vector<http::verb> getSupportedMethods() const override {
        return {http::verb::get, http::verb::post};
    }
    
    std::vector<std::string> getRequiredScopes() const override {
        return {"custom:access"};
    }

private:
    std::shared_ptr<StorageEngine> storage_;
    std::shared_ptr<ConcernsContext> concerns_;
};
```

---

## Integration Points

### With Core Module
Server headers use ConcernsContext for observability:
```cpp
#include "core/concerns/concerns_context.h"
```

### With Storage Module
API handlers interact with storage:
```cpp
#include "storage/storage_engine.h"
```

### With Security Module
Authentication and encryption:
```cpp
#include "security/encryption.h"
#include "security/key_provider.h"
```

### With Index Module
Vector, graph, spatial operations:
```cpp
#include "index/vector_index.h"
#include "index/graph_index.h"
#include "index/spatial_index.h"
```

---

## Dependencies

### Internal Headers
- `core/concerns/` - Observability interfaces
- `storage/` - Storage interfaces
- `query/` - Query execution interfaces
- `index/` - Index interfaces
- `security/` - Security interfaces
- `utils/` - Utility headers

### External Dependencies
- **Boost.Beast** - HTTP server types
- **Boost.Asio** - Async I/O types
- **OpenSSL** - TLS types
- **nlohmann/json** - JSON handling
- **Protocol Buffers** - gRPC types (optional)

---

## Build Configuration

### CMake Integration

```cmake
# Include server headers
target_include_directories(my_app PRIVATE
    ${CMAKE_SOURCE_DIR}/include/server
)

# Link server library
target_link_libraries(my_app themis-server)
```

### Conditional Compilation

Some headers use preprocessor flags for optional features:

```cpp
#ifdef THEMIS_ENABLE_WEBSOCKET
#include "server/websocket_session.h"
#endif

#ifdef THEMIS_ENABLE_GRPC
#include "server/themis_core_grpc_service.h"
#endif

#ifdef THEMIS_ENABLE_MQTT
#include "server/mqtt_session.h"
#endif
```

---

## Implementation Details

For implementation details, see:
- Source code: `../../src/server/`
- Documentation: `../../docs/src/server/`
- Examples: `../../examples/server/`

---

## Status

**Production Ready** (as of v1.5.0)

✅ **Stable Interfaces:**
- HTTP server and protocol interfaces
- API handler interfaces
- Authentication interfaces
- Rate limiting interfaces
- Policy enforcement interfaces
- WebSocket, PostgreSQL, gRPC interfaces

⚠️ **Beta Interfaces:**
- HTTP/3 session interface
- MQTT session interface
- MCP server interface
- API Gateway query federation

---

## Related Documentation

- [Server Module Implementation](../../src/server/README.md)
- [Future Enhancements](FUTURE_ENHANCEMENTS.md)
- [Architecture Overview](../../ARCHITECTURE.md)
- [API Documentation](../../docs/api/)
- [Core Module Headers](../core/README.md)
- [Storage Module Headers](../storage/README.md)

---

## Contributing

When contributing server headers:

1. Maintain backward compatibility
2. Use forward declarations to minimize includes
3. Document all public APIs with Doxygen comments
4. Follow naming conventions
5. Add usage examples in comments
6. Update this README with new headers

For detailed contribution guidelines, see [CONTRIBUTING.md](../../CONTRIBUTING.md).

---

## See Also

- [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md) - Planned interface improvements
- [Implementation README](../../src/server/README.md) - Server implementation guide
- [Core Headers](../core/README.md) - Core module interfaces
- [Storage Headers](../storage/README.md) - Storage module interfaces
