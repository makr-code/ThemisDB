# Protocol Buffer Definitions

This directory contains Protocol Buffer (`.proto`) definitions for ThemisDB RPC communication.

**Part of:** ThemisDB v1.3.0

---

## Overview

Protocol Buffers are used for:
- **Inter-Shard Communication** - High-performance, type-safe messaging between shards
- **gRPC Services** - Service definitions for RPC endpoints
- **Data Serialization** - Efficient binary serialization for network transport

---

## Directory Structure

```
proto/
├── sharding/
│   └── shard_rpc.proto       # Inter-shard RPC service definitions
├── client/                   # Client-facing RPC (future)
│   └── themis_client.proto
├── themisdb.proto            # ThemisDB API service (document CRUD, AQL, vector search)
└── README.md                 # This file
```

---

## Building

Protocol buffer files are automatically compiled during the CMake build process when `THEMIS_BUILD_RPC_FRAMEWORK=ON`.

### Prerequisites

```bash
# Install protobuf compiler and gRPC plugin
# Ubuntu/Debian
sudo apt install protobuf-compiler libprotobuf-dev libgrpc++-dev

# macOS
brew install protobuf grpc

# Or via vcpkg
vcpkg install protobuf grpc
```

### Generated Files

The build system generates C++ code from `.proto` files:

```
build/proto/sharding/
├── shard_rpc.pb.h            # Protobuf message classes
├── shard_rpc.pb.cc           # Protobuf message implementations
├── shard_rpc.grpc.pb.h       # gRPC service stubs
└── shard_rpc.grpc.pb.cc      # gRPC service implementations
```

---

## Usage

### Including Generated Code

```cpp
#include "shard_rpc.grpc.pb.h"

// Use protobuf messages
themis::sharding::ReplicateRequest request;
request.set_shard_id("shard_001");

// Use gRPC service
auto stub = themis::sharding::ShardService::NewStub(channel);
```

### Manual Compilation (for development)

```bash
# Compile protobuf messages
protoc --cpp_out=. --proto_path=. sharding/shard_rpc.proto

# Compile gRPC services
protoc --grpc_out=. --cpp_out=. \
  --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` \
  --proto_path=. sharding/shard_rpc.proto
```

---

## Proto Files

### sharding/shard_rpc.proto

Inter-shard communication protocol for ThemisDB cluster.

**Services:**
- `ShardService` - Main inter-shard RPC service

**Key Messages:**
- `ReplicateRequest/Response` - Data replication
- `PrepareRequest/Response` - Distributed transaction (2PC)
- `MigrateRequest/Chunk` - Data migration for rebalancing
- `StatusRequest/Response` - Shard health and metrics
- `GossipMessage` - Cluster gossip protocol

**Usage:** See [RPC mTLS Inter-Shard Documentation](../docs/plugins/RPC_MTLS_INTER_SHARD.md)

---

### themisdb.proto

Client-facing gRPC API service for ThemisDB.  Mirrors the REST API surface
and covers document CRUD, AQL query execution (including server-side streaming),
and vector search operations.

**Services:**
- `ThemisDBService` - Main client-facing API service

**Key Methods:**
- `CreateDocument / GetDocument / UpdateDocument / DeleteDocument` – document CRUD (mirrors `/entities` REST routes)
- `BatchWrite / BatchRead` – bulk document operations
- `ExecuteAQL` – AQL query, returns all rows in one response
- `StreamAQL` – AQL query with server-side streaming of result rows
- `VectorSearch` – k-NN vector similarity search (mirrors `/vector/search`)
- `FilteredVectorSearch` – k-NN with attribute pre-filter
- `HybridSearch` – dense + sparse hybrid search
- `FullTextSearch` – BM25 full-text search
- `HealthCheck` – liveness / readiness probe

**Usage:** See [gRPC API Surface Future Enhancements](../src/api/FUTURE_ENHANCEMENTS.md)

---

## Style Guide

When adding new `.proto` files:

1. **Use proto3 syntax**
   ```protobuf
   syntax = "proto3";
   ```

2. **Set package namespace**
   ```protobuf
   package themis.sharding;
   ```

3. **Enable C++ optimizations**
   ```protobuf
   option cc_enable_arenas = true;
   option optimize_for = SPEED;
   ```

4. **Document messages and fields**
   ```protobuf
   // Entity represents a database entity
   message Entity {
       string uuid = 1;              // Unique entity identifier
       bytes data = 2;               // Serialized entity data
   }
   ```

5. **Use consistent field numbering**
   - Start from 1
   - Never reuse field numbers (even for deleted fields)
   - Reserve numbers for deleted fields

---

## Versioning

Protocol buffer definitions follow semantic versioning:

- **Backward compatible changes** (minor version bump):
  - Adding new fields
  - Adding new messages
  - Adding new service methods

- **Breaking changes** (major version bump):
  - Changing field types
  - Renaming fields
  - Removing required fields
  - Changing service method signatures

---

## Documentation

- [RPC Plugin Architecture](../docs/plugins/RPC_PLUGIN_ARCHITECTURE.md)
- [RPC mTLS Inter-Shard](../docs/plugins/RPC_MTLS_INTER_SHARD.md)
- [Protocol Buffers Language Guide](https://protobuf.dev/programming-guides/proto3/)
- [gRPC C++ Tutorial](https://grpc.io/docs/languages/cpp/basics/)

---

**ThemisDB v1.3.0** - RPC Framework
