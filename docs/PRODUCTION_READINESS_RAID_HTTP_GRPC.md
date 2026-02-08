# Production-Readiness: RAID Redundancy, HTTP Handlers, and gRPC Services

## Overview

This document describes the production-readiness improvements for ThemisDB's sharding, RAID redundancy, HTTP handler wiring, and gRPC service configuration.

## 1. RAID Redundancy Strategy Integration

### Status: ✅ IMPLEMENTED

The RedundancyStrategy has been integrated into the EntityApiHandler write path, enabling RAID-style redundancy modes (MIRROR, STRIPE, PARITY, RAID6, GEO_MIRROR) at runtime.

### Implementation Details

#### Components Modified

1. **include/server/entity_api_handler.h**
   - Added `feature_raid` flag to `EntityApiConfig`
   - Added optional RAID parameters: `redundancy_manager`, `hash_ring`, `shard_topology`
   - Added forward declarations for sharding components

2. **src/server/entity_api_handler.cpp**
   - Integrated RAID write logic into `handlePut()` method
   - Added comprehensive logging and tracing
   - Graceful fallback when RAID components are unavailable

3. **src/server/http_server.cpp**
   - Updated EntityApiHandler initialization
   - Added `feature_raid` flag (disabled by default)

### How RAID Integration Works

```cpp
// In EntityApiHandler::handlePut()
if (redundancy_manager_ && hash_ring_ && shard_topology_ && config_.feature_raid) {
    // Get redundancy strategy for collection
    auto strategy = redundancy_manager_->getStrategy(table);
    
    // Apply RAID write
    auto write_result = strategy->write(
        key,           // document_id
        data_bytes,    // data
        table,         // collection
        *hash_ring_,   // consistent hash ring
        *shard_topology_, // shard topology
        write_handler  // callback for shard writes
    );
    
    // Log and trace results
    THEMIS_INFO("RAID write successful: {} shards written", 
                write_result.written_shards.size());
}
```

### Configuration

RAID redundancy is **disabled by default** and requires explicit configuration:

```yaml
# config/raid_entity_config.example.yaml
entity_api:
  feature_raid: true

raid:
  enabled: true
  collections:
    users:
      mode: MIRROR
      replication_factor: 3
      write_concern: MAJORITY
```

### Initialization

To enable RAID at startup, initialize the components in `main_server.cpp`:

```cpp
// Create RAID components
auto hash_ring = std::make_shared<ConsistentHashRing>(100);
hash_ring->addNode("shard-0");
hash_ring->addNode("shard-1");
hash_ring->addNode("shard-2");

auto shard_topology = std::make_shared<ShardTopology>();

auto redundancy_manager = std::make_shared<CollectionRedundancyManager>();
RedundancyConfig raid_config;
raid_config.mode = RedundancyMode::MIRROR;
raid_config.replication_factor = 3;
redundancy_manager->setCollectionConfig("users", raid_config);

// Pass to HttpServer constructor (requires extending constructor)
// OR pass to EntityApiHandler when initializing in HttpServer
```

### Testing

A comprehensive integration test has been created:

- **tests/test_entity_api_raid_integration.cpp**
  - Tests RAID disabled by default
  - Tests RAID enabled with components
  - Tests graceful fallback when components missing

### Monitoring

RAID operations are traced and logged:

- Span attributes: `raid.mode`, `raid.shards_written`, `raid.latency_ms`
- Logs: Info-level logs for successful RAID writes
- Warnings: Logged but don't fail requests if RAID optional

---

## 2. HTTP Server Handler Wiring

### Status: ✅ VERIFIED - NO CHANGES NEEDED

The HTTP server handlers (EntityApiHandler, WALApiHandler) are **already properly wired** and active in production.

### Current Implementation

#### EntityApiHandler

**Location:** `src/server/http_server.cpp` (lines 657-678)

```cpp
entity_api_ = std::make_unique<themis::server::EntityApiHandler>(
    storage_,
    secondary_index_,
    graph_index_,
    tx_manager_,
    field_encryption_,
    key_provider_,
    auth_,
    entity_config,
    spatial_index_.get(),
    changefeed_,
    wal_manager_,
    replication_coordinator_,
    multi_primary_coordinator_
);
```

**Endpoints Handled:**
- `GET /entities/:key` - Retrieve entity
- `PUT /entities/:key` - Create/update entity
- `DELETE /entities/:key` - Delete entity
- `POST /entities/batch` - Batch operations

#### WALApiHandler

**Location:** `src/server/http_server.cpp` (lines 722-727)

```cpp
wal_api_ = std::make_unique<themis::server::WALApiHandler>(
    storage_, wal_applier_, wal_manager_, replication_coordinator_, auth_,
    wal_shared_secret_, wal_hmac_secret_
);
```

**Endpoints Handled:**
- `POST /api/v1/wal/apply` - Apply WAL entries for replication

### Request Routing

Both handlers are properly integrated into the main request dispatcher in `HttpServer::handleRequest()`:

```cpp
// Entity API routes
if (target.starts_with("/entities/")) {
    if (req.method() == http::verb::get) {
        response = entity_api_->handleGet(req);
    } else if (req.method() == http::verb::put) {
        response = entity_api_->handlePut(req);
    } else if (req.method() == http::verb::delete_) {
        response = entity_api_->handleDelete(req);
    }
}

// WAL API routes
if (target == "/api/v1/wal/apply" && req.method() == http::verb::post) {
    response = wal_api_->handleApply(req);
}
```

### Verification

No changes were required. The handlers are:
1. ✅ Properly initialized with all dependencies
2. ✅ Active in request routing
3. ✅ Used in production path
4. ✅ Have proper error handling
5. ✅ Include metrics and monitoring

---

## 3. gRPC WAL Apply Service

### Status: ✅ VERIFIED - NO CHANGES NEEDED

The gRPC WAL Apply service (WalGrpcService) is **already properly registered and started** when `THEMIS_ENABLE_GRPC` is enabled.

### Current Implementation

**Location:** `src/main_server.cpp` (lines 1088-1115)

```cpp
#ifdef THEMIS_ENABLE_GRPC
// Start WAL gRPC Apply service
g_wal_grpc_service = std::make_unique<server::WalGrpcService>(wal_applier);
if (auto* wal_service = g_wal_grpc_service->service()) {
    std::string grpc_host = "0.0.0.0";
    int grpc_port = 50051;
    if (const char* h = std::getenv("THEMIS_WAL_GRPC_HOST")) grpc_host = h;
    if (const char* p = std::getenv("THEMIS_WAL_GRPC_PORT")) {
        try { grpc_port = std::stoi(p); } catch (...) {}
    }
    std::string grpc_addr = grpc_host + ":" + std::to_string(grpc_port);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(grpc_addr, grpc::InsecureServerCredentials());
    builder.RegisterService(static_cast<grpc::Service*>(wal_service));
    builder.SetMaxReceiveMessageSize(100 * 1024 * 1024);
    builder.SetMaxSendMessageSize(100 * 1024 * 1024);

    g_wal_grpc_server = builder.BuildAndStart();
    if (g_wal_grpc_server) {
        THEMIS_INFO("WAL gRPC Apply service listening on {}", grpc_addr);
    } else {
        THEMIS_WARN("Failed to start WAL gRPC Apply service (address: {})", grpc_addr);
    }
}
#endif
```

### Configuration

The gRPC service is configured via environment variables:

- `THEMIS_WAL_GRPC_HOST` - Host to bind (default: `0.0.0.0`)
- `THEMIS_WAL_GRPC_PORT` - Port to bind (default: `50051`)

### Features

1. ✅ Proper initialization with WALApplier
2. ✅ Uses grpc::ServerBuilder for registration
3. ✅ Configured message size limits (100MB)
4. ✅ Logging for success/failure
5. ✅ Graceful fallback if gRPC stubs unavailable
6. ✅ Guarded by THEMIS_ENABLE_GRPC feature flag

### Security Note

⚠️ **Production Warning:** The service currently uses `InsecureServerCredentials`. For production:

```cpp
// For production, use TLS:
auto creds = grpc::SslServerCredentials(ssl_opts);
builder.AddListeningPort(grpc_addr, creds);
```

See `config/raid_entity_config.example.yaml` for TLS configuration example.

---

## 4. Build System Compatibility

### Status: ✅ VERIFIED

All changes are compatible with the existing build system.

### Feature Flags

Relevant CMake feature flags:

| Flag | Default | Purpose |
|------|---------|---------|
| `THEMIS_ENABLE_GRPC` | ON | Inter-shard gRPC communication |
| `THEMIS_ENABLE_HTTP_SERVER` | ON | Built-in HTTP server |
| `THEMIS_ENABLE_HTTP2` | OFF | HTTP/2 protocol |
| `THEMIS_ENABLE_HTTP3` | OFF | HTTP/3 (QUIC) |

### Configuration

**Location:** `cmake/features/NetworkFeatures.cmake`

The RAID integration uses existing feature flags and doesn't require new build-time flags. Runtime configuration is done via `EntityApiConfig::feature_raid`.

---

## Summary of Changes

### Files Modified

1. **include/server/entity_api_handler.h**
   - Added RAID support declarations
   - Added `feature_raid` flag

2. **src/server/entity_api_handler.cpp**
   - Integrated RAID write path
   - Added logging and tracing

3. **src/server/http_server.cpp**
   - Updated handler initialization

### Files Created

1. **tests/test_entity_api_raid_integration.cpp**
   - Integration tests for RAID functionality

2. **config/raid_entity_config.example.yaml**
   - Configuration example for RAID

3. **docs/PRODUCTION_READINESS_RAID_HTTP_GRPC.md**
   - This documentation file

### No Changes Required

- ✅ HTTP handler wiring (already correct)
- ✅ gRPC service registration (already correct)
- ✅ Build system (compatible with existing flags)

---

## Next Steps for Production

### 1. Enable RAID (Optional)

To use RAID redundancy in production:

1. Copy and customize the example configuration:
   ```bash
   cp config/raid_entity_config.example.yaml config/raid_entity.yaml
   ```

2. Initialize RAID components in `main_server.cpp`:
   ```cpp
   auto hash_ring = std::make_shared<ConsistentHashRing>(100);
   // Add shards to ring...
   
   auto redundancy_manager = std::make_shared<CollectionRedundancyManager>();
   // Configure per-collection redundancy...
   ```

3. Pass components to HttpServer (requires constructor extension)

4. Set `feature_raid: true` in configuration

### 2. Enable gRPC TLS (Required for Production)

Replace insecure credentials with TLS:

```cpp
grpc::SslServerCredentialsOptions ssl_opts;
ssl_opts.pem_root_certs = read_file(ca_cert_path);
ssl_opts.pem_key_cert_pairs.push_back({
    read_file(server_key_path),
    read_file(server_cert_path)
});

auto creds = grpc::SslServerCredentials(ssl_opts);
builder.AddListeningPort(grpc_addr, creds);
```

### 3. Testing Recommendations

- Run existing test suite: `make test`
- Run RAID integration test: `./tests/test_entity_api_raid_integration`
- Load test with RAID enabled
- Verify metrics and logging

### 4. Monitoring

Monitor these metrics:

- `raid_write_total` - Total RAID writes
- `raid_write_latency_ms` - RAID write latency
- `raid_shards_written` - Shards written per operation
- `themis_http_requests_total{endpoint="/entities"}` - Entity endpoint usage
- `themis_grpc_requests_total{service="WalApply"}` - gRPC WAL apply requests

---

## References

- RedundancyStrategy: `include/sharding/redundancy_strategy.h`
- EntityApiHandler: `include/server/entity_api_handler.h`
- WALApiHandler: `include/server/wal_api_handler.h`
- WalGrpcService: `include/server/wal_grpc_service.h`
- Configuration example: `config/raid_entity_config.example.yaml`
- Architecture: `ARCHITECTURE.md` (Sharding section)
